/**
 * @file dashboard_screen.cpp
 * @brief Implementation of main dashboard screen
 */

#include "dashboard_screen.h"

DashboardScreen::DashboardScreen() {
    // Create the screen
    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_pad_all(screen_, 10, 0);
    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create a horizontal flex container for layout
    lv_obj_set_flex_flow(screen_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(screen_, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create RPM gauge (left)
    rpm_gauge_ = new RPMGauge(screen_);
    lv_obj_set_flex_grow(rpm_gauge_->getContainer(), 0);
    
    // Create a vertical container for right side
    lv_obj_t* right_container = lv_obj_create(screen_);
    lv_obj_set_size(right_container, LV_PCT(40), LV_PCT(100));
    lv_obj_set_flex_flow(right_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(right_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(right_container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(right_container, 0, 0);
    
    // Create speed display (top right)
    speed_display_ = new SpeedDisplay(right_container);
    lv_obj_set_flex_grow(speed_display_->getContainer(), 0);
    
    // Create temperature gauge (bottom right)
    temp_gauge_ = new TempGauge(right_container);
    lv_obj_set_flex_grow(temp_gauge_->getContainer(), 0);
}

DashboardScreen::~DashboardScreen() {
    delete rpm_gauge_;
    delete speed_display_;
    delete temp_gauge_;
    
    if (screen_) {
        lv_obj_del(screen_);
    }
}

void DashboardScreen::update(const MockData& data) {
    rpm_gauge_->update(data.getRPM());
    speed_display_->update(data.getSpeed());
    temp_gauge_->update(data.getCoolantTemp());
}
