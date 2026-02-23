#pragma once

#include "vector_renderer.h"
#include "binary_gauge_loader.h"
#include "animation_engine.h"
#include "pid_binding_system.h"

#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

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
     * @param target_buffer The buffer to render into
     * @param width Width of the target buffer
     * @param height Height of the target buffer (tile height)
     * @param stride Byte stride of the target buffer
     * @param y_offset Y offset in the full gauge coordinate space (for tiled rendering)
     */
    void render(uint8_t* target_buffer, int width, int height, int stride, int y_offset = 0);

    /**
     * @brief Set PID data value
     */
    void set_pid_value(uint32_t pid_id, float value);

    /**
     * @brief Set target viewport used to fit the gauge onto the display
     */
    void set_viewport(uint32_t viewport_width, uint32_t viewport_height);

    /**
     * @brief Get current width of the gauge
     */
    uint32_t get_width() const { return width_; }

    /**
     * @brief Get current height of the gauge
     */
    uint32_t get_height() const { return height_; }

private:
    struct RuntimePathAnimation {
        size_t path_index;
        float min_value;
        float max_value;
        std::string pid_name;
        uint32_t pid_id;
        bool uses_pid;
        bool reverse;
    };

    std::unique_ptr<VectorRenderer> renderer_;
    std::unique_ptr<AnimationEngine> animation_engine_;
    std::unique_ptr<PIDBindingSystem> pid_system_;

    BinaryGaugeLoader::GaugeAsset current_asset_;
    std::vector<std::string> path_ids_;
    std::vector<RuntimePathAnimation> runtime_animations_;
    std::unordered_map<std::string, uint32_t> pid_name_to_id_;
    std::unordered_set<uint32_t> seen_pid_ids_;
    std::vector<VectorRenderer::BezierPath> paths_;
    std::vector<VectorRenderer::BezierPath> transformed_paths_;
    uint32_t animation_time_ms_;
    uint32_t width_;
    uint32_t height_;
    uint32_t viewport_width_;
    uint32_t viewport_height_;

    void rebuild_transformed_paths();
    const RuntimePathAnimation* find_runtime_animation(size_t path_index) const;
    float get_runtime_animation_value(const RuntimePathAnimation& animation) const;
    VectorRenderer::BezierPath trim_path_by_ratio(const VectorRenderer::BezierPath& path, float ratio, bool reverse = false) const;
};

} // namespace digidash
