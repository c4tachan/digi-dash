#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <cstdint>
#include "digidash/color_utils.h"

using namespace digidash;

TEST_CASE("RGBA to RGB565 known colors", "[color_utils]") {
    uint16_t out = 0;

    uint8_t rgba_red[4] = { 0xFF, 0x00, 0x00, 0xFF };
    convert_rgba_buffer_to_rgb565(rgba_red, &out, 1);
    REQUIRE(out == 0xF800);

    uint8_t rgba_green[4] = { 0x00, 0xFF, 0x00, 0xFF };
    convert_rgba_buffer_to_rgb565(rgba_green, &out, 1);
    REQUIRE(out == 0x07E0);

    uint8_t rgba_blue[4] = { 0x00, 0x00, 0xFF, 0xFF };
    convert_rgba_buffer_to_rgb565(rgba_blue, &out, 1);
    REQUIRE(out == 0x001F);
}

TEST_CASE("RGBA to RGB565 mixed sample", "[color_utils]") {
    uint16_t out = 0;
    uint8_t rgba[4] = { 0x12, 0x34, 0x56, 0xFF };
    convert_rgba_buffer_to_rgb565(rgba, &out, 1);
    INFO("Converted -> 0x" << std::hex << out);
    REQUIRE(out == 0x11AA);
}
