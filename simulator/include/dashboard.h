/**
 * @file dashboard.h
 * @brief Single display dashboard with multiple gauge elements
 */

#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "lvgl.h"
#include "gauge.h"
#include "config.h"
#include "data_source.h"
#include <SDL2/SDL.h>
#include <vector>
#include <memory>
#include <map>

class Dashboard {
public:
    /**
     * @brief Create a new dashboard from display configuration
     * @param display_config Display configuration with elements
     * @param gauge_definitions Map of all available gauge definitions
     */
    Dashboard(const DisplayConfig& display_config,
              const std::map<std::string, GaugeDefinition>& gauge_definitions);
    ~Dashboard();
    
    /**
     * @brief Update all dashboard elements with new data
     * @param data Data source (mock or real OBD II)
     * @param pid_map Map of PID names to configurations
     */
    void update(const DataSource& data, const std::map<std::string, PidConfig>& pid_map);
    
    /**
     * @brief Render this dashboard
     */
    void render();
    
    /**
     * @brief Check if dashboard is still valid
     */
    bool isValid() const { return sdl_window_ != nullptr && renderer_ != nullptr; }
    
    /**
     * @brief Get display width
     */
    int getWidth() const { return width_; }
    
    /**
     * @brief Get display height
     */
    int getHeight() const { return height_; }
    
    /**
     * @brief Handle SDL events for this dashboard
     * @param event SDL event to process
     * @return true if event was handled by this dashboard
     */
    bool handleEvent(const SDL_Event& event);

private:
    SDL_Window* sdl_window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    lv_display_t* display_;
    
    void* buf1_;
    void* buf2_;
    
    // Gauge elements in this display
    std::vector<std::unique_ptr<Gauge>> elements_;
    
    lv_obj_t* screen_;
    
    int width_;
    int height_;
    int last_x_;
    int last_y_;
    bool is_pressed_;
    
    static void displayFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);
};

#endif // DASHBOARD_H
