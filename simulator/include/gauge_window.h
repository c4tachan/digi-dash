/**
 * @file gauge_window.h
 * @brief A standalone window for displaying gauges
 */

#ifndef GAUGE_WINDOW_H
#define GAUGE_WINDOW_H

#include "lvgl.h"
#include "gauge.h"
#include "config.h"
#include "data_source.h"
#include <SDL2/SDL.h>
#include <vector>
#include <memory>
#include <map>

class GaugeWindow {
public:
    /**
     * @brief Create a new gauge window from window configuration
     * @param window_config Window configuration with gauge instances
     * @param gauge_definitions Map of all available gauge definitions
     * @param window_id Unique identifier for this window
     */
    GaugeWindow(const WindowConfig& window_config, 
                const std::map<std::string, GaugeDefinition>& gauge_definitions,
                int window_id);
    ~GaugeWindow();
    
    /**
     * @brief Update all gauges with new data
     * @param data Data source (mock or real OBD II)
     * @param pid_map Map of PID names to values
     */
    void update(const DataSource& data, const std::map<std::string, PidConfig>& pid_map);
    
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
    
    // Gauges in this window
    std::vector<std::unique_ptr<Gauge>> gauges_;
    
    lv_obj_t* screen_;
    
    int width_;
    int height_;
    int window_id_;
    int last_x_;
    int last_y_;
    bool is_pressed_;
    
    static void displayFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);
};

#endif // GAUGE_WINDOW_H
