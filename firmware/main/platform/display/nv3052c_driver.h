#pragma once

#include <cstdint>
#include "esp_lcd_panel_io.h"
#include "esp_err.h"

namespace digidash {

/**
 * NV3052C LCD Panel Driver
 * Provides initialization sequence for NV3052C controller on HD40015C 0-Y 720x720 display
 */

struct Nv3052cDriver {
    /**
     * Initialize NV3052C panel controller
     * Sends the exact initialization sequence for the NV3052C controller
     * 
     * @param io LCD panel I/O handle
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t init(esp_lcd_panel_io_handle_t io);
};

} // namespace digidash
