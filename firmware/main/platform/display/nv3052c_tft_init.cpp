#include "nv3052c_tft_init.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "NV3052C_TFT";

namespace digidash {

Nv3052cTftInit::Nv3052cTftInit(Pca9554Expander& pca_expander)
    : pca_expander_(pca_expander)
    , initialized_(false) {
}

void Nv3052cTftInit::hardware_reset() {
    ESP_LOGI(TAG, "Performing hardware reset");
    
    // Assert reset (active LOW)
    pca_expander_.clear_pins(Pca9554Expander::PIN_TFT_RESET);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Release reset
    pca_expander_.set_pins(Pca9554Expander::PIN_TFT_RESET);
    vTaskDelay(pdMS_TO_TICKS(120));
}

void Nv3052cTftInit::write_bit(uint8_t bit) {
    // Set data on MOSI
    if (bit) {
        pca_expander_.set_pins(Pca9554Expander::PIN_TFT_MOSI);
    } else {
        pca_expander_.clear_pins(Pca9554Expander::PIN_TFT_MOSI);
    }
    
    // Clock LOW then HIGH (LCD samples on rising edge)
    pca_expander_.clear_pins(Pca9554Expander::PIN_TFT_SCK);
    pca_expander_.set_pins(Pca9554Expander::PIN_TFT_SCK);
}

void Nv3052cTftInit::write_9bit(uint8_t dc_bit, uint8_t data) {
    // First bit: D/C (0=command, 1=data)
    write_bit(dc_bit);
    
    // Next 8 bits: actual data (MSB first)
    for (int i = 7; i >= 0; i--) {
        write_bit((data >> i) & 1);
    }
}

void Nv3052cTftInit::write_command(uint8_t cmd, uint8_t data) {
    pca_expander_.clear_pins(Pca9554Expander::PIN_TFT_CS);
    write_9bit(0, cmd);   // D/C=0 for command
    write_9bit(1, data);  // D/C=1 for data
    pca_expander_.set_pins(Pca9554Expander::PIN_TFT_CS);
}

void Nv3052cTftInit::send_init_sequence() {
    ESP_LOGI(TAG, "Sending NV3052C initialization sequence");
    
    // NV3052C initialization sequence from working example
    write_command(0xFF, 0x30); write_command(0xFF, 0x52); write_command(0xFF, 0x01);
    write_command(0xE3, 0x00); write_command(0x0A, 0x11); write_command(0x23, 0xA0);
    write_command(0x24, 0x32); write_command(0x25, 0x12); write_command(0x26, 0x2E);
    write_command(0x27, 0x2E); write_command(0x29, 0x02); write_command(0x2A, 0xCF);
    write_command(0x32, 0x34); write_command(0x38, 0x9C); write_command(0x39, 0xA7);
    write_command(0x3A, 0x27); write_command(0x3B, 0x94); write_command(0x42, 0x6D);
    write_command(0x43, 0x83); write_command(0x81, 0x00); write_command(0x91, 0x67);
    write_command(0x92, 0x67); write_command(0xA0, 0x52); write_command(0xA1, 0x50);
    write_command(0xA4, 0x9C); write_command(0xA7, 0x02); write_command(0xA8, 0x02);
    write_command(0xA9, 0x02); write_command(0xAA, 0xA8); write_command(0xAB, 0x28);
    write_command(0xAE, 0xD2); write_command(0xAF, 0x02); write_command(0xB0, 0xD2);
    write_command(0xB2, 0x26); write_command(0xB3, 0x26); write_command(0xFF, 0x30);
    write_command(0xFF, 0x52); write_command(0xFF, 0x02); write_command(0xB1, 0x0A);
    write_command(0xD1, 0x0E); write_command(0xB4, 0x2F); write_command(0xD4, 0x2D);
    write_command(0xB2, 0x0C); write_command(0xD2, 0x0C); write_command(0xB3, 0x30);
    write_command(0xD3, 0x2A); write_command(0xB6, 0x1E); write_command(0xD6, 0x16);
    write_command(0xB7, 0x3B); write_command(0xD7, 0x35); write_command(0xC1, 0x08);
    write_command(0xE1, 0x08); write_command(0xB8, 0x0D); write_command(0xD8, 0x0D);
    write_command(0xB9, 0x05); write_command(0xD9, 0x05); write_command(0xBD, 0x15);
    write_command(0xDD, 0x15); write_command(0xBC, 0x13); write_command(0xDC, 0x13);
    write_command(0xBB, 0x12); write_command(0xDB, 0x10); write_command(0xBA, 0x11);
    write_command(0xDA, 0x11); write_command(0xBE, 0x17); write_command(0xDE, 0x17);
    write_command(0xBF, 0x0F); write_command(0xDF, 0x0F); write_command(0xC0, 0x16);
    write_command(0xE0, 0x16); write_command(0xB5, 0x2E); write_command(0xD5, 0x3F);
    write_command(0xB0, 0x03); write_command(0xD0, 0x02); write_command(0xFF, 0x30);
    write_command(0xFF, 0x52); write_command(0xFF, 0x03); write_command(0x08, 0x09);
    write_command(0x09, 0x0A); write_command(0x0A, 0x0B); write_command(0x0B, 0x0C);
    write_command(0x28, 0x22); write_command(0x2A, 0xE9); write_command(0x2B, 0xE9);
    write_command(0x34, 0x51); write_command(0x35, 0x01); write_command(0x36, 0x26);
    write_command(0x37, 0x13); write_command(0x40, 0x07); write_command(0x41, 0x08);
    write_command(0x42, 0x09); write_command(0x43, 0x0A); write_command(0x44, 0x22);
    write_command(0x45, 0xDB); write_command(0x46, 0xdC); write_command(0x47, 0x22);
    write_command(0x48, 0xDD); write_command(0x49, 0xDE); write_command(0x50, 0x0B);
    write_command(0x51, 0x0C); write_command(0x52, 0x0D); write_command(0x53, 0x0E);
    write_command(0x54, 0x22); write_command(0x55, 0xDF); write_command(0x56, 0xE0);
    write_command(0x57, 0x22); write_command(0x58, 0xE1); write_command(0x59, 0xE2);
    write_command(0x80, 0x1E); write_command(0x81, 0x1E); write_command(0x82, 0x1F);
    write_command(0x83, 0x1F); write_command(0x84, 0x05); write_command(0x85, 0x0A);
    write_command(0x86, 0x0A); write_command(0x87, 0x0C); write_command(0x88, 0x0C);
    write_command(0x89, 0x0E); write_command(0x8A, 0x0E); write_command(0x8B, 0x10);
    write_command(0x8C, 0x10); write_command(0x8D, 0x00); write_command(0x8E, 0x00);
    write_command(0x8F, 0x1F); write_command(0x90, 0x1F); write_command(0x91, 0x1E);
    write_command(0x92, 0x1E); write_command(0x93, 0x02); write_command(0x94, 0x04);
    write_command(0x96, 0x1E); write_command(0x97, 0x1E); write_command(0x98, 0x1F);
    write_command(0x99, 0x1F); write_command(0x9A, 0x05); write_command(0x9B, 0x09);
    write_command(0x9C, 0x09); write_command(0x9D, 0x0B); write_command(0x9E, 0x0B);
    write_command(0x9F, 0x0D); write_command(0xA0, 0x0D); write_command(0xA1, 0x0F);
    write_command(0xA2, 0x0F); write_command(0xA3, 0x00); write_command(0xA4, 0x00);
    write_command(0xA5, 0x1F); write_command(0xA6, 0x1F); write_command(0xA7, 0x1E);
    write_command(0xA8, 0x1E); write_command(0xA9, 0x01); write_command(0xAA, 0x03);
    write_command(0xFF, 0x30); write_command(0xFF, 0x52); write_command(0xFF, 0x00);
    
    // Pixel format: RGB565 (0x55)
    write_command(0x3A, 0x55);
    
    // Memory access control (orientation)
    write_command(0x36, 0x0A);
    
    // Sleep Out - command only (send command without data byte)
    pca_expander_.clear_pins(Pca9554Expander::PIN_TFT_CS);
    write_9bit(0, 0x11);  // D/C=0 for command
    pca_expander_.set_pins(Pca9554Expander::PIN_TFT_CS);
    
    vTaskDelay(pdMS_TO_TICKS(120));
    
    // Display On - command only
    pca_expander_.clear_pins(Pca9554Expander::PIN_TFT_CS);
    write_9bit(0, 0x29);  // D/C=0 for command
    pca_expander_.set_pins(Pca9554Expander::PIN_TFT_CS);
    
    vTaskDelay(pdMS_TO_TICKS(50));
    
    ESP_LOGI(TAG, "NV3052C initialization sequence complete");
}

esp_err_t Nv3052cTftInit::initialize() {
    if (initialized_) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing NV3052C TFT controller");
    
    // Reset the display controller
    hardware_reset();
    
    // Send initialization sequence
    send_init_sequence();
    
    initialized_ = true;
    ESP_LOGI(TAG, "NV3052C TFT controller initialized successfully");
    return ESP_OK;
}

void Nv3052cTftInit::enable_backlight() {
    ESP_LOGI(TAG, "Enabling backlight");
    pca_expander_.set_pins(Pca9554Expander::PIN_TFT_BACKLIGHT);
}

void Nv3052cTftInit::disable_backlight() {
    ESP_LOGI(TAG, "Disabling backlight");
    pca_expander_.clear_pins(Pca9554Expander::PIN_TFT_BACKLIGHT);
}

} // namespace digidash
