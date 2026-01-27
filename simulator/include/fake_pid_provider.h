#pragma once

#include <cstdint>
#include <vector>

namespace digidash {

/**
 * @brief Fake OBD2 PID provider for simulator testing
 */
class FakePIDProvider {
public:
    FakePIDProvider();

    /**
     * @brief Get a fake PID value (simulates OBD2 data)
     */
    float get_engine_rpm();
    float get_vehicle_speed();
    float get_throttle_position();
    float get_coolant_temp();

    /**
     * @brief Update provider state
     */
    void update(uint32_t delta_ms);

    /**
     * @brief Set simulation mode (sine wave, constant, etc.)
     */
    void set_simulation_mode(int mode);

private:
    uint32_t elapsed_time_;
    int simulation_mode_;
};

} // namespace digidash
