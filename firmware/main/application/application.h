#pragma once

#include <memory>
#include "platform/display/display_driver.h"
#include "subsystems/storage/storage_manager.h"
#include "subsystems/rendering/render_engine.h"

namespace digidash {

class Application {
public:
    Application();
    ~Application();

    bool initialize();
    void run();
    
private:
    std::unique_ptr<DisplayDriver> display_;
    std::unique_ptr<StorageManager> storage_;
    std::unique_ptr<RenderEngine> renderer_;
    
    bool initialized_;
};

} // namespace digidash
