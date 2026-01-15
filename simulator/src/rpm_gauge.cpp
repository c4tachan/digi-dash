/**
 * @file rpm_gauge.cpp
 * @brief Implementation of RPM gauge widget
 */

#include "rpm_gauge.h"
#include <cstdio>

RPMGauge::RPMGauge(lv_obj_t* parent) {
    // Create container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 280, 280);
    lv_obj_set_style_bg_color(container_, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 10, 0);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create arc gauge
    arc_ = lv_arc_create(container_);
    lv_obj_set_size(arc_, 240, 240);
    lv_obj_center(arc_);
    lv_arc_set_rotation(arc_, 135);
    lv_arc_set_bg_angles(arc_, 0, 270);
    lv_arc_set_range(arc_, 0, 8000);
    lv_obj_remove_style(arc_, nullptr, LV_PART_KNOB);
    lv_obj_set_style_arc_width(arc_, 15, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_, 15, LV_PART_INDICATOR);
    
    // Set colors
    lv_obj_set_style_arc_color(arc_, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_, lv_color_hex(0xFF6B00), LV_PART_INDICATOR);
    
    // Create center label for RPM value
    label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(label_, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(label_, "0");
    lv_obj_center(label_);
    
    // Create title label
    title_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_label_, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(title_label_, "RPM");
    lv_obj_align(title_label_, LV_ALIGN_BOTTOM_MID, 0, -5);
}

RPMGauge::~RPMGauge() {
    if (container_) {
        lv_obj_del(container_);
    }
}

void RPMGauge::update(int rpm) {
    // Clamp RPM to 8000
    if (rpm > 8000) rpm = 8000;
    if (rpm < 0) rpm = 0;
    
    // Update arc value
    lv_arc_set_value(arc_, rpm);
    
    // Update text label
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", rpm);
    lv_label_set_text(label_, buf);
    
    // Mark for redraw
    lv_obj_invalidate(container_);
}
