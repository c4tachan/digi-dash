#pragma once

#include <cstdint>
#include "esp_lcd_panel_ops.h"
#include "digidash/platform_display.h"

namespace digidash {

class DisplayDriver : public PlatformDisplay {
public:
    DisplayDriver(uint32_t width, uint32_t height);
    ~DisplayDriver() override;

    bool initialize();
    
    // PlatformDisplay interface implementation (Liskov Substitution Principle)
    int get_width() const override { return width_; }
    int get_height() const override { return height_; }
    int get_stride() const override { return width_ * 2; } // RGB565: 2 bytes per pixel
    uint8_t* lock_framebuffer() override { return nullptr; } // QEMU RGB uses draw_bitmap
    void unlock_and_update() override {} // QEMU RGB uses draw_bitmap
    void clear(uint32_t color) override;
    
    // ESP-IDF specific methods
    void draw_bitmap(uint32_t x_start, uint32_t y_start, uint32_t x_end, uint32_t y_end, const void* color_data);
    void refresh();
    
private:
    uint32_t width_;
    uint32_t height_;
    esp_lcd_panel_handle_t panel_handle_;
    bool initialized_;
};

} // namespace digidash
