/**
 * @file dashboard_screen.h
 * @brief Main dashboard screen with all gauges
 */

#ifndef DASHBOARD_SCREEN_H
#define DASHBOARD_SCREEN_H

#include "lvgl.h"
#include "rpm_gauge.h"
#include "speed_display.h"
#include "temp_gauge.h"
#include "mock_data.h"

class DashboardScreen {
public:
    DashboardScreen();
    ~DashboardScreen();
    
    /**
     * @brief Update the dashboard with new data
     * @param data Mock data source
     */
    void update(const MockData& data);
    
    /**
     * @brief Get the screen object
     * @return LVGL screen object
     */
    lv_obj_t* getScreen() const { return screen_; }
    
private:
    lv_obj_t* screen_;
    RPMGauge* rpm_gauge_;
    SpeedDisplay* speed_display_;
    TempGauge* temp_gauge_;
};

#endif // DASHBOARD_SCREEN_H
