#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the dashboard display system
 * 
 * @param width Display width in pixels
 * @param height Display height in pixels
 * @return esp_err_t ESP_OK on success
 */
esp_err_t digi_dash_display_init(uint16_t width, uint16_t height);

/**
 * @brief Update dashboard data
 * 
 * @param speed_kmh Current speed in km/h
 * @param rpm Current engine RPM
 * @param temp_c Engine temperature in Celsius
 * @return esp_err_t ESP_OK on success
 */
esp_err_t digi_dash_update_data(float speed_kmh, uint16_t rpm, float temp_c);

/**
 * @brief Get current dashboard status
 * 
 * @return true if dashboard is initialized and running
 */
bool digi_dash_is_initialized(void);

/**
 * @brief Convert speed from km/h to mph
 * 
 * @param kmh Speed in kilometers per hour
 * @return float Speed in miles per hour
 */
float digi_dash_kmh_to_mph(float kmh);

/**
 * @brief Convert temperature from Celsius to Fahrenheit
 * 
 * @param celsius Temperature in Celsius
 * @return float Temperature in Fahrenheit
 */
float digi_dash_celsius_to_fahrenheit(float celsius);

/**
 * @brief Check if value is within safe range
 * 
 * @param value Value to check
 * @param min Minimum safe value
 * @param max Maximum safe value
 * @return true if value is within range
 */
bool digi_dash_is_in_range(float value, float min, float max);

#ifdef __cplusplus
}
#endif
