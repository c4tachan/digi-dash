#pragma once

#include "pca9554_expander.h"
#include "esp_err.h"
#include <memory>

namespace digidash {

/**
 * @brief NV3052C TFT Controller Initialization via PCA9554 Bit-banged SPI
 * 
 * Single Responsibility: Initialize NV3052C display controller
 * Dependency Inversion: Depends on Pca9554Expander abstraction
 * Open/Closed: Initialization sequence can be extended without modifying class
 */
class Nv3052cTftInit {
public:
    /**
     * @brief Construct TFT initializer
     * @param pca_expander Reference to PCA9554 expander (must outlive this object)
     */
    explicit Nv3052cTftInit(Pca9554Expander& pca_expander);
    
    /**
     * @brief Initialize the NV3052C TFT controller
     * Performs hardware reset and sends initialization sequence
     * @return ESP_OK on success
     */
    esp_err_t initialize();
    
    /**
     * @brief Enable display backlight via PCA9554
     */
    void enable_backlight();
    
    /**
     * @brief Disable display backlight via PCA9554
     */
    void disable_backlight();

private:
    Pca9554Expander& pca_expander_;
    bool initialized_;
    
    /**
     * @brief Hardware reset sequence for NV3052C
     */
    void hardware_reset();
    
    /**
     * @brief Write a single bit via bit-banged SPI
     * @param bit Bit value (0 or 1)
     */
    void write_bit(uint8_t bit);
    
    /**
     * @brief Write 9 bits: D/C bit + 8 data bits
     * @param dc_bit Data/Command bit (0=command, 1=data)
     * @param data 8-bit data value
     */
    void write_9bit(uint8_t dc_bit, uint8_t data);
    
    /**
     * @brief Write command with data parameter
     * @param cmd Command byte
     * @param data Data byte
     */
    void write_command(uint8_t cmd, uint8_t data);
    
    /**
     * @brief Send complete NV3052C initialization sequence
     */
    void send_init_sequence();
};

} // namespace digidash
