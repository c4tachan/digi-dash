/**
 * @file temp_gauge.h
 * @brief Temperature gauge widget
 */

#ifndef TEMP_GAUGE_H
#define TEMP_GAUGE_H

#include "lvgl.h"
#include <cstdint>

class TempGauge {
public:
    TempGauge(lv_obj_t* parent);
    ~TempGauge();
    
    /**
     * @brief Update the displayed temperature value
     * @param temp Temperature in Celsius (0-150)
     */
    void update(uint8_t temp);
    
    /**
     * @brief Get the container widget
     */
    lv_obj_t* getContainer() const { return container_; }
    
private:
    lv_obj_t* container_;
    lv_obj_t* bar_;
    lv_obj_t* label_;
    lv_obj_t* title_label_;
};

#endif // TEMP_GAUGE_H
