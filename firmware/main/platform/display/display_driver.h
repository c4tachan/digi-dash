#pragma once

#include <cstdint>
#include "esp_lcd_panel_ops.h"

namespace digidash {

class DisplayDriver {
public:
    DisplayDriver(uint32_t width, uint32_t height);
    ~DisplayDriver();

    bool initialize();
    void draw_bitmap(uint32_t x_start, uint32_t y_start, uint32_t x_end, uint32_t y_end, const void* color_data);
    void refresh();
    
    uint32_t get_width() const { return width_; }
    uint32_t get_height() const { return height_; }
    
private:
    uint32_t width_;
    uint32_t height_;
    esp_lcd_panel_handle_t panel_handle_;
    bool initialized_;
};

} // namespace digidash
