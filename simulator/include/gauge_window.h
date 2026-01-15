/**
 * @file gauge_window.h
 * @brief A standalone window for displaying a single gauge
 */

#ifndef GAUGE_WINDOW_H
#define GAUGE_WINDOW_H

#include "lvgl.h"
#include "rpm_gauge.h"
#include "speed_display.h"
#include "temp_gauge.h"
#include "data_source.h"
#include <SDL2/SDL.h>

class GaugeWindow {
public:
    enum GaugeType {
        GAUGE_RPM = 0,
        GAUGE_SPEED = 1,
        GAUGE_TEMP = 2
    };
    
    /**
     * @brief Create a new gauge window
     * @param gauge_type Type of gauge to display
     * @param x Window X position
     * @param y Window Y position
     * @param width Window width
     * @param height Window height
     * @param window_id Unique identifier for this window
     */
    GaugeWindow(GaugeType gauge_type, int x, int y, int width, int height, int window_id);
    ~GaugeWindow();
    
    /**
     * @brief Update gauge with new data
     * @param data Data source (mock or real OBD II)
     */
    void update(const DataSource& data);
    
    /**
     * @brief Render this window
     */
    void render();
    
    /**
     * @brief Check if window is still valid
     */
    bool isValid() const { return window_ != nullptr && renderer_ != nullptr; }
    
    /**
     * @brief Get SDL window ID
     */
    uint32_t getWindowID() const;
    
    /**
     * @brief Get window width
     */
    int getWidth() const { return width_; }
    
    /**
     * @brief Get window height
     */
    int getHeight() const { return height_; }
    
    /**
     * @brief Get current gauge type
     */
    GaugeType getCurrentGaugeType() const { return gauge_type_; }
    
    /**
     * @brief Handle SDL events for this window (mouse clicks, key presses, etc.)
     * @param event SDL event to process
     * @return true if event was handled by this window
     */
    bool handleEvent(const SDL_Event& event);
    
private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    lv_display_t* display_;
    
    void* buf1_;
    void* buf2_;
    
    // Gauge objects
    RPMGauge* rpm_gauge_;
    SpeedDisplay* speed_display_;
    TempGauge* temp_gauge_;
    
    lv_obj_t* screen_;
    
    int width_;
    int height_;
    int window_id_;
    int last_x_;
    int last_y_;
    bool is_pressed_;
    GaugeType gauge_type_;
    
    /**
     * @brief Display flush callback
     */
    static void displayFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);
};

#endif // GAUGE_WINDOW_H
