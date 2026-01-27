#include "fake_pid_provider.h"
#include <cmath>

namespace digidash {

FakePIDProvider::FakePIDProvider() : elapsed_time_(0), simulation_mode_(0) {}

float FakePIDProvider::get_engine_rpm() {
    // Sine wave: 1000-3500 RPM
    float sine = std::sin(elapsed_time_ * 0.002f);
    return 2250.0f + sine * 1250.0f;
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
    // Ramp up from 60 to 100°C
    float temp = 60.0f + (elapsed_time_ % 5000) / 50.0f;
    return temp > 100.0f ? 100.0f : temp;
}

void FakePIDProvider::update(uint32_t delta_ms) { elapsed_time_ += delta_ms; }

void FakePIDProvider::set_simulation_mode(int mode) {
    simulation_mode_ = mode;
}

} // namespace digidash
