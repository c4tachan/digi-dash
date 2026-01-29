#pragma once

#include <cstdint>
#include <memory>
#include "tile_renderer.h"

namespace digidash {

class DisplayDriver;

/**
 * @brief Main rendering engine adapter
 * 
 * Coordinates rendering operations using an abstract TileRenderer strategy.
 * Allows different rendering approaches without modifying this class.
 * (Dependency Inversion Principle & Strategy Pattern)
 */
class RenderEngine {
public:
    RenderEngine(DisplayDriver& display, uint32_t tile_height = 60);
    ~RenderEngine();

    bool initialize();
    bool load_gauge(const uint8_t* data, size_t size);
    void render_frame();
    
    uint32_t get_frame_count() const { return renderer_->get_frame_count(); }
    
private:
    DisplayDriver& display_;
    std::unique_ptr<TileRenderer> renderer_;
};

} // namespace digidash
