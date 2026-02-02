#pragma once

#include <cstdint>
#include <cstddef>

namespace digidash {

inline uint16_t rgba_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

inline void convert_rgba_buffer_to_rgb565(const uint8_t* rgba_buffer, uint16_t* rgb565_buffer, size_t pixel_count) {
    for (size_t i = 0; i < pixel_count; ++i) {
        uint8_t r = rgba_buffer[i * 4 + 0];
        uint8_t g = rgba_buffer[i * 4 + 1];
        uint8_t b = rgba_buffer[i * 4 + 2];
        rgb565_buffer[i] = rgba_to_rgb565(r, g, b);
    }
}

} // namespace digidash
