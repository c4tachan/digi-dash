#include "fake_pid_provider.h"
#include <cmath>

namespace digidash {

namespace {

float triangle01(uint32_t elapsed_ms, uint32_t period_ms) {
    if (period_ms < 2) {
        return 0.0f;
    }

    const uint32_t half_period = period_ms / 2;
    const uint32_t phase = elapsed_ms % period_ms;

    if (phase <= half_period) {
        return static_cast<float>(phase) / static_cast<float>(half_period);
    }

    return static_cast<float>(period_ms - phase) / static_cast<float>(half_period);
}

}

FakePIDProvider::FakePIDProvider() : elapsed_time_(0), simulation_mode_(0) {}

float FakePIDProvider::get_engine_rpm() {
    // Full configured sweep: 0-8000 RPM
    const float t = triangle01(elapsed_time_, 5000);
    return 8000.0f * t;
}

float FakePIDProvider::get_vehicle_speed() {
    // Sine wave: 0-100 KPH
    float sine = std::sin(elapsed_time_ * 0.001f);
    return 50.0f + sine * 50.0f;
}

float FakePIDProvider::get_throttle_position() {
    // Sine wave: 0-100%
    float sine = std::sin(elapsed_time_ * 0.003f);
    return 50.0f + sine * 50.0f;
}

float FakePIDProvider::get_coolant_temp() {
    // Full configured sweep: 160-260
    const float t = triangle01(elapsed_time_, 6000);
    return 160.0f + 100.0f * t;
}

void FakePIDProvider::update(uint32_t delta_ms) { elapsed_time_ += delta_ms; }

void FakePIDProvider::set_simulation_mode(int mode) {
    simulation_mode_ = mode;
}

} // namespace digidash
