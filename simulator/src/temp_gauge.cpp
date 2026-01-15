/**
 * @file temp_gauge.cpp
 * @brief Implementation of temperature gauge widget
 */

#include "temp_gauge.h"
#include <cstdio>

TempGauge::TempGauge(lv_obj_t* parent) {
    // Create container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 200, 120);
    lv_obj_set_style_bg_color(container_, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 10, 0);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create title label
    title_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(title_label_, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(title_label_, "COOLANT");
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 2);
    
    // Create bar gauge
    bar_ = lv_bar_create(container_);
    lv_obj_set_size(bar_, 30, 80);
    lv_obj_align(bar_, LV_ALIGN_CENTER, -30, 0);
    lv_bar_set_range(bar_, 0, 150);
    lv_bar_set_value(bar_, 85, LV_ANIM_OFF);
    
    // Set colors
    lv_obj_set_style_bg_color(bar_, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_, lv_color_hex(0x00FF88), LV_PART_INDICATOR);
    
    // Create temperature label
    label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(label_, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label_, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(label_, "85°C");
    lv_obj_align(label_, LV_ALIGN_CENTER, 35, 0);
}

TempGauge::~TempGauge() {
    if (container_) {
        lv_obj_del(container_);
    }
}

void TempGauge::update(uint8_t temp) {
    // Clamp temperature to 150
    if (temp > 150) temp = 150;
    
    // Update bar value
    lv_bar_set_value(bar_, temp, LV_ANIM_OFF);
    
    // Update color based on temperature
    if (temp < 90) {
        lv_obj_set_style_bg_color(bar_, lv_color_hex(0x00FF88), LV_PART_INDICATOR); // Green
    } else if (temp < 105) {
        lv_obj_set_style_bg_color(bar_, lv_color_hex(0xFFAA00), LV_PART_INDICATOR); // Orange
    } else {
        lv_obj_set_style_bg_color(bar_, lv_color_hex(0xFF0000), LV_PART_INDICATOR); // Red
    }
    
    // Update text label
    char buf[16];
    snprintf(buf, sizeof(buf), "%u°C", temp);
    lv_label_set_text(label_, buf);
    
    // Mark for redraw
    lv_obj_invalidate(container_);
}
