/**
 * @file main.cpp
 * @brief Main entry point for the LVGL simulator
 */

#include "lvgl.h"
#include "lv_examples.h"
#include "lv_demos.h"
#include "dashboard.h"
#include "config.h"
#include "data_source.h"
#include "mock_data.h"
#include "obd2_data_source.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <memory>

/**
 * @brief Tick callback for LVGL
 */
static uint32_t tick_get_cb(void) {
    return SDL_GetTicks();
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    printf("Starting Digi-Dash Simulator...\n");
    
    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Initialize LVGL
    lv_init();
    
    // Set tick callback
    lv_tick_set_cb(tick_get_cb);
    
    printf("LVGL initialized successfully\n");
    
    // Load configuration from JSON file
    DashboardConfig config = DashboardConfig::loadFromFile("dashboard.json");
    if (config.display.elements.empty()) {
        fprintf(stderr, "Failed to load configuration from dashboard.json\n");
        return 1;
    }
    
    // Create dashboard from configuration
    std::unique_ptr<Dashboard> dashboard;
    try {
        dashboard = std::make_unique<Dashboard>(config.display, config.gauges);
        if (!dashboard->isValid()) {
            fprintf(stderr, "Failed to create valid dashboard\n");
            return 1;
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "Exception creating dashboard: %s\n", e.what());
        return 1;
    }
    
    // Parse command-line arguments for data source
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
    
    // Create data source (mock or real OBD-II)
    std::unique_ptr<DataSource> data_source;
    
    if (use_mock) {
        data_source = std::make_unique<MockData>();
        printf("Using mock data generator\n");
    } else if (obd2_device) {
        data_source = std::make_unique<OBD2DataSource>(obd2_device, config.pids);
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
    printf("Close window or press ESC to exit\n");
    fflush(stdout);
    
    while (running && dashboard->isValid()) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    running = false;
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
                // Route mouse events to dashboard
                dashboard->handleEvent(event);
            }
        }
        
        // Update data source periodically
        uint32_t current_time = SDL_GetTicks();
        if (current_time - last_update >= 50) {  // ~20 FPS updates
            data_source->update();
            dashboard->update(*data_source, config.pids);
            last_update = current_time;
            frame_count++;
            
            if (frame_count % 20 == 0) {
                printf("Frame %u - RPM: %d, Speed: %d, Temp: %d°C\n", 
                       frame_count, data_source->getRPM(), data_source->getSpeed(), 
                       data_source->getCoolantTemp());
                fflush(stdout);
            }
        }
        
        // Handle LVGL rendering once per frame
        lv_timer_handler();
        
        // Render the dashboard
        dashboard->render();
        
        SDL_Delay(5);
    }
    
    printf("Exiting...\n");
    fflush(stdout);
    dashboard.reset();
    SDL_Quit();
    
    return 0;
}
