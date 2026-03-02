#include "render_engine.h"
#include "tile_height_renderer.h"

namespace digidash {

RenderEngine::RenderEngine(DisplayDriver& display, uint32_t tile_height)
    : display_(display)
    , renderer_(std::make_unique<TileHeightRenderer>(display, tile_height)) {
}

RenderEngine::~RenderEngine() = default;

bool RenderEngine::initialize() {
    return renderer_->initialize();
}

bool RenderEngine::load_gauge(const uint8_t* data, size_t size) {
    return renderer_->load_gauge(data, size);
}

void RenderEngine::render_frame() {
    renderer_->render_frame();
}

void RenderEngine::set_pid_value(uint32_t pid_id, float value) {
    renderer_->set_pid_value(pid_id, value);
}

void RenderEngine::set_obd_connected(bool connected) {
    renderer_->set_obd_connected(connected);
}

} // namespace digidash
