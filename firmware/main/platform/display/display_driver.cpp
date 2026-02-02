#include "display_driver.h"
#include "qualia_display_config.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include <cstdlib>
#include <cstring>

static const char* TAG = "DisplayDriver";

namespace digidash {

DisplayDriver::DisplayDriver(uint32_t width, uint32_t height)
    : width_(width)
    , height_(height)
    , panel_handle_(nullptr)
    , initialized_(false) {
}

DisplayDriver::~DisplayDriver() {
    if (panel_handle_) {
        esp_lcd_panel_del(panel_handle_);
    }
}

bool DisplayDriver::initialize() {
    if (initialized_) {
        ESP_LOGW(TAG, "Display already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Installing RGB LCD panel driver (%lux%lu)", 
             (unsigned long)width_, (unsigned long)height_);
    
    esp_lcd_rgb_panel_config_t panel_config = {};
    panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
    panel_config.timings.pclk_hz = QUALIA_LCD_PIXEL_CLOCK;
    panel_config.timings.h_res = QUALIA_LCD_H_RES;
    panel_config.timings.v_res = QUALIA_LCD_V_RES;
    panel_config.timings.hsync_pulse_width = QUALIA_LCD_HSYNC_PULSE_WIDTH;
    panel_config.timings.hsync_back_porch = QUALIA_LCD_HSYNC_BACK_PORCH;
    panel_config.timings.hsync_front_porch = QUALIA_LCD_HSYNC_FRONT_PORCH;
    panel_config.timings.vsync_pulse_width = QUALIA_LCD_VSYNC_PULSE_WIDTH;
    panel_config.timings.vsync_back_porch = QUALIA_LCD_VSYNC_BACK_PORCH;
    panel_config.timings.vsync_front_porch = QUALIA_LCD_VSYNC_FRONT_PORCH;
    panel_config.timings.flags.hsync_idle_low = QUALIA_LCD_HSYNC_POLARITY;
    panel_config.timings.flags.vsync_idle_low = QUALIA_LCD_VSYNC_POLARITY;
    panel_config.timings.flags.de_idle_high = !QUALIA_LCD_DE_POLARITY;
    panel_config.timings.flags.pclk_active_neg = QUALIA_LCD_PCLK_ACTIVE_NEG;
    
    panel_config.data_width = 16;  // RGB565
    panel_config.bits_per_pixel = 16;
    panel_config.num_fbs = 1;  // Single frame buffer in PSRAM
    panel_config.bounce_buffer_size_px = 0;
    
    panel_config.hsync_gpio_num = QUALIA_PIN_NUM_HSYNC;
    panel_config.vsync_gpio_num = QUALIA_PIN_NUM_VSYNC;
    panel_config.de_gpio_num = QUALIA_PIN_NUM_DE;
    panel_config.pclk_gpio_num = QUALIA_PIN_NUM_PCLK;
    
    panel_config.data_gpio_nums[0] = QUALIA_PIN_NUM_DATA0;
    panel_config.data_gpio_nums[1] = QUALIA_PIN_NUM_DATA1;
    panel_config.data_gpio_nums[2] = QUALIA_PIN_NUM_DATA2;
    panel_config.data_gpio_nums[3] = QUALIA_PIN_NUM_DATA3;
    panel_config.data_gpio_nums[4] = QUALIA_PIN_NUM_DATA4;
    panel_config.data_gpio_nums[5] = QUALIA_PIN_NUM_DATA5;
    panel_config.data_gpio_nums[6] = QUALIA_PIN_NUM_DATA6;
    panel_config.data_gpio_nums[7] = QUALIA_PIN_NUM_DATA7;
    panel_config.data_gpio_nums[8] = QUALIA_PIN_NUM_DATA8;
    panel_config.data_gpio_nums[9] = QUALIA_PIN_NUM_DATA9;
    panel_config.data_gpio_nums[10] = QUALIA_PIN_NUM_DATA10;
    panel_config.data_gpio_nums[11] = QUALIA_PIN_NUM_DATA11;
    panel_config.data_gpio_nums[12] = QUALIA_PIN_NUM_DATA12;
    panel_config.data_gpio_nums[13] = QUALIA_PIN_NUM_DATA13;
    panel_config.data_gpio_nums[14] = QUALIA_PIN_NUM_DATA14;
    panel_config.data_gpio_nums[15] = QUALIA_PIN_NUM_DATA15;
    
    panel_config.flags.fb_in_psram = 1;  // Frame buffer in PSRAM
    
    esp_err_t ret = esp_lcd_new_rgb_panel(&panel_config, &panel_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "Initializing RGB LCD panel");
    ret = esp_lcd_panel_reset(panel_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel reset failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = esp_lcd_panel_init(panel_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel init failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    initialized_ = true;
    ESP_LOGI(TAG, "Display initialized successfully");
    return true;
}

void DisplayDriver::draw_bitmap(uint32_t x_start, uint32_t y_start, uint32_t x_end, uint32_t y_end, const void* color_data) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Display not initialized");
        return;
    }
    
    esp_lcd_panel_draw_bitmap(panel_handle_, x_start, y_start, x_end, y_end, color_data);
}

void DisplayDriver::refresh() {
    // RGB LCD panel updates automatically via DMA
    // No explicit refresh needed for real hardware
}

void DisplayDriver::clear(uint32_t color) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Display not initialized");
        return;
    }

    // Convert 0xRRGGBB color to RGB565
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

    size_t row_bytes = width_ * sizeof(uint16_t);
    uint16_t* row = (uint16_t*)malloc(row_bytes);
    if (!row) {
        ESP_LOGE(TAG, "Failed to allocate row buffer for clear");
        return;
    }

    for (uint32_t i = 0; i < width_; ++i) {
        row[i] = rgb565;
    }

    for (uint32_t y = 0; y < height_; ++y) {
        esp_lcd_panel_draw_bitmap(panel_handle_, 0, y, width_, y + 1, row);
    }

    free(row);
}

} // namespace digidash
