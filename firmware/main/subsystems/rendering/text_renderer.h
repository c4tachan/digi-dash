#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace digidash {

/**
 * @brief Simple monospace bitmap font text renderer
 * 
 * Renders ASCII text using an 8x8 pixel monospace font.
 */
class TextRenderer {
public:
    /**
     * @brief Draw text at position (x, y) to a framebuffer
     * @param text The text string to render (ASCII only)
     * @param x Starting X coordinate (in pixels)
     * @param y Starting Y coordinate (in pixels)
     * @param target_buffer Target RGBA framebuffer (8 bits per channel)
     * @param buffer_width Width of the target buffer
     * @param buffer_height Height of the target buffer
     * @param stride Byte stride of the target buffer (bytes per row)
     * @param color_r Red component (0-255)
     * @param color_g Green component (0-255)
     * @param color_b Blue component (0-255)
     * @param color_a Alpha component (0-255)
     */
    static void draw_text(const std::string& text, int x, int y,
                         uint8_t* target_buffer,
                         int buffer_width, int buffer_height, int stride,
                         uint8_t color_r, uint8_t color_g, 
                         uint8_t color_b, uint8_t color_a = 255);

private:
    // 8x8 pixel monospace font for ASCII characters 32-126
    static constexpr int CHAR_WIDTH = 8;
    static constexpr int CHAR_HEIGHT = 8;
    
    // Font data: each character is 8 bytes (8 rows of 8 pixels, 1 bit per pixel)
    static const uint8_t font_data_[95][8];
    
    static void draw_char(char c, int x, int y,
                         uint8_t* target_buffer,
                         int buffer_width, int buffer_height, int stride,
                         uint8_t color_r, uint8_t color_g,
                         uint8_t color_b, uint8_t color_a);
};

} // namespace digidash

