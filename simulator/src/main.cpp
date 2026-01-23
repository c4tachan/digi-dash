/**
 * @file main.cpp
 * @brief Main entry point for the LVGL simulator
 */

#include "lvgl.h"
#include "src/widgets/image/lv_image.h"

#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>

// Forward declarations for LVGL filesystem
extern "C" {
    void lv_fs_posix_init(void);
}
#include "lvgl.h"

/**
 * @brief Tick callback for LVGL
 */
static uint32_t tick_get_cb(void) {
    return SDL_GetTicks();
}

// SDL display flush callback
static void sdl_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    lv_display_flush_ready(disp);
}

/**
 * @brief Main function
 */

int main(int argc, char* argv[]) {
    printf("Starting Digi-Dash Simulator (SVG test)...\n");

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create SDL window
    SDL_Window* window = SDL_CreateWindow(
        "Digi-Dash Simulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        480, 320,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize LVGL
    lv_init();
    lv_tick_set_cb(tick_get_cb);

    // Initialize POSIX filesystem driver (for loading SVG from disk)
    lv_fs_posix_init();
    printf("LVGL initialized with POSIX filesystem support\n");

    // Create LVGL display with draw buffer
    static lv_display_t* display = lv_display_create(480, 320);
    if (!display) {
        fprintf(stderr, "Failed to create LVGL display\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Allocate draw buffer (required for rendering)
    static lv_color_t buf1[480 * 320 / 10];
    lv_display_set_buffers(display, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, sdl_display_flush);

    printf("Display buffer allocated: %zu bytes\n", sizeof(buf1));

    // Remove all widgets/screens
    lv_obj_clean(lv_screen_active());
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

    // Try to load SVG from assets directory
    // POSIX driver uses 'P:' prefix, path is relative to current working directory
    const char* svg_path = "P:../assets/dashboard.svg";
    printf("Attempting to load SVG from: %s\n", svg_path);
    
    lv_obj_t* img = lv_image_create(lv_screen_active());
    lv_image_set_src(img, svg_path);
    lv_obj_center(img);

    // Add status label
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Digi-Dash Simulator\nLVGL + ThorVG SVG\nPress ESC to exit");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    // Main event loop
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }
        lv_timer_handler();
        SDL_Delay(5);
    }

    printf("Exiting...\n");
    fflush(stdout);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
