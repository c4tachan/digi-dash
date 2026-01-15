/**
 * @file screen_manager.h
 * @brief Manages multiple gauge screens with navigation
 */

#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "lvgl.h"
#include "rpm_gauge.h"
#include "speed_display.h"
#include "temp_gauge.h"
#include "mock_data.h"

class ScreenManager {
public:
    enum ScreenType {
        SCREEN_RPM = 0,
        SCREEN_SPEED = 1,
        SCREEN_TEMP = 2,
        SCREEN_COUNT = 3
    };
    
    ScreenManager();
    ~ScreenManager();
    
    /**
     * @brief Update all gauges with new data
     * @param data Mock data source
     */
    void update(const MockData& data);
    
    /**
     * @brief Navigate to the next screen
     */
    void nextScreen();
    
    /**
     * @brief Navigate to the previous screen
     */
    void prevScreen();
    
    /**
     * @brief Jump to a specific screen
     * @param screen Screen index
     */
    void goToScreen(ScreenType screen);
    
    /**
     * @brief Get the current screen
     * @return LVGL screen object
     */
    lv_obj_t* getCurrentScreen() const { return screens_[current_screen_]; }
    
    /**
     * @brief Get current screen index
     */
    ScreenType getCurrentScreenType() const { return static_cast<ScreenType>(current_screen_); }
    
private:
    lv_obj_t* screens_[SCREEN_COUNT];
    int current_screen_;
    
    RPMGauge* rpm_gauge_;
    SpeedDisplay* speed_display_;
    TempGauge* temp_gauge_;
    
    /**
     * @brief Create a screen with title and navigation hints
     * @param title Screen title
     * @return LVGL screen object
     */
    lv_obj_t* createScreen(const char* title);
};

#endif // SCREEN_MANAGER_H
