#pragma once

#include <cstdint>

namespace digidash {

/**
 * @brief Abstract input interface
 * 
 * Handles buttons, touchscreen, rotary encoders, etc.
 */
class PlatformInput {
public:
    enum class InputType {
        BUTTON_PRESS,
        BUTTON_RELEASE,
        TOUCH_DOWN,
        TOUCH_UP,
        TOUCH_MOVE,
        ROTARY_ENCODER
    };

    struct InputEvent {
        InputType type;
        uint32_t button_id;
        int x;
        int y;
        int delta;  // For rotary encoders
    };

    virtual ~PlatformInput() = default;

    /**
     * @brief Poll for next input event
     * @return true if an event was available
     */
    virtual bool poll_event(InputEvent& event_out) = 0;

    /**
     * @brief Register a button
     */
    virtual void register_button(uint32_t button_id, int pin) = 0;
};

} // namespace digidash
