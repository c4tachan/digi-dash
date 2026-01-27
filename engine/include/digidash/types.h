#pragma once

#include <cstdint>

namespace digidash {

enum class StrokeLineCap : uint8_t {
    Butt = 0,
    Round = 1,
    Square = 2
};

struct Color {
    uint8_t r, g, b, a;
};

} // namespace digidash
