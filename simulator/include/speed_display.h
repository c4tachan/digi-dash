/**
 * @file speed_display.h
 * @brief Speed display widget
 */

#ifndef SPEED_DISPLAY_H
#define SPEED_DISPLAY_H

#include "lvgl.h"
#include <cstdint>

class SpeedDisplay {
public:
    SpeedDisplay(lv_obj_t* parent);
    ~SpeedDisplay();
    
    /**
     * @brief Update the displayed speed value
     * @param speed Speed in km/h (0-300)
     */
    void update(int speed);
    
    /**
     * @brief Get the container widget
     */
    lv_obj_t* getContainer() const { return container_; }
    
private:
    lv_obj_t* container_;
    lv_obj_t* speed_label_;
    lv_obj_t* unit_label_;
    lv_obj_t* title_label_;
};

#endif // SPEED_DISPLAY_H
