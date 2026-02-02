#include <catch2/catch_test_macros.hpp>

#include "platform/display/display_driver.h"
#include "subsystems/rendering/tile_height_renderer.h"
#include "esp_stubs.h"
#include "digidash/color_utils.h"

using namespace digidash;

TEST_CASE("TileHeightRenderer renders tiles via test callback", "[renderer]") {
    DisplayDriver display(4, 6);
    REQUIRE(display.initialize());

    esp_stub_clear_framebuffer();

    TileHeightRenderer renderer(display, 3); // tile height 3 -> two tiles for height 6
    REQUIRE(renderer.initialize());

    renderer.set_test_render_callback([](uint8_t* target, int width, int height, int stride, int y_offset) {
        // Fill the tile with a color depending on its y_offset
        uint8_t r = (y_offset == 0) ? 10 : 200;
        uint8_t g = (y_offset == 0) ? 20 : 150;
        uint8_t b = (y_offset == 0) ? 30 : 100;
        int pixels_per_row = stride / 4;
        for (int yy = 0; yy < height; ++yy) {
            for (int xx = 0; xx < width; ++xx) {
                int i = yy * pixels_per_row + xx;
                target[i*4 + 0] = r;
                target[i*4 + 1] = g;
                target[i*4 + 2] = b;
                target[i*4 + 3] = 255;
            }
        }
    });

    renderer.render_frame();

    const auto& fb = esp_stub_get_framebuffer();
    REQUIRE(fb.size() == 4 * 6);

    uint16_t expected_top = digidash::rgba_to_rgb565(10, 20, 30);
    uint16_t expected_bottom = digidash::rgba_to_rgb565(200, 150, 100);

    // top tile rows 0..2
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 4; ++x) {
            REQUIRE(fb[y * 4 + x] == expected_top);
        }
    }

    // bottom tile rows 3..5
    for (int y = 3; y < 6; ++y) {
        for (int x = 0; x < 4; ++x) {
            REQUIRE(fb[y * 4 + x] == expected_bottom);
        }
    }
}
