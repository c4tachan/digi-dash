/**
 * @file window_manager.h
 * @brief Manages multiple gauge windows
 */

#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "gauge_window.h"
#include "config.h"
#include "data_source.h"
#include <SDL2/SDL.h>
#include <vector>
#include <memory>

class WindowManager {
public:
    WindowManager();
    ~WindowManager();
    
    /**
     * @brief Initialize windows from configuration
     * @param config Dashboard configuration
     * @return True if all windows created successfully
     */
    bool initializeFromConfig(const DashboardConfig& config);
    
    /**
     * @brief Remove a window by ID
     * @param window_id SDL window ID
     */
    void removeWindow(uint32_t window_id);
    
    /**
     * @brief Update all windows with new data
     * @param data Data source (mock or real OBD II)
     * @param pid_map Map of PID names to configurations
     */
    void update(const DataSource& data, const std::map<std::string, PidConfig>& pid_map);
    
    /**
     * @brief Handle SDL event for all windows
     * @param event SDL event to process
     */
    void handleEvent(const SDL_Event& event);
    
    /**
     * @brief Render all windows
     */
    void renderAll();
    
    /**
     * @brief Get number of windows
     */
    size_t getWindowCount() const { return windows_.size(); }
    
    /**
     * @brief Find window by SDL window ID
     * @return Pointer to window or nullptr if not found
     */
    GaugeWindow* findWindow(uint32_t window_id);
    
    /**
     * @brief Check if any windows are still open
     */
    bool hasWindows() const { return !windows_.empty(); }
    
    /**
     * @brief Get max number of windows allowed
     */
    static constexpr size_t MAX_WINDOWS = 16;
    
private:
    std::vector<std::unique_ptr<GaugeWindow>> windows_;
    int next_window_id_;
};

#endif // WINDOW_MANAGER_H
