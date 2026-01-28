#include "display_driver.h"
#include "esp_log.h"
#include "esp_lcd_qemu_rgb.h"
#include "soc/syscon_reg.h"
#include "soc/soc.h"

static const char* TAG = "DisplayDriver";

#define RGB_QEMU_ORIGIN 0x51454d55

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

    // Initialize QEMU RGB panel
    REG_WRITE(SYSCON_DATE_REG - 4, RGB_QEMU_ORIGIN);
    uint32_t qemu_origin = REG_READ(SYSCON_DATE_REG - 4);
    ESP_LOGI(TAG, "QEMU origin register: 0x%08lx", (unsigned long)qemu_origin);
    
    ESP_LOGI(TAG, "Installing RGB LCD panel driver (%lux%lu)", 
             (unsigned long)width_, (unsigned long)height_);
    
    esp_lcd_rgb_qemu_config_t panel_config = {
        .width = width_,
        .height = height_,
        .bpp = RGB_QEMU_BPP_16,
    };
    
    esp_err_t ret = esp_lcd_new_rgb_qemu(&panel_config, &panel_handle_);
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
    if (!initialized_) {
        ESP_LOGE(TAG, "Display not initialized");
        return;
    }
    
    esp_lcd_rgb_qemu_refresh(panel_handle_);
}

} // namespace digidash
