#include "digidash/gauge_scene.h"

namespace digidash {

GaugeScene::GaugeScene() {}
GaugeScene::~GaugeScene() {}

bool GaugeScene::load_gauge(const BinaryGaugeLoader::GaugeAsset&) { return true; }
void GaugeScene::update(uint32_t) {}
void GaugeScene::render(uint8_t*, int, int, int, int) {}
void GaugeScene::set_pid_value(uint32_t, float) {}

} // namespace digidash
