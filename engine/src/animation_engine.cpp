#include "digidash/animation_engine.h"

namespace digidash {

AnimationEngine::AnimationEngine() : next_animation_id_(1) {}

AnimationEngine::~AnimationEngine() {}

uint32_t AnimationEngine::start_animation(const Animation& anim) {
    AnimationState state;
    state.animation = anim;
    state.elapsed_ms = 0;
    state.active = true;
    
    uint32_t id = next_animation_id_++;
    animations_.push_back(state);
    return id;
}

void AnimationEngine::stop_animation(uint32_t animation_id) {
    for (auto& state : animations_) {
        if (state.animation.duration_ms == animation_id) {
            state.active = false;
            break;
        }
    }
}

void AnimationEngine::update(uint32_t delta_ms) {
    for (auto& state : animations_) {
        if (!state.active) continue;
        
        state.elapsed_ms += delta_ms;
        
        if (state.elapsed_ms >= state.animation.duration_ms) {
            if (state.animation.loop) {
                state.elapsed_ms = 0;
            } else {
                state.active = false;
                if (state.completion_callback) {
                    state.completion_callback();
                }
            }
        }
    }
}

float AnimationEngine::get_value(uint32_t animation_id) {
    // TODO: Implement easing functions
    for (const auto& state : animations_) {
        if (!state.active) continue;
        
        float t = static_cast<float>(state.elapsed_ms) /
                  state.animation.duration_ms;
        t = (t > 1.0f) ? 1.0f : t;
        
        return state.animation.start_value +
               (state.animation.end_value - state.animation.start_value) * t;
    }
    return 0.0f;
}

void AnimationEngine::set_completion_callback(
    uint32_t animation_id, std::function<void()> callback) {
    // TODO: Implement callback assignment by ID
}

} // namespace digidash
