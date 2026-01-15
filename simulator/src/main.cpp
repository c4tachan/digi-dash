/**
 * @file main.cpp
 * @brief Main entry point for the LVGL simulator
 */

#include "lvgl.h"
#include "lv_examples.h"
#include "lv_demos.h"
#include "window_manager.h"
#include "config.h"
#include "data_source.h"
#include "mock_data.h"
#include "obd2_data_source.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <memory>

// Display configuration
static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 720;

// SDL and LVGL objects
static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* screen_texture = nullptr;
static void* lvgl_buf1 = nullptr;
static void* lvgl_buf2 = nullptr;

/**
 * @brief Display flush callback - copy LVGL buffer to SDL texture
 */
static void display_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    // Lock the texture for pixel access
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(screen_texture, nullptr, &pixels, &pitch) == 0) {
        uint32_t* dest = (uint32_t*)pixels;
        uint32_t* src = (uint32_t*)px_map;
        
        // Copy the area
        for (int y = area->y1; y <= area->y2; y++) {
            int src_offset = (y - area->y1) * (area->x2 - area->x1 + 1);
            int dest_offset = y * (pitch / 4) + area->x1;
            
            for (int x = area->x1; x <= area->x2; x++) {
                dest[dest_offset++] = src[src_offset++];
            }
        }
        
        SDL_UnlockTexture(screen_texture);
    }
    
    lv_display_flush_ready(disp);
}

/**
 * @brief Tick callback for LVGL
 */
static uint32_t tick_get_cb(void) {
    return SDL_GetTicks();
}

/**
 * @brief Initialize SDL2
 */
static bool initialize_sdl() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }
    
    // Create window
    window = SDL_CreateWindow("Digi-Dash Simulator",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               WINDOW_WIDTH, WINDOW_HEIGHT,
                               SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    
    // Create persistent texture for rendering
    screen_texture = SDL_CreateTexture(renderer,
                                       SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!screen_texture) {
        fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    
    printf("SDL2 initialized successfully\n");
    return true;
}

/**
 * @brief Clean up SDL2 resources
 */
static void cleanup() {
    if (lvgl_buf1) free(lvgl_buf1);
    if (lvgl_buf2) free(lvgl_buf2);
    if (screen_texture) SDL_DestroyTexture(screen_texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    printf("Cleaned up SDL2 resources\n");
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    printf("Starting Digi-Dash Simulator...\n");
    
    // Parse command-line arguments
    const char* obd2_device = nullptr;
    bool use_mock = true;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--obd2") == 0 && i + 1 < argc) {
            obd2_device = argv[++i];
            use_mock = false;
            printf("OBD-II mode: Will connect to %s\n", obd2_device);
        } else if (strcmp(argv[i], "--mock") == 0) {
            use_mock = true;
            printf("Mock data mode enabled\n");
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --mock              Use mock data generator (default)\n");
            printf("  --obd2 <device>     Connect to OBD-II adapter at device path\n");
            printf("                      Examples: /dev/rfcomm0, /dev/ttyUSB0\n");
            printf("  --help, -h          Show this help message\n");
            return 0;
        }
    }
    
    // Initialize SDL2
    if (!initialize_sdl()) {
        fprintf(stderr, "Failed to initialize SDL2\n");
        return 1;
    }
    
    // Initialize LVGL
    lv_init();
    
    // Set tick callback
    lv_tick_set_cb(tick_get_cb);
    
    // Create display
    lv_display_t* disp = lv_display_create(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // Allocate two buffers (1/10 screen size each for partial rendering)
    size_t buf_size = (WINDOW_WIDTH * WINDOW_HEIGHT / 10) * 4;
    lvgl_buf1 = malloc(buf_size);
    lvgl_buf2 = malloc(buf_size);
    
    if (!lvgl_buf1 || !lvgl_buf2) {
        fprintf(stderr, "Failed to allocate LVGL buffers\n");
        return 1;
    }
    
    // Set double buffering with partial rendering
    lv_display_set_buffers(disp, lvgl_buf1, lvgl_buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // Set color format
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_ARGB8888);
    
    // Set flush callback
    lv_display_set_flush_cb(disp, display_flush_cb);
    
    printf("LVGL initialized successfully\n");
    
    // Create window manager for managing multiple gauge windows
    WindowManager window_manager;
    
    // Load configuration from JSON file
    DashboardConfig config = DashboardConfig::loadFromFile("dashboard.json");
    if (config.windows.empty()) {
        fprintf(stderr, "Failed to load configuration from dashboard.json\n");
        return 1;
    }
    
    // Initialize windows from configuration
    if (!window_manager.initializeFromConfig(config)) {
        fprintf(stderr, "Failed to initialize windows from configuration\n");
        return 1;
    }
    
    // Create data source (mock or real OBD-II)
    std::unique_ptr<DataSource> data_source;
    
    if (use_mock) {
        data_source = std::make_unique<MockData>();
        printf("Using mock data generator\n");
    } else if (obd2_device) {
        // Create PID configuration for standard OBD-II PIDs
        std::map<std::string, PidConfig> pid_config;
        pid_config["rpm"] = {"010C", "RPM", "rpm"};
        pid_config["speed"] = {"010D", "Speed", "km/h"};
        pid_config["coolant_temp"] = {"0105", "Coolant Temperature", "°C"};
        
        data_source = std::make_unique<OBD2DataSource>(obd2_device, pid_config);
        printf("Attempting OBD-II connection to %s\n", obd2_device);
    } else {
        fprintf(stderr, "Error: No data source configured\n");
        return 1;
    }
    
    printf("Using data source: %s\n", data_source->getName());
    
    // Main event loop
    bool running = true;
    SDL_Event event;
    uint32_t last_update = SDL_GetTicks();
    uint32_t frame_count = 0;
    
    printf("Entering main loop...\n");
    printf("Close any window to remove it, close all to exit\n");
    fflush(stdout);
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    // Remove the closed window
                    window_manager.removeWindow(event.window.windowID);
                    
                    // Exit if no windows remain
                    if (!window_manager.hasWindows()) {
                        running = false;
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    default:
                        break;
                }
            } else if (event.type == SDL_MOUSEMOTION ||
                       event.type == SDL_MOUSEBUTTONDOWN ||
                       event.type == SDL_MOUSEBUTTONUP) {
                // Route mouse events to windows
                window_manager.handleEvent(event);
            }
        }
        
        // Check if any windows remain
        if (!window_manager.hasWindows()) {
            running = false;
            break;
        }
        
        // Update data source periodically
        uint32_t current_time = SDL_GetTicks();
        if (current_time - last_update >= 50) {  // ~20 FPS updates
            data_source->update();
            window_manager.update(*data_source);
            last_update = current_time;
            frame_count++;
            
            if (frame_count % 20 == 0) {
                printf("Frame %u - RPM: %d, Speed: %d, Temp: %d°C | Windows: %zu\n", 
                       frame_count, data_source->getRPM(), data_source->getSpeed(), 
                       data_source->getCoolantTemp(), window_manager.getWindowCount());
                fflush(stdout);
            }
        }
        
        // Handle LVGL rendering once per frame (only if we have windows)
        if (window_manager.hasWindows()) {
            lv_timer_handler();
        }
        
        // Render all windows
        window_manager.renderAll();
        
        SDL_Delay(5);
    }
    
    printf("Exiting...\n");
    fflush(stdout);
    cleanup();
    
    return 0;
}
