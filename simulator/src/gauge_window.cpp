/**
 * @file gauge_window.cpp
 * @brief Implementation of gauge window with factory-created gauges
 */

#include "gauge_window.h"
#include "gauge_factory.h"
#include <cstdio>
#include <map>
#include <memory>

// Global callback context (maps window ID to GaugeWindow instance)
static std::map<lv_display_t*, GaugeWindow*> g_window_map;

void GaugeWindow::displayFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    auto it = g_window_map.find(disp);
    if (it == g_window_map.end()) {
        return;
    }
    
    GaugeWindow* window = it->second;
    
    void* pixels;
    int pitch;
    if (SDL_LockTexture(window->texture_, nullptr, &pixels, &pitch) == 0) {
        uint32_t* dest = reinterpret_cast<uint32_t*>(pixels);
        uint32_t* src = reinterpret_cast<uint32_t*>(px_map);
        
        for (int y = area->y1; y <= area->y2; y++) {
            int src_offset = (y - area->y1) * (area->x2 - area->x1 + 1);
            int dest_offset = y * (pitch / 4) + area->x1;
            
            for (int x = area->x1; x <= area->x2; x++) {
                dest[dest_offset++] = src[src_offset++];
            }
        }
        
        SDL_UnlockTexture(window->texture_);
    }
    
    lv_display_flush_ready(disp);
}


GaugeWindow::GaugeWindow(const WindowConfig& window_config,
                         const std::map<std::string, GaugeDefinition>& gauge_definitions,
                         int window_id)
    : window_(nullptr), renderer_(nullptr), texture_(nullptr), display_(nullptr),
      buf1_(nullptr), buf2_(nullptr), screen_(nullptr),
      width_(window_config.width), height_(window_config.height), 
      window_id_(window_id), last_x_(0), last_y_(false), is_pressed_(false) {
    
    // Create SDL window
    window_ = SDL_CreateWindow(
        "Gauge Window",
        window_config.x, window_config.y,
        width_, height_,
        SDL_WINDOW_SHOWN
    );
    
    if (!window_) {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        return;
    }
    
    // Create renderer
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        fprintf(stderr, "Failed to create SDL renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window_);
        window_ = nullptr;
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
        SDL_DestroyWindow(window_);
        renderer_ = nullptr;
        window_ = nullptr;
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
    g_window_map[display_] = this;
    lv_display_set_flush_cb(display_, displayFlushCallback);
    
    // Create main screen to hold gauges
    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen_, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_flex_flow(screen_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create gauges from configuration
    for (const auto& gauge_cfg : window_config.gauges) {
        // Find the gauge definition
        auto def_it = gauge_definitions.find(gauge_cfg.gauge_name);
        if (def_it == gauge_definitions.end()) {
            fprintf(stderr, "Gauge definition not found: %s\n", gauge_cfg.gauge_name.c_str());
            continue;
        }
        
        // Create gauge from definition
        Gauge* gauge = GaugeFactory::createGauge(screen_, def_it->second);
        if (gauge) {
            gauges_.push_back(std::unique_ptr<Gauge>(gauge));
            printf("Added gauge %s to window %d\n", gauge_cfg.gauge_name.c_str(), window_id_);
        } else {
            fprintf(stderr, "Failed to create gauge: %s\n", gauge_cfg.gauge_name.c_str());
        }
    }
    
    lv_screen_load(screen_);
    
    printf("Created gauge window %d at (%d, %d) with size %dx%d containing %zu gauges\n", 
           window_id_, window_config.x, window_config.y, width_, height_, gauges_.size());
}

GaugeWindow::~GaugeWindow() {
    // Clear gauges vector (unique_ptr will delete them)
    gauges_.clear();
    
    if (display_) {
        g_window_map.erase(display_);
    }
    
    if (screen_) {
        lv_obj_del(screen_);
        screen_ = nullptr;
    }
    
    if (display_) {
        lv_display_delete(display_);
        display_ = nullptr;
    }
    
    if (buf1_) {
        free(buf1_);
        buf1_ = nullptr;
    }
    if (buf2_) {
        free(buf2_);
        buf2_ = nullptr;
    }
    
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    printf("Destroyed gauge window %d\n", window_id_);
}


void GaugeWindow::update(const DataSource& data, const std::map<std::string, PidConfig>& pid_map) {
    // Map PID names to values from data source
    // This is a simple lookup - in reality you'd need to decode the raw OBD II values
    std::map<std::string, int> data_values;
    data_values["rpm"] = data.getRPM();
    data_values["speed"] = data.getSpeed();
    data_values["coolant_temp"] = data.getCoolantTemp();
    data_values["temp"] = data.getCoolantTemp();  // Alias
    
    // Update all gauges with their respective data
    for (auto& gauge : gauges_) {
        const auto& def = gauge->getDefinition();
        
        // Find the value for this gauge's data source
        auto value_it = data_values.find(def.data_source);
        if (value_it != data_values.end()) {
            gauge->update(value_it->second);
        }
    }
}

void GaugeWindow::render() {
    if (!isValid()) return;
    
    // Render to SDL (LVGL rendering is handled in main loop)
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

uint32_t GaugeWindow::getWindowID() const {
    if (window_) {
        return SDL_GetWindowID(window_);
    }
    return 0;
}

bool GaugeWindow::handleEvent(const SDL_Event& event) {
    // Check if event is for this window
    uint32_t our_window_id = getWindowID();
    if (our_window_id == 0) {
        return false;
    }
    
    // Filter events for this specific window
    bool is_our_window = false;
    int event_x = 0, event_y = 0;
    
    if (event.type == SDL_MOUSEMOTION && event.motion.windowID == our_window_id) {
        is_our_window = true;
        event_x = event.motion.x;
        event_y = event.motion.y;
    } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.windowID == our_window_id) {
        is_our_window = true;
        event_x = event.button.x;
        event_y = event.button.y;
    } else if (event.type == SDL_MOUSEBUTTONUP && event.button.windowID == our_window_id) {
        is_our_window = true;
        event_x = event.button.x;
        event_y = event.button.y;
    }
    
    if (!is_our_window) {
        return false;
    }
    
    // Make sure this display is active for input processing
    lv_display_set_default(display_);
    
    // Track mouse state
    last_x_ = event_x;
    last_y_ = event_y;
    
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        is_pressed_ = true;
        // Find object at this position and send click event
        lv_point_t point = {event_x, event_y};
        lv_obj_t* obj = lv_indev_search_obj(screen_, &point);
        if (obj) {
            // Send both pressed and released events
            lv_obj_send_event(obj, LV_EVENT_PRESSED, nullptr);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        is_pressed_ = false;
        // Find object at this position
        lv_point_t point = {event_x, event_y};
        lv_obj_t* obj = lv_indev_search_obj(screen_, &point);
        if (obj) {
            // Send released and clicked events
            lv_obj_send_event(obj, LV_EVENT_RELEASED, nullptr);
            lv_obj_send_event(obj, LV_EVENT_CLICKED, nullptr);
        }
    }
    
    return true;
}
