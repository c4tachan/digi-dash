#pragma once

#include <cstdint>
#include <memory>
#include "esp_lcd_panel_ops.h"
#include "driver/i2c_master.h"
#include "digidash/platform_display.h"
#include "pca9554_expander.h"
#include "nv3052c_tft_init.h"

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
    uint8_t* lock_framebuffer() override { return framebuffer_; } // Return manual framebuffer
    void unlock_and_update() override; // Trigger DMA transfer in double-FB mode
    void clear(uint32_t color) override;
    
    // ESP-IDF specific methods
    void draw_bitmap(uint32_t x_start, uint32_t y_start, uint32_t x_end, uint32_t y_end, const void* color_data);
    void refresh();
    
    // Test pattern methods (verify display is working before rendering gauge)
    void test_pattern_solid_red();
    void test_pattern_solid_green();
    void test_pattern_solid_blue();
    
private:
    uint32_t width_;
    uint32_t height_;
    esp_lcd_panel_handle_t panel_handle_;
    i2c_master_bus_handle_t i2c_bus_handle_;  // I2C bus for PCA9554
    bool initialized_;
    
    // Hardware abstraction components (Dependency Inversion Principle)
    std::unique_ptr<Pca9554Expander> pca_expander_;
    std::unique_ptr<Nv3052cTftInit> tft_init_;
    
    // Framebuffer pointers (managed by RGB panel driver in double-FB mode)
    void* framebuffer0_;
    void* framebuffer1_;
    uint8_t* framebuffer_;  // Points to current active framebuffer
    
    // Helper functions
    bool init_i2c_bus();
    bool init_pca9554();
    bool init_tft_controller();
    bool init_rgb_panel();
    void enable_backlight();
};

} // namespace digidash
