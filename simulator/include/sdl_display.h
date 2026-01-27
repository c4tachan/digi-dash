#pragma once

#include "digidash/platform_display.h"
#include <SDL2/SDL.h>
#include <memory>

namespace digidash {

/**
 * @brief SDL2 implementation of PlatformDisplay
 */
class SDLDisplay : public PlatformDisplay {
public:
    SDLDisplay(int width, int height);
    ~SDLDisplay();

    bool init();

    int get_width() const override { return width_; }
    int get_height() const override { return height_; }
    int get_stride() const override { return stride_; }

    uint8_t* lock_framebuffer() override;
    void unlock_and_update() override;
    void clear(uint32_t color) override;

    bool is_running() const { return running_; }
    void request_quit() { running_ = false; }

private:
    int width_;
    int height_;
    int stride_;
    bool running_;

    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    uint8_t* framebuffer_;
};

} // namespace digidash
