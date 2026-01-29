#pragma once

#include "tile_renderer.h"
#include "digidash/gauge_scene.h"
#include <memory>
#include <cstdint>

namespace digidash {

class DisplayDriver;

/**
 * @brief Concrete tile-based renderer using fixed-height tiles
 * 
 * Renders the gauge in horizontal tiles of fixed height.
 * Memory efficient for large displays.
 * Implements the TileRenderer interface (Dependency Inversion Principle).
 */
class TileHeightRenderer : public TileRenderer {
public:
    TileHeightRenderer(DisplayDriver& display, uint32_t tile_height = 60);
    ~TileHeightRenderer() override;

    bool initialize() override;
    bool load_gauge(const uint8_t* data, size_t size) override;
    void render_frame() override;
    uint32_t get_frame_count() const override { return frame_count_; }

private:
    void convert_rgba_to_rgb565(const uint8_t* rgba_buffer, uint16_t* rgb565_buffer, size_t pixel_count);

    DisplayDriver& display_;
    uint32_t tile_height_;
    uint32_t num_tiles_;

    std::unique_ptr<GaugeScene> gauge_scene_;
    uint8_t* rgba_tile_buffer_;
    uint16_t* rgb565_tile_buffer_;

    uint32_t frame_count_;
    bool initialized_;
};

} // namespace digidash
