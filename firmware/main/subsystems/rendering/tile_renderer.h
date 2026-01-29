#pragma once

#include <cstdint>
#include <memory>

namespace digidash {

/**
 * @brief Abstract tile rendering strategy (Strategy Pattern)
 * 
 * Allows different rendering approaches (tile-based, strip-based, full-frame, etc.)
 * while keeping RenderEngine independent of the rendering strategy.
 * Enables Open/Closed Principle - extend with new renderers without modifying RenderEngine.
 */
class TileRenderer {
public:
    virtual ~TileRenderer() = default;

    /**
     * @brief Initialize the renderer
     */
    virtual bool initialize() = 0;

    /**
     * @brief Load a gauge asset
     */
    virtual bool load_gauge(const uint8_t* data, size_t size) = 0;

    /**
     * @brief Render a single frame
     */
    virtual void render_frame() = 0;

    /**
     * @brief Get frame count
     */
    virtual uint32_t get_frame_count() const = 0;
};

} // namespace digidash
