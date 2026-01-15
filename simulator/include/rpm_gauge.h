/**
 * @file rpm_gauge.h
 * @brief RPM gauge widget
 */

#ifndef RPM_GAUGE_H
#define RPM_GAUGE_H

#include "lvgl.h"
#include <cstdint>

class RPMGauge {
public:
    RPMGauge(lv_obj_t* parent);
    ~RPMGauge();
    
    /**
     * @brief Update the displayed RPM value
     * @param rpm RPM value (0-8000)
     */
    void update(int rpm);
    
    /**
     * @brief Get the container widget
     */
    lv_obj_t* getContainer() const { return container_; }
    
private:
    lv_obj_t* container_;
    lv_obj_t* arc_;
    lv_obj_t* label_;
    lv_obj_t* title_label_;
};

#endif // RPM_GAUGE_H

