/**
 * @file main.cpp
 * @brief Main entry point for the LVGL simulator - PNG/SVG decoding debug
 */

#include "lvgl.h"
#include "src/widgets/image/lv_image.h"

#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

// ThorVG probe
bool tvg_probe_render(const char* path, int width, int height, const char* ppm_out);

// Forward declarations for LVGL filesystem and SVG support
extern "C" {
    void lv_fs_posix_init(void);
    void lv_svg_decoder_init(void);  // SVG decoder initialization
}
#include "lvgl.h"

#define TAG "digi-dash-sim"

/**
 * @brief Tick callback for LVGL
 */
static uint32_t tick_get_cb(void) {
    return SDL_GetTicks();
}

// Global SDL state for display flushing
static SDL_Window* g_window = NULL;
static SDL_Renderer* g_renderer = NULL;
static SDL_Texture* g_texture = NULL;

// SDL display flush callback with actual rendering
static int flush_count = 0;
static void sdl_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    flush_count++;

    if (!g_renderer || !g_texture || !px_map) {
        lv_display_flush_ready(disp);
        return;
    }

    const int w = area->x2 - area->x1 + 1;
    const int h = area->y2 - area->y1 + 1;
    SDL_Rect rect{area->x1, area->y1, w, h};

    // LV_COLOR_DEPTH is 32, so 4 bytes per pixel (ARGB8888)
    SDL_UpdateTexture(g_texture, &rect, px_map, w * 4);
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, nullptr, nullptr);
    SDL_RenderPresent(g_renderer);

    if (flush_count % 60 == 0) {
        printf("[digi-dash-sim] Display flush #%d area (%d,%d)-(%d,%d)\n",
               flush_count, area->x1, area->y1, area->x2, area->y2);
    }

    lv_display_flush_ready(disp);
}

/**
 * @brief Main function
 */

