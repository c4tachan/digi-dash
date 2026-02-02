#include "nv3052c_driver.h"
#include "esp_log.h"
#include <cstring>

static const char* TAG = "NV3052C";

namespace digidash {

// NV3052C initialization sequence as specified by manufacturer
static const struct {
    uint8_t cmd;
    uint8_t data[6];
    uint8_t data_len;
} nv3052c_init_sequence[] = {
    {0xFF, {0x30}, 1},                                          // Switch to page 0x30
    {0xFF, {0x52}, 1},                                          // Switch to page 0x52
    {0xFF, {0x01}, 1},                                          // Switch to page 0x01
    {0xE3, {0x00}, 1},                                          // E3 = 0x00
    {0xE1, {0x93}, 1},                                          // E1 = 0x93
    {0xAC, {0x00}, 1},                                          // AC = 0x00
    {0xA7, {0x47}, 1},                                          // A7 = 0x47
    {0xA0, {0xDD}, 1},                                          // A0 = 0xDD
    {0xA8, {0x74}, 1},                                          // A8 = 0x74
    {0xAD, {0x33}, 1},                                          // AD = 0x33
    {0xAE, {0x2B}, 1},                                          // AE = 0x2B
    {0xA9, {0x6C}, 1},                                          // A9 = 0x6C
    {0xAA, {0x2F}, 1},                                          // AA = 0x2F
    {0xAB, {0x00}, 1},                                          // AB = 0x00
    {0xFF, {0x30}, 1},                                          // Switch to page 0x30
    {0xFF, {0x52}, 1},                                          // Switch to page 0x52
    {0xFF, {0x02}, 1},                                          // Switch to page 0x02
    {0xB0, {0x0B, 0x16, 0x1D, 0x0D, 0x10, 0x05}, 6},          // Gamma curve (positive)
    {0xB1, {0x0B, 0x16, 0x1D, 0x0D, 0x10, 0x05}, 6},          // Gamma curve
    {0xB2, {0x0B, 0x16, 0x1D, 0x0D, 0x10, 0x05}, 6},          // Gamma curve
    {0xB3, {0x0B, 0x16, 0x1D, 0x0D, 0x10, 0x05}, 6},          // Gamma curve
    {0xFF, {0x30}, 1},                                          // Switch to page 0x30
    {0xFF, {0x52}, 1},                                          // Switch to page 0x52
    {0xFF, {0x00}, 1},                                          // Switch back to page 0x00
};

esp_err_t Nv3052cDriver::init(esp_lcd_panel_io_handle_t io) {
    if (!io) {
        ESP_LOGE(TAG, "Invalid panel I/O handle");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Starting NV3052C initialization sequence");
    
    // Send all initialization commands
    for (size_t i = 0; i < sizeof(nv3052c_init_sequence) / sizeof(nv3052c_init_sequence[0]); i++) {
        const auto& cmd_entry = nv3052c_init_sequence[i];
        
        ESP_LOGD(TAG, "Sending command 0x%02X with %d data bytes", cmd_entry.cmd, cmd_entry.data_len);
        
        esp_err_t ret = esp_lcd_panel_io_tx_param(io, cmd_entry.cmd, cmd_entry.data, cmd_entry.data_len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send init command 0x%02X: %s", cmd_entry.cmd, esp_err_to_name(ret));
            return ret;
        }
    }
    
    // Send Sleep Out command (0x11)
    ESP_LOGI(TAG, "Sending Sleep Out command (0x11)");
    esp_err_t ret = esp_lcd_panel_io_tx_param(io, 0x11, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send Sleep Out command: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Wait 120ms for Sleep Out to complete
    vTaskDelay(120 / portTICK_PERIOD_MS);
    
    // Send Display On command (0x29)
    ESP_LOGI(TAG, "Sending Display On command (0x29)");
    ret = esp_lcd_panel_io_tx_param(io, 0x29, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send Display On command: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "NV3052C initialization complete");
    return ESP_OK;
}

} // namespace digidash
