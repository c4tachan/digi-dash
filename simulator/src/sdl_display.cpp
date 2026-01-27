#include "sdl_display.h"
#include <cstring>

namespace digidash {

SDLDisplay::SDLDisplay(int width, int height)
    : width_(width), height_(height), running_(true),
      window_(nullptr), renderer_(nullptr), texture_(nullptr),
      framebuffer_(nullptr) {
    stride_ = width * 4; // RGBA32
}

SDLDisplay::~SDLDisplay() {
    if (framebuffer_) {
        delete[] framebuffer_;
    }
    if (texture_) {
        SDL_DestroyTexture(texture_);
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

bool SDLDisplay::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }

    window_ = SDL_CreateWindow("Digi-Dash Simulator", SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, width_, height_,
                               SDL_WINDOW_SHOWN);
    if (!window_) {
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        return false;
    }

    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, width_, height_);
    if (!texture_) {
        return false;
    }

    framebuffer_ = new uint8_t[stride_ * height_];
    std::memset(framebuffer_, 0, stride_ * height_);

    return true;
}

uint8_t* SDLDisplay::lock_framebuffer() { return framebuffer_; }

void SDLDisplay::unlock_and_update() {
    SDL_UpdateTexture(texture_, nullptr, framebuffer_, stride_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

void SDLDisplay::clear(uint32_t color) {
    uint32_t* pixels = reinterpret_cast<uint32_t*>(framebuffer_);
    int num_pixels = (stride_ / 4) * height_;
    for (int i = 0; i < num_pixels; ++i) {
        pixels[i] = color;
    }
}

} // namespace digidash
