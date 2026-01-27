#pragma once

#include <cstdint>

namespace digidash {

/**
 * @brief Abstract display interface
 * 
 * Implemented by both SDL2 (simulator) and LVGL (ESP32)
 */
class PlatformDisplay {
public:
    virtual ~PlatformDisplay() = default;

    /**
     * @brief Get display width in pixels
     */
    virtual int get_width() const = 0;

    /**
     * @brief Get display height in pixels
     */
    virtual int get_height() const = 0;

    /**
     * @brief Get pixel format stride (bytes per row)
     */
    virtual int get_stride() const = 0;

    /**
     * @brief Lock the framebuffer for direct access
     */
    virtual uint8_t* lock_framebuffer() = 0;

    /**
     * @brief Unlock and update the display
     */
    virtual void unlock_and_update() = 0;

    /**
     * @brief Clear the display to a color
     */
    virtual void clear(uint32_t color) = 0;
};

} // namespace digidash
