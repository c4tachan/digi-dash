#include "sdl_display.h"
#include "sdl_input.h"
#include "fake_pid_provider.h"
#include "digidash/gauge_scene.h"
#include "digidash/binary_gauge_loader.h"
#include "digidash/platform_input.h"

#include <iostream>
#include <chrono>
#include <cstring>
#include <SDL2/SDL.h>

using namespace digidash;

int main(int argc, char* argv[]) {
    std::string gauge_file = "dashboard_tiny.gauge";
    
    // Allow override from command line
    if (argc > 1) {
        gauge_file = argv[1];
    }
    
    // Initialize display
    auto display = std::make_unique<SDLDisplay>(720, 720);
    if (!display->init()) {
        std::cerr << "Failed to initialize SDL display\n";
        return 1;
    }

    // Initialize input
    auto input = std::make_unique<SDLInput>();

    // Load gauge asset
    BinaryGaugeLoader loader;
    BinaryGaugeLoader::GaugeAsset asset;
    
    if (!loader.load_from_file(gauge_file, asset)) {
        std::cerr << "Failed to load gauge file: " << gauge_file << "\n";
        std::cerr << "Make sure you've run: python3 tools/svg_preprocessor/svg_parser.py assets/dashboard_tiny.svg\n";
        return 1;
    }
    
    std::cout << "Loaded gauge: " << asset.width << "x" << asset.height << "\n";

    // Initialize gauge scene
    auto gauge = std::make_unique<GaugeScene>();
    if (!gauge->load_gauge(asset)) {
        std::cerr << "Failed to load gauge into scene\n";
        return 1;
    }

    // Initialize fake PID provider
    FakePIDProvider pid_provider;

    // Main loop
    auto last_time = std::chrono::high_resolution_clock::now();
    const uint32_t target_fps = 30;
    const uint32_t frame_time_ms = 1000 / target_fps;

    std::cout << "Digi-Dash Simulator started (720x720, 30 FPS)\n";
    std::cout << "Rendering gauge from: " << gauge_file << "\n";
    std::cout << "Press Q or close window to exit\n";

    while (display->is_running()) {
        // Process input events first
        input->process_sdl_events();
        
        digidash::PlatformInput::InputEvent event;
        while (input->poll_event(event)) {
            if (event.type == digidash::PlatformInput::InputType::BUTTON_PRESS) {
                if (event.button_id == SDLK_ESCAPE || event.button_id == SDLK_q) {
                    display->request_quit();
                    break;
                }
            }
        }
        
        auto now = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - last_time)
                        .count();

        if (delta >= frame_time_ms) {
            last_time = now;

            // Update simulation
            pid_provider.update(delta);
            gauge->set_pid_value(0, pid_provider.get_engine_rpm());
            gauge->set_pid_value(1, pid_provider.get_vehicle_speed());
            gauge->set_pid_value(2, pid_provider.get_throttle_position());
            gauge->set_pid_value(3, pid_provider.get_coolant_temp());

            // Update gauge
            gauge->update(delta);

            // Render
            display->clear(0xFF000000); // Black background
            uint8_t* fb = display->lock_framebuffer();
            gauge->render(fb, display->get_width(), display->get_height(),
                         display->get_stride());
            display->unlock_and_update();
        }
    }

    std::cout << "Simulator exiting normally\n";
    return 0;
}
