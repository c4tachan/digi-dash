#pragma once

#include <memory>
#include "platform/display/display_driver.h"
#include "subsystems/storage/storage_manager.h"
#include "subsystems/rendering/render_engine.h"
#include "subsystems/rendering/text_renderer.h"
#include "subsystems/obd2/obd2_uart_manager.h"

namespace digidash {

class Application {
public:
    Application();
    ~Application();

    bool initialize();
    void run();
    
private:
    void display_hello_world();
    std::unique_ptr<DisplayDriver> display_;
    std::unique_ptr<StorageManager> storage_;
    std::unique_ptr<RenderEngine> renderer_;
    std::unique_ptr<OBD2UartManager> obd2_;
    
    bool initialized_;
};

} // namespace digidash