int main(int argc, char* argv[]) {
    printf("[%s] Starting Digi-Dash Simulator (PNG/SVG decoding test)...\n", TAG);
    fflush(stdout);

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[%s] SDL_Init Error: %s\n", TAG, SDL_GetError());
        return 1;
    }

    // Create SDL window
    SDL_Window* window = SDL_CreateWindow(
        "Digi-Dash Simulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        720, 720,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "[%s] SDL_CreateWindow Error: %s\n", TAG, SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "[%s] SDL_CreateRenderer Error: %s\n", TAG, SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING, 720, 720);
    if (!texture) {
        fprintf(stderr, "[%s] SDL_CreateTexture Error: %s\n", TAG, SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Store globally for display flush callback
    g_window = window;
    g_renderer = renderer;
    g_texture = texture;

    // Initialize LVGL
    printf("[%s] Initializing LVGL...\n", TAG);
    lv_init();
    lv_tick_set_cb(tick_get_cb);

    // Initialize SVG decoder (ThorVG)
    printf("[%s] Initializing SVG decoder (ThorVG)...\n", TAG);
    lv_svg_decoder_init();

    // Initialize POSIX filesystem driver (for loading PNG/SVG from disk)
    printf("[%s] Initializing POSIX filesystem driver...\n", TAG);
    lv_fs_posix_init();
    printf("[%s] LVGL initialized with POSIX filesystem support\n", TAG);

    // Create LVGL display with draw buffer (720x720 to match ESP)
    printf("[%s] Creating 720x720 LVGL display...\n", TAG);
    static lv_display_t* display = lv_display_create(720, 720);
    if (!display) {
        fprintf(stderr, "[%s] Failed to create LVGL display\n", TAG);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Allocate draw buffer (partial mode with generous height to satisfy ThorVG)
    // 720px width * 120 lines = 86,400 pixels (~337 KB @ 32 bpp)
    static lv_color_t buf_lines[720 * 120];
    lv_display_set_buffers(display, buf_lines, NULL, sizeof(buf_lines), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, sdl_display_flush);

    printf("[%s] Display buffer allocated: %zu bytes (partial mode)\n", TAG, sizeof(buf_lines));

    // Remove all widgets/screens
    lv_obj_clean(lv_screen_active());
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

    // TEST 1: Simple rectangle to confirm display is working
    printf("\n[%s] ========== DISPLAY TEST ==========\n", TAG);
    printf("[%s] Testing basic display with colored rectangle...\n", TAG);
    lv_obj_t* test_rect = lv_obj_create(lv_screen_active());
    lv_obj_set_size(test_rect, 100, 100);
    lv_obj_set_pos(test_rect, 50, 50);
    lv_obj_set_style_bg_color(test_rect, lv_color_hex(0xFF0000), 0);  // Red rectangle
    printf("[%s] Red rectangle created at (50,50) size 100x100\n", TAG);
    printf("[%s] Flush count before SVG test: %d\n", TAG, flush_count);

    // Test image loading with POSIX FS
    printf("\n[%s] ========== SVG DECODING TEST ==========\n", TAG);
    printf("[%s] Current working directory: %s\n", TAG, getcwd(NULL, 0));
    printf("[%s] Using ThorVG for SVG support\n\n", TAG);
    
    // Check if SVG file exists
    struct stat st;
    const char* img_path_disk = "../../assets/dashboard.svg";
    if (stat(img_path_disk, &st) == 0) {
        printf("[%s] ✓ File exists on disk: %s (%ld bytes)\n", TAG, img_path_disk, st.st_size);
        
        // Read first 200 bytes to check SVG content
        FILE* f = fopen(img_path_disk, "rb");
        if (f) {
            unsigned char header[200];
            size_t read = fread(header, 1, sizeof(header), f);
            fclose(f);
            printf("[%s] SVG Header (first 200 bytes):\n", TAG);
            printf("[%s] ", TAG);
            for (size_t i = 0; i < read && i < 200; i++) {
                if (header[i] >= 32 && header[i] <= 126) {
                    printf("%c", header[i]);
                } else if (header[i] == '\n') {
                    printf("\n[%s] ", TAG);
                } else if (header[i] == '\t') {
                    printf("  ");
                } else {
                    printf(".");
                }
            }
            printf("\n");
            
            // Check for fill color definition
            if (strstr((const char*)header, "fill=") || strstr((const char*)header, "style")) {
                printf("[%s] ✓ SVG appears to have fill/style definitions\n", TAG);
            } else {
                printf("[%s] ⚠ SVG might be missing fill colors - could render invisible!\n", TAG);
            }
        }
    } else {
        printf("[%s] ✗ File not found: %s\n", TAG, img_path_disk);
    }

    // Run ThorVG probe (direct SVG render) to confirm strokes/text
    printf("[%s] Running ThorVG probe render...\n", TAG);
    const char* probe_ppm = "./tvg_probe.ppm";
    bool probe_ok = tvg_probe_render(img_path_disk, 720, 720, probe_ppm);
    printf("[%s] ThorVG probe result: %s (ppm: %s)\n", TAG, probe_ok ? "OK" : "FAIL", probe_ppm);

    // Also probe the minimal SVG (strokes only, no text/metadata)
    const char* minimal_svg = "../../assets/dashboard_strokes_only.svg";
    const char* minimal_ppm = "./tvg_probe_minimal.ppm";
    printf("[%s] Running ThorVG probe on minimal SVG (strokes only)...\n", TAG);
    bool minimal_ok = tvg_probe_render(minimal_svg, 720, 720, minimal_ppm);
    printf("[%s] ThorVG minimal probe result: %s (ppm: %s)\n", TAG, minimal_ok ? "OK" : "FAIL", minimal_ppm);

    // Try to load SVG via POSIX filesystem driver
    // POSIX driver uses 'P:' prefix, path is relative to current working directory
    const char* img_path_lvgl = "P:../../assets/dashboard.svg";
    printf("[%s] Attempting to load SVG via LVGL FS: %s\n", TAG, img_path_lvgl);
    
    // Test LVGL FS driver
    lv_fs_file_t file;
    lv_fs_res_t fs_res = lv_fs_open(&file, img_path_lvgl, LV_FS_MODE_RD);
    if (fs_res == LV_FS_RES_OK) {
        printf("[%s] ✓ Successfully opened SVG via LVGL FS\n", TAG);
        uint32_t file_pos = 0;
        lv_fs_seek(&file, 0, LV_FS_SEEK_END);
        lv_fs_tell(&file, &file_pos);
        printf("[%s]   File size via LVGL FS: %u bytes\n", TAG, file_pos);
        lv_fs_close(&file);
    } else {
        printf("[%s] ✗ Failed to open SVG via LVGL FS: res=%d\n", TAG, fs_res);
    }

    // Create image widget and load dashboard_final.svg
    printf("[%s] Creating image widget (dashboard_final.svg)...\n", TAG);
    lv_obj_t* img = lv_image_create(lv_screen_active());
    
    // Set magenta border for debugging
    lv_obj_set_style_border_color(img, lv_color_hex(0xFF00FF), 0);
    lv_obj_set_style_border_width(img, 10, 0);

    // Load the SVG final version (text-free, strokes only)
    const char* tiny_img_path = "P:../../assets/dashboard_final.svg";
    printf("[%s] Setting image source to: %s\n", TAG, tiny_img_path);
    printf("[%s] [DEBUG] img pointer: %p\n", TAG, (void*)img);
    printf("[%s] [DEBUG] img is valid LVGL object\n", TAG);
    
    lv_image_set_src(img, tiny_img_path);
    
    // Scale the SVG to fit the display (SVG viewBox might be very large)
    // Set size to match display dimensions
    lv_obj_set_width(img, 720);
    lv_obj_set_height(img, 720);
    
    // Get image descriptor to check if decode succeeded
    const void* img_src = lv_image_get_src(img);
    if (img_src) {
        printf("[%s] ✓ dashboard_final.svg decoded successfully!\n", TAG);
        // Cast to image descriptor to get dimensions
        const lv_image_dsc_t* img_dsc = (const lv_image_dsc_t*)img_src;
        printf("[%s]   Width: %d, Height: %d\n", TAG, img_dsc->header.w, img_dsc->header.h);
    } else {
        printf("[%s] ✗ dashboard_final.svg decode failed - descriptor is NULL\n", TAG);
        printf("[%s] Checking image coordinates...\n", TAG);
        lv_area_t coords;
        lv_obj_get_coords(img, &coords);
        printf("[%s]   x1=%d, y1=%d, x2=%d, y2=%d (width=%d, height=%d)\n", TAG,
               coords.x1, coords.y1, coords.x2, coords.y2, 
               lv_obj_get_width(img), lv_obj_get_height(img));
    }
    
    lv_obj_center(img);
    
    printf("[%s] Flush count after SVG render: %d\n", TAG, flush_count);

    // Add status label
    lv_obj_t* label = lv_label_create(lv_screen_active());
    if (img_src) {
        lv_label_set_text(label, "✓ MINIMAL SVG LOADED\n(strokes only)\n\nPress ESC to exit");
    } else {
        lv_label_set_text(label, "✗ SVG DECODE FAILED\n\nPress ESC to exit\n\nCheck console output");
    }
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    printf("[%s] ========== END SVG TEST ==========\n\n", TAG);
    printf("[%s] Window created. Press ESC in window to exit or click X to close\n", TAG);
    printf("[%s] Rendering...\n\n", TAG);
    fflush(stdout);

    // Main event loop
    bool running = true;
    SDL_Event event;
    int frame = 0;
    while (running) {
        // Handle SDL events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                printf("[%s] Window closed\n", TAG);
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                    printf("[%s] ESC pressed\n", TAG);
                }
            }
        }
        
        // Render LVGL
        lv_timer_handler();
        frame++;
        
        if (frame % 60 == 0) {
            printf("[%s] Frame %d, Flushes: %d\n", TAG, frame, flush_count);
        }
        
        SDL_Delay(16);  // ~60 FPS
    }

    printf("[%s] Exiting...\n", TAG);
    fflush(stdout);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
