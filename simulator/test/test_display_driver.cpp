#include <catch2/catch_test_macros.hpp>

#include "esp_stubs/esp_stubs.h"
#include <digidash/display_driver.h>

using namespace digidash;

TEST_CASE("DisplayDriver clear fills framebuffer with RGB565") {
    // small panel
    const int W = 4;
    const int H = 3;
    DisplayDriver drv(W, H);

    REQUIRE(drv.initialize() == true);

    // clear with color 0x112233 -> RGB565
    drv.clear(0x112233);

    const auto& fb = esp_stub_get_framebuffer();
    REQUIRE((int)fb.size() == W * H);

    // compute expected rgb565
    uint8_t r = (0x112233 >> 16) & 0xFF;
    uint8_t g = (0x112233 >> 8) & 0xFF;
    uint8_t b = (0x112233) & 0xFF;
    uint16_t expected = static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));

    for (size_t i = 0; i < fb.size(); ++i) {
        REQUIRE(fb[i] == expected);
    }
}

TEST_CASE("DisplayDriver draw_bitmap writes given area") {
    const int W = 5;
    const int H = 4;
    DisplayDriver drv(W, H);
    REQUIRE(drv.initialize() == true);
    esp_stub_clear_framebuffer();

    // Prepare a 2x2 block with distinct colors
    uint16_t block[4];
    block[0] = 0xF800; block[1] = 0x07E0; block[2] = 0x001F; block[3] = 0xFFFF;

    drv.draw_bitmap(1, 1, 3, 3, block); // writes 2x2 at (1,1)

    const auto& fb = esp_stub_get_framebuffer();
    // Check positions
    REQUIRE(fb[1 * W + 1] == block[0]);
    REQUIRE(fb[1 * W + 2] == block[1]);
    REQUIRE(fb[2 * W + 1] == block[2]);
    REQUIRE(fb[2 * W + 2] == block[3]);
}

TEST_CASE("DisplayDriver draw_bitmap ignores call when not initialized") {
    DisplayDriver drv(10, 10);
    // Don't initialize; stub framebuffer should not change
    esp_stub_clear_framebuffer();
    uint16_t dummy = 0xFFFF;
    drv.draw_bitmap(0, 0, 1, 1, &dummy);
    const auto& fb = esp_stub_get_framebuffer();
    REQUIRE(fb[0] == 0);
}

TEST_CASE("DisplayDriver refresh ignores call when not initialized") {
    DisplayDriver drv(10, 10);
    // Calling refresh without initialize should be safe (no crash)
    drv.refresh();
    REQUIRE(true); // If we got here, no crash occurred
}

TEST_CASE("DisplayDriver clear ignores call when not initialized") {
    DisplayDriver drv(10, 10);
    // Calling clear without initialize should be safe
    drv.clear(0xFF0000);
    REQUIRE(true); // If we got here, no crash occurred
}

TEST_CASE("DisplayDriver initialize returns true if already initialized") {
    DisplayDriver drv(10, 10);
    REQUIRE(drv.initialize() == true);
    REQUIRE(drv.initialize() == true); // Second call should also succeed
}

TEST_CASE("DisplayDriver getters work after initialization") {
    DisplayDriver drv(64, 32);
    REQUIRE(drv.initialize() == true);
    REQUIRE(drv.get_width() == 64);
    REQUIRE(drv.get_height() == 32);
    REQUIRE(drv.get_stride() == 64 * 2); // RGB565 is 2 bytes per pixel
}
