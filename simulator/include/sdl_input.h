#pragma once

#include "digidash/platform_input.h"
#include <SDL2/SDL.h>
#include <queue>

namespace digidash {

/**
 * @brief SDL2 implementation of PlatformInput
 */
class SDLInput : public PlatformInput {
public:
    SDLInput();
    ~SDLInput();

    bool poll_event(InputEvent& event_out) override;
    void register_button(uint32_t button_id, int pin) override;

    void process_sdl_events();

private:
    std::queue<InputEvent> event_queue_;
};

} // namespace digidash
