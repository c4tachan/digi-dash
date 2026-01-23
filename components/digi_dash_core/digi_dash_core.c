#include "digi_dash_core.h"
#include <stdbool.h>
#include <math.h>

static bool initialized = false;

esp_err_t digi_dash_display_init(uint16_t width, uint16_t height)
{
    if (width == 0 || height == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    initialized = true;
    return ESP_OK;
}

esp_err_t digi_dash_update_data(float speed_kmh, uint16_t rpm, float temp_c)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (speed_kmh < 0 || rpm > 10000 || temp_c < -50 || temp_c > 200) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return ESP_OK;
}

bool digi_dash_is_initialized(void)
{
    return initialized;
}

float digi_dash_kmh_to_mph(float kmh)
{
    return kmh * 0.621371f;
}

float digi_dash_celsius_to_fahrenheit(float celsius)
{
    return (celsius * 9.0f / 5.0f) + 32.0f;
}

bool digi_dash_is_in_range(float value, float min, float max)
{
    return (value >= min && value <= max);
}
