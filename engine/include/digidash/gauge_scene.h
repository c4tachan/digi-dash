#pragma once

#include "vector_renderer.h"
#include "binary_gauge_loader.h"
#include "animation_engine.h"
#include "pid_binding_system.h"

#include <memory>
#include <cstdint>

namespace digidash {

/**
 * @brief Main gauge scene manager
 * 
 * Orchestrates rendering, animations, and data binding for a single gauge.
 * Coordinates between the engine components.
 */
class GaugeScene {
public:
    GaugeScene();
    ~GaugeScene();

    /**
     * @brief Load a gauge asset and initialize the scene
     */
    bool load_gauge(const BinaryGaugeLoader::GaugeAsset& asset);

    /**
     * @brief Update scene state (animations, data bindings)
     */
    void update(uint32_t delta_ms);

    /**
     * @brief Render the scene to a target buffer
     */
    void render(uint8_t* target_buffer, int width, int height, int stride);

    /**
     * @brief Set PID data value
     */
    void set_pid_value(uint32_t pid_id, float value);

    /**
     * @brief Get current width of the gauge
     */
    uint32_t get_width() const { return width_; }

    /**
     * @brief Get current height of the gauge
     */
    uint32_t get_height() const { return height_; }

private:
    std::unique_ptr<VectorRenderer> renderer_;
    std::unique_ptr<AnimationEngine> animation_engine_;
    std::unique_ptr<PIDBindingSystem> pid_system_;

    BinaryGaugeLoader::GaugeAsset current_asset_;
    std::vector<VectorRenderer::BezierPath> paths_;
    uint32_t width_;
    uint32_t height_;
};

} // namespace digidash
