#include "pca9554_expander.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "PCA9554";

namespace digidash {

Pca9554Expander::Pca9554Expander(i2c_master_bus_handle_t i2c_bus_handle, uint8_t device_address)
    : i2c_bus_handle_(i2c_bus_handle)
    , device_handle_(nullptr)
    , device_address_(device_address)
    , output_state_(0x00)
    , initialized_(false) {
}

Pca9554Expander::~Pca9554Expander() {
    if (device_handle_) {
        i2c_master_bus_rm_device(device_handle_);
        device_handle_ = nullptr;
    }
}

esp_err_t Pca9554Expander::initialize() {
    if (initialized_) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing PCA9554 at address 0x%02X", device_address_);
    
    // Add device to I2C bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = static_cast<uint16_t>(device_address_),
        .scl_speed_hz = 100000,  // 100kHz
    };
    
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle_, &dev_cfg, &device_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device to I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure all pins as outputs except IRQ (pin 3)
    uint8_t config = PIN_TFT_IRQ;  // 0x08 - IRQ is input, all others output
    ret = write_register(REG_CONFIGURATION, config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure pins: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Disable polarity inversion
    ret = write_register(REG_POLARITY, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set polarity: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set initial output state: CS, RESET, SCK, MOSI high
    output_state_ = PIN_TFT_CS | PIN_TFT_RESET | PIN_TFT_SCK | PIN_TFT_MOSI;
    ret = write_register(REG_OUTPUT, output_state_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set initial output: %s", esp_err_to_name(ret));
        return ret;
    }
    
    initialized_ = true;
    ESP_LOGI(TAG, "PCA9554 initialized successfully");
    return ESP_OK;
}

void Pca9554Expander::set_pins(uint8_t mask) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Not initialized");
        return;
    }
    
    output_state_ |= mask;
    flush_output();
}

void Pca9554Expander::clear_pins(uint8_t mask) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Not initialized");
        return;
    }
    
    output_state_ &= ~mask;
    flush_output();
}

void Pca9554Expander::write_output(uint8_t value) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Not initialized");
        return;
    }
    
    output_state_ = value;
    flush_output();
}

uint8_t Pca9554Expander::read_input() {
    if (!initialized_) {
        ESP_LOGE(TAG, "Not initialized");
        return 0xFF;
    }
    
    uint8_t value = 0xFF;
    esp_err_t ret = read_register(REG_INPUT, &value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read input: %s", esp_err_to_name(ret));
        return 0xFF;
    }
    
    return value;
}

esp_err_t Pca9554Expander::write_register(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    esp_err_t ret = i2c_master_transmit(device_handle_, data, sizeof(data), 100);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2C write failed: %s, retrying...", esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(10));
        ret = i2c_master_transmit(device_handle_, data, sizeof(data), 100);
    }
    
    return ret;
}

esp_err_t Pca9554Expander::read_register(uint8_t reg, uint8_t* value) {
    // Write register address, then read
    esp_err_t ret = i2c_master_transmit_receive(
        device_handle_,
        &reg, 1,
        value, 1,
        100
    );
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t Pca9554Expander::flush_output() {
    esp_err_t ret = write_register(REG_OUTPUT, output_state_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to flush output: %s", esp_err_to_name(ret));
    }
    return ret;
}

} // namespace digidash
