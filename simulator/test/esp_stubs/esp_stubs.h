#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

// Minimal stubs for ESP headers used by DisplayDriver in tests.
typedef void* esp_lcd_panel_handle_t;
typedef int esp_err_t;
#define ESP_OK 0

struct esp_lcd_rgb_qemu_config_t {
    uint32_t width;
    uint32_t height;
    int bpp;
};

#define RGB_QEMU_BPP_16 16

// Fake framebuffer storage
void esp_stub_set_panel_size(int width, int height);
const std::vector<uint16_t>& esp_stub_get_framebuffer();
void esp_stub_clear_framebuffer();

// Stub functions (implemented in esp_stubs.cpp)
esp_err_t esp_lcd_new_rgb_qemu(const esp_lcd_rgb_qemu_config_t* cfg, esp_lcd_panel_handle_t* out);
esp_err_t esp_lcd_panel_del(esp_lcd_panel_handle_t panel);
esp_err_t esp_lcd_panel_reset(esp_lcd_panel_handle_t panel);
esp_err_t esp_lcd_panel_init(esp_lcd_panel_handle_t panel);
esp_err_t esp_lcd_panel_draw_bitmap(esp_lcd_panel_handle_t panel, uint32_t x_start, uint32_t y_start, uint32_t x_end, uint32_t y_end, const void* color_data);
esp_err_t esp_lcd_rgb_qemu_refresh(esp_lcd_panel_handle_t panel);

const char* esp_err_to_name(esp_err_t);

// Simple reg stubs used by DisplayDriver
void esp_stub_reg_write(uint32_t val);
uint32_t esp_stub_reg_read();

// REG macros used in firmware. We ignore the address parameter in test stubs.
#define REG_WRITE(addr, val) esp_stub_reg_write(val)
#define REG_READ(addr) esp_stub_reg_read()

// SYSCON_DATE_REG placeholder
#define SYSCON_DATE_REG 0x1000

// Minimal logging macros (no-op)
#define ESP_LOGI(tag, fmt, ...)
#define ESP_LOGE(tag, fmt, ...)
#define ESP_LOGW(tag, fmt, ...)
