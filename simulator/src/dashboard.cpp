/**
 * @file dashboard.cpp
 * @brief Implementation of single display dashboard with gauge elements
 */

#include "dashboard.h"
#include "gauge_factory.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>

// Global callback context (maps LVGL display to Dashboard instance)
static std::map<lv_display_t*, Dashboard*> g_display_map;

void Dashboard::displayFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    auto it = g_display_map.find(disp);
    if (it == g_display_map.end()) {
        return;
    }
    
    Dashboard* dashboard = it->second;
    
    void* pixels;
    int pitch;
    if (SDL_LockTexture(dashboard->texture_, nullptr, &pixels, &pitch) == 0) {
        uint32_t* dest = reinterpret_cast<uint32_t*>(pixels);
        uint32_t* src = reinterpret_cast<uint32_t*>(px_map);
        
        int width = area->x2 - area->x1 + 1;
        
        for (int y = area->y1; y <= area->y2; y++) {
            // Source offset: LVGL buffer is sequential for the partial area
            int src_offset = (y - area->y1) * width;
            // Destination offset: SDL texture full stride
            int dest_offset = y * (pitch / 4) + area->x1;
            
            // Copy one row at a time
            memcpy(&dest[dest_offset], &src[src_offset], width * sizeof(uint32_t));
        }
        
        SDL_UnlockTexture(dashboard->texture_);
    }
    
    lv_display_flush_ready(disp);
}


Dashboard::Dashboard(const DisplayConfig& display_config,
                     const std::map<std::string, GaugeDefinition>& gauge_definitions)
    : sdl_window_(nullptr), renderer_(nullptr), texture_(nullptr), display_(nullptr),
      buf1_(nullptr), buf2_(nullptr), screen_(nullptr),
      width_(display_config.width), height_(display_config.height), 
      last_x_(0), last_y_(0), is_pressed_(false) {
    
    // Create SDL window
    sdl_window_ = SDL_CreateWindow(
        "Dashboard",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width_, height_,
        SDL_WINDOW_SHOWN
    );
    
    if (!sdl_window_) {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        return;
    }
    
    // Create renderer
    renderer_ = SDL_CreateRenderer(sdl_window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        fprintf(stderr, "Failed to create SDL renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl_window_);
        sdl_window_ = nullptr;
        return;
    }
    
    // Create texture for rendering
    texture_ = SDL_CreateTexture(renderer_,
                                 SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING,
                                 width_, height_);
    if (!texture_) {
        fprintf(stderr, "Failed to create SDL texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(sdl_window_);
        renderer_ = nullptr;
        sdl_window_ = nullptr;
        return;
    }
    
    // Initialize LVGL display
    display_ = lv_display_create(width_, height_);
    if (!display_) {
        fprintf(stderr, "Failed to create LVGL display\n");
        return;
    }
    
    // Allocate buffers
    size_t buf_size = (width_ * height_ / 10) * 4;
    buf1_ = malloc(buf_size);
    buf2_ = malloc(buf_size);
    
    if (!buf1_ || !buf2_) {
        fprintf(stderr, "Failed to allocate LVGL buffers\n");
        return;
    }
    
    // Setup LVGL display
    lv_display_set_buffers(display_, buf1_, buf2_, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(display_, LV_COLOR_FORMAT_ARGB8888);
    
    // Set this display as active for object creation
    lv_display_set_default(display_);
    
    // Register flush callback
    g_display_map[display_] = this;
    lv_display_set_flush_cb(display_, displayFlushCallback);
    
    // Create main screen to hold dashboard elements
    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen_, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
    // Disable flex layout to allow absolute positioning
    lv_obj_set_flex_flow(screen_, LV_FLEX_FLOW_COLUMN);
    
    // Create gauge elements from configuration
    for (const auto& element : display_config.elements) {
        // Find the gauge definition
        auto def_it = gauge_definitions.find(element.gauge_name);
        if (def_it == gauge_definitions.end()) {
            fprintf(stderr, "Gauge definition not found: %s\n", element.gauge_name.c_str());
            continue;
        }
        
        // Create gauge from definition
        Gauge* gauge = GaugeFactory::createGauge(screen_, def_it->second);
        if (gauge) {
            // Position and size the gauge element
            lv_obj_t* container = gauge->getContainer();
            lv_obj_clear_flag(container, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
            lv_obj_set_pos(container, element.x, element.y);
            lv_obj_set_size(container, element.width, element.height);
            
            elements_.push_back(std::unique_ptr<Gauge>(gauge));
            printf("Added element %s at (%d, %d) size %dx%d\n", 
                   element.gauge_name.c_str(), element.x, element.y, 
                   element.width, element.height);
        } else {
            fprintf(stderr, "Failed to create gauge element: %s\n", element.gauge_name.c_str());
        }
    }
    
    lv_screen_load(screen_);
    
    printf("Created dashboard %dx%d with %zu elements\n", 
           width_, height_, elements_.size());
}

Dashboard::~Dashboard() {
    if (display_) {
        g_display_map.erase(display_);
        lv_display_delete(display_);
    }
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (sdl_window_) SDL_DestroyWindow(sdl_window_);
    if (buf1_) free(buf1_);
    if (buf2_) free(buf2_);
    printf("Destroyed dashboard\n");
}

void Dashboard::update(const DataSource& data, const std::map<std::string, PidConfig>& pid_map) {
    // Build map of data values keyed by data source name
    std::map<std::string, int> data_values;
    
    data_values["rpm"] = data.getRPM();
    data_values["speed"] = data.getSpeed();
    data_values["coolant_temp"] = data.getCoolantTemp();
    data_values["temp"] = data.getCoolantTemp();  // Alias
    
    // Update each gauge element
    for (auto& element : elements_) {
        if (element) {
            element->update(data_values[element->getDataSource()]);
            
            // For RPM arc gauge, also display speed and temp
            if (element->getDefinition().name == "rpm_gauge") {
                element->setAuxiliaryValues(data.getSpeed(), data.getCoolantTemp());
            }
        }
    }
}

void Dashboard::render() {
    // Clear renderer with black background
    SDL_SetRenderDrawColor(renderer_, 0x1a, 0x1a, 0x1a, 0xFF);
    SDL_RenderClear(renderer_);
    
    // Copy texture to SDL renderer
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

bool Dashboard::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEMOTION) {
        int event_x = event.motion.x;
        int event_y = event.motion.y;
        last_x_ = event_x;
        last_y_ = event_y;
        
        if (is_pressed_) {
            // Mouse drag event
            lv_indev_search_obj(screen_, nullptr);
        }
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        is_pressed_ = true;
        int event_x = event.button.x;
        int event_y = event.button.y;
        
        lv_point_t point = {event_x, event_y};
        lv_obj_t* obj = lv_indev_search_obj(screen_, &point);
        if (obj) {
            // Send both pressed and released events
            lv_obj_send_event(obj, LV_EVENT_PRESSED, nullptr);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        is_pressed_ = false;
        // Find object at this position
        lv_point_t point = {event.button.x, event.button.y};
        lv_obj_t* obj = lv_indev_search_obj(screen_, &point);
        if (obj) {
            // Send released and clicked events
            lv_obj_send_event(obj, LV_EVENT_RELEASED, nullptr);
            lv_obj_send_event(obj, LV_EVENT_CLICKED, nullptr);
        }
    }
    
    return true;
}
