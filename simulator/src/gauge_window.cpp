/**
 * @file gauge_window.cpp
 * @brief Implementation of gauge window
 */

#include "gauge_window.h"
#include "rpm_gauge.h"
#include "speed_display.h"
#include "temp_gauge.h"
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


GaugeWindow::GaugeWindow(GaugeType gauge_type, int x, int y, int width, int height, int window_id)
    : window_(nullptr), renderer_(nullptr), texture_(nullptr), display_(nullptr),
      buf1_(nullptr), buf2_(nullptr), rpm_gauge_(nullptr), speed_display_(nullptr),
      temp_gauge_(nullptr), screen_(nullptr),
      width_(width), height_(height), window_id_(window_id), last_x_(0), last_y_(false),
      gauge_type_(gauge_type) {
    
    // Create SDL window
    window_ = SDL_CreateWindow(
        "Gauge Window",
        x, y,
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
    
    // Create main screen to hold the gauge
    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen_, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_flex_flow(screen_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create gauge based on type
    switch (gauge_type_) {
        case GAUGE_RPM:
            rpm_gauge_ = new RPMGauge(screen_);
            break;
        case GAUGE_SPEED:
            speed_display_ = new SpeedDisplay(screen_);
            break;
        case GAUGE_TEMP:
            temp_gauge_ = new TempGauge(screen_);
            break;
    }
    
    lv_screen_load(screen_);
    
    printf("Created gauge window %d at (%d, %d) with size %dx%d\n", 
           window_id_, x, y, width_, height_);
}

GaugeWindow::~GaugeWindow() {
    // Clean up gauge objects
    if (rpm_gauge_) {
        delete rpm_gauge_;
        rpm_gauge_ = nullptr;
    }
    if (speed_display_) {
        delete speed_display_;
        speed_display_ = nullptr;
    }
    if (temp_gauge_) {
        delete temp_gauge_;
        temp_gauge_ = nullptr;
    }
    
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


void GaugeWindow::update(const DataSource& data) {
    switch (gauge_type_) {
        case GAUGE_RPM:
            if (rpm_gauge_) {
                rpm_gauge_->update(data.getRPM());
            }
            break;
        case GAUGE_SPEED:
            if (speed_display_) {
                speed_display_->update(data.getSpeed());
            }
            break;
        case GAUGE_TEMP:
            if (temp_gauge_) {
                temp_gauge_->update(data.getCoolantTemp());
            }
            break;
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
