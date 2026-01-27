#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <functional>

namespace digidash {

/**
 * @brief Animation engine for gauge needle and dynamic elements
 * 
 * Supports:
 * - Easing functions (linear, ease-in, ease-out, etc.)
 * - Property animations (rotation, position, opacity)
 * - Keyframe sequences
 * - Concurrent animations
 */
class AnimationEngine {
public:
    enum class EasingType {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT,
        CUBIC_BEZIER
    };

    struct Animation {
        float start_value;
        float end_value;
        uint32_t duration_ms;
        EasingType easing;
        bool loop;
    };

    AnimationEngine();
    ~AnimationEngine();

    /**
     * @brief Start a new animation
     */
    uint32_t start_animation(const Animation& anim);

    /**
     * @brief Stop a running animation by ID
     */
    void stop_animation(uint32_t animation_id);

    /**
     * @brief Update all active animations (call every frame)
     */
    void update(uint32_t delta_ms);

    /**
     * @brief Get current value of an animation
     */
    float get_value(uint32_t animation_id);

    /**
     * @brief Register callback for animation completion
     */
    void set_completion_callback(uint32_t animation_id, 
                                  std::function<void()> callback);

private:
    struct AnimationState {
        Animation animation;
        uint32_t elapsed_ms;
        bool active;
        std::function<void()> completion_callback;
    };

    std::vector<AnimationState> animations_;
    uint32_t next_animation_id_;
};

} // namespace digidash
