#pragma once

#include <cstdint>
#include "driver/i2c_master.h"
#include "esp_err.h"

namespace digidash {

/**
 * @brief PCA9554 I2C GPIO Expander Driver
 * 
 * Single Responsibility: Manages I2C communication with PCA9554 GPIO expander
 * Interface Segregation: Provides only GPIO expander functionality
 * Dependency Inversion: Depends on abstraction (I2C handles) not concrete implementation
 */
class Pca9554Expander {
public:
    // Pin bit masks for PCA9554
    static constexpr uint8_t PIN_TFT_SCK       = (1 << 0);
    static constexpr uint8_t PIN_TFT_CS        = (1 << 1);
    static constexpr uint8_t PIN_TFT_RESET     = (1 << 2);
    static constexpr uint8_t PIN_TFT_IRQ       = (1 << 3);  // Input
    static constexpr uint8_t PIN_TFT_BACKLIGHT = (1 << 4);
    static constexpr uint8_t PIN_TFT_MOSI      = (1 << 7);
    
    /**
     * @brief Construct PCA9554 expander
     * @param i2c_bus_handle I2C bus handle (owned by caller)
     * @param device_address I2C device address (default 0x3F)
     */
    explicit Pca9554Expander(i2c_master_bus_handle_t i2c_bus_handle, uint8_t device_address = 0x3F);
    
    /**
     * @brief Destructor - cleanup I2C device handle
     */
    ~Pca9554Expander();
    
    // Prevent copying (owns I2C device handle)
    Pca9554Expander(const Pca9554Expander&) = delete;
    Pca9554Expander& operator=(const Pca9554Expander&) = delete;
    
    /**
     * @brief Initialize the PCA9554 expander
     * Configures all pins as outputs except IRQ, sets initial states
     * @return ESP_OK on success
     */
    esp_err_t initialize();
    
    /**
     * @brief Set one or more output pins high
     * @param mask Bit mask of pins to set
     */
    void set_pins(uint8_t mask);
    
    /**
     * @brief Clear one or more output pins low
     * @param mask Bit mask of pins to clear
     */
    void clear_pins(uint8_t mask);
    
    /**
     * @brief Write entire output register
     * @param value Value to write to output register
     */
    void write_output(uint8_t value);
    
    /**
     * @brief Read input register
     * @return Input register value, or 0xFF on error
     */
    uint8_t read_input();
    
    /**
     * @brief Get current output state (shadow register)
     * @return Current output state
     */
    uint8_t get_output_state() const { return output_state_; }

private:
    // PCA9554 register addresses
    static constexpr uint8_t REG_INPUT          = 0x00;
    static constexpr uint8_t REG_OUTPUT         = 0x01;
    static constexpr uint8_t REG_POLARITY       = 0x02;
    static constexpr uint8_t REG_CONFIGURATION  = 0x03;
    
    i2c_master_bus_handle_t i2c_bus_handle_;  // Not owned - managed by caller
    i2c_master_dev_handle_t device_handle_;   // Owned - cleaned up in destructor
    uint8_t device_address_;
    uint8_t output_state_;  // Shadow register for output state
    bool initialized_;
    
    /**
     * @brief Write to a PCA9554 register
     * @param reg Register address
     * @param value Value to write
     * @return ESP_OK on success
     */
    esp_err_t write_register(uint8_t reg, uint8_t value);
    
    /**
     * @brief Read from a PCA9554 register
     * @param reg Register address
     * @param value Pointer to store read value
     * @return ESP_OK on success
     */
    esp_err_t read_register(uint8_t reg, uint8_t* value);
    
    /**
     * @brief Flush output_state_ to hardware
     * @return ESP_OK on success
     */
    esp_err_t flush_output();
};

} // namespace digidash
