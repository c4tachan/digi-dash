#include "sdl_input.h"

namespace digidash {

SDLInput::SDLInput() {}

SDLInput::~SDLInput() {}

bool SDLInput::poll_event(InputEvent& event_out) {
    if (event_queue_.empty()) {
        return false;
    }
    event_out = event_queue_.front();
    event_queue_.pop();
    return true;
}

void SDLInput::register_button(uint32_t button_id, int pin) {
    // TODO: Map button_id to SDL events
}

void SDLInput::process_sdl_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                // Signal quit via a special event
                {
                    InputEvent quit_event;
                    quit_event.type = InputType::BUTTON_PRESS;
                    quit_event.button_id = SDLK_ESCAPE;  // Special code for quit
                    event_queue_.push(quit_event);
                }
                break;
            case SDL_KEYDOWN:
                {
                    InputEvent input_event;
                    input_event.type = InputType::BUTTON_PRESS;
                    input_event.button_id = event.key.keysym.sym;
                    event_queue_.push(input_event);
                }
                break;
            case SDL_KEYUP:
                {
                    InputEvent input_event;
                    input_event.type = InputType::BUTTON_RELEASE;
                    input_event.button_id = event.key.keysym.sym;
                    event_queue_.push(input_event);
                }
                break;
            default:
                break;
        }
    }
}

} // namespace digidash
