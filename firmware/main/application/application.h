#pragma once

#include <memory>
#include <cstdint>
#include "platform/display/display_driver.h"
#include "subsystems/storage/storage_manager.h"
#include "subsystems/rendering/render_engine.h"
#include "subsystems/rendering/text_renderer.h"

namespace digidash {

class Application {
public:
    Application();
    ~Application();

    bool initialize();
    void run();
    
private:
    void display_hello_world(float angle_rad = 0.0f);
    std::unique_ptr<DisplayDriver> display_;
    std::unique_ptr<StorageManager> storage_;
    std::unique_ptr<RenderEngine> renderer_;
    std::unique_ptr<uint16_t[]> pattern_base_buffer_;
    
    bool initialized_;
    float crosshair_angle_rad_;
    uint32_t live_pclk_hz_;
    size_t live_sweep_index_;
    uint32_t last_pclk_step_tick_;
};

} // namespace digidash
