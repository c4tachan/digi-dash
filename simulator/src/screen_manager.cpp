/**
 * @file screen_manager.cpp
 * @brief Implementation of screen manager
 */

#include "screen_manager.h"

lv_obj_t* ScreenManager::createScreen(const char* title) {
    lv_obj_t* screen = lv_obj_create(nullptr);
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_pad_all(screen, 15, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Add title label
    lv_obj_t* title_label = lv_label_create(screen);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_bottom(title_label, 20, 0);
    
    // Add navigation hint at bottom
    lv_obj_t* hint_label = lv_label_create(screen);
    lv_label_set_text(hint_label, "LEFT/RIGHT to navigate | ESC to exit");
    lv_obj_set_style_text_font(hint_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hint_label, lv_color_hex(0x888888), 0);
    lv_obj_add_flag(hint_label, LV_OBJ_FLAG_FLOATING);
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    return screen;
}

ScreenManager::ScreenManager() : current_screen_(SCREEN_RPM) {
    // Create RPM screen
    screens_[SCREEN_RPM] = createScreen("RPM Gauge");
    lv_obj_set_flex_flow(screens_[SCREEN_RPM], LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screens_[SCREEN_RPM], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    rpm_gauge_ = new RPMGauge(screens_[SCREEN_RPM]);
    lv_obj_set_flex_grow(rpm_gauge_->getContainer(), 1);
    
    // Create Speed screen
    screens_[SCREEN_SPEED] = createScreen("Speed Display");
    lv_obj_set_flex_flow(screens_[SCREEN_SPEED], LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screens_[SCREEN_SPEED], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    speed_display_ = new SpeedDisplay(screens_[SCREEN_SPEED]);
    lv_obj_set_flex_grow(speed_display_->getContainer(), 1);
    
    // Create Temperature screen
    screens_[SCREEN_TEMP] = createScreen("Coolant Temperature");
    lv_obj_set_flex_flow(screens_[SCREEN_TEMP], LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screens_[SCREEN_TEMP], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    temp_gauge_ = new TempGauge(screens_[SCREEN_TEMP]);
    lv_obj_set_flex_grow(temp_gauge_->getContainer(), 1);
    
    // Load the first screen
    lv_screen_load(screens_[SCREEN_RPM]);
}

ScreenManager::~ScreenManager() {
    delete rpm_gauge_;
    delete speed_display_;
    delete temp_gauge_;
    
    for (int i = 0; i < SCREEN_COUNT; i++) {
        if (screens_[i]) {
            lv_obj_del(screens_[i]);
        }
    }
}

void ScreenManager::update(const MockData& data) {
    rpm_gauge_->update(data.getRPM());
    speed_display_->update(data.getSpeed());
    temp_gauge_->update(data.getCoolantTemp());
}

void ScreenManager::nextScreen() {
    current_screen_ = (current_screen_ + 1) % SCREEN_COUNT;
    lv_screen_load(screens_[current_screen_]);
}

void ScreenManager::prevScreen() {
    current_screen_ = (current_screen_ - 1 + SCREEN_COUNT) % SCREEN_COUNT;
    lv_screen_load(screens_[current_screen_]);
}

void ScreenManager::goToScreen(ScreenType screen) {
    if (screen >= 0 && screen < SCREEN_COUNT) {
        current_screen_ = screen;
        lv_screen_load(screens_[current_screen_]);
    }
}
