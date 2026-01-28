#pragma once

#include <cstdint>
#include <memory>
#include "digidash/gauge_scene.h"
#include "platform/display/display_driver.h"

namespace digidash {

class RenderEngine {
public:
    RenderEngine(DisplayDriver& display, uint32_t tile_height = 60);
    ~RenderEngine();

    bool initialize();
    bool load_gauge(const uint8_t* data, size_t size);
    void render_frame();
    
    uint32_t get_frame_count() const { return frame_count_; }
    
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
