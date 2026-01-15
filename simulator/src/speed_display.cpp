/**
 * @file speed_display.cpp
 * @brief Implementation of speed display widget
 */

#include "speed_display.h"
#include <cstdio>

SpeedDisplay::SpeedDisplay(lv_obj_t* parent) {
    // Create container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 200, 120);
    lv_obj_set_style_bg_color(container_, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 10, 0);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create title label
    title_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_label_, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(title_label_, "SPEED");
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 5);
    
    // Create speed value label
    speed_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(speed_label_, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(speed_label_, lv_color_hex(0x00D9FF), 0);
    lv_label_set_text(speed_label_, "0");
    lv_obj_align(speed_label_, LV_ALIGN_CENTER, 0, -5);
    
    // Create unit label
    unit_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(unit_label_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(unit_label_, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(unit_label_, "km/h");
    lv_obj_align(unit_label_, LV_ALIGN_BOTTOM_MID, 0, 5);
}

SpeedDisplay::~SpeedDisplay() {
    if (container_) {
        lv_obj_del(container_);
    }
}

void SpeedDisplay::update(int speed) {
    // Clamp speed to 300
    if (speed > 300) speed = 300;
    if (speed < 0) speed = 0;
    
    // Update text label
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", speed);
    lv_label_set_text(speed_label_, buf);
    
    // Mark for redraw
    lv_obj_invalidate(container_);
}
