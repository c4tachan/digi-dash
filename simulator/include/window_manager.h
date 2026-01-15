/**
 * @file window_manager.h
 * @brief Manages multiple gauge windows
 */

#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "gauge_window.h"
#include "mock_data.h"
#include <vector>
#include <memory>

class WindowManager {
public:
    WindowManager();
    ~WindowManager();
    
    /**
     * @brief Create and add a new gauge window
     * @param gauge_type Type of gauge to display
     * @return True if window was created successfully
     */
    bool addGaugeWindow(GaugeWindow::GaugeType gauge_type = GaugeWindow::GAUGE_RPM);
    
    /**
     * @brief Remove a window by ID
     * @param window_id SDL window ID
     */
    void removeWindow(uint32_t window_id);
    
    /**
     * @brief Update all windows with new data
     * @param data Mock data source
     */
    void update(const MockData& data);
    
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
