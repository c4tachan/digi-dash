#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

namespace digidash {

/**
 * @brief PID data binding system
 * 
 * Maps OBD2 PID values to gauge elements (needle position, text, etc.)
 * Handles unit conversions and value formatting.
 */
class PIDBindingSystem {
public:
    enum class PIDType {
        ENGINE_SPEED,        // RPM
        VEHICLE_SPEED,       // KPH/MPH
        THROTTLE_POSITION,   // Percentage
        FUEL_PRESSURE,       // PSI/BAR
        COOLANT_TEMP,        // Celsius/Fahrenheit
        INTAKE_TEMP,         // Celsius/Fahrenheit
        MASS_AIRFLOW,        // g/s
        OXYGEN_SENSOR,       // V
        CUSTOM
    };

    struct PIDBinding {
        uint32_t pid_id;
        PIDType type;
        std::string unit;
        float min_value;
        float max_value;
        float scale;
        float offset;
    };

    PIDBindingSystem();
    ~PIDBindingSystem();

    /**
     * @brief Register a PID binding
     */
    void register_binding(const PIDBinding& binding);

    /**
     * @brief Set the current value for a PID
     */
    void set_pid_value(uint32_t pid_id, float raw_value);

    /**
     * @brief Get the scaled value for a PID
     */
    float get_pid_value(uint32_t pid_id) const;

    /**
     * @brief Get formatted string representation
     */
    std::string format_value(uint32_t pid_id) const;

private:
    std::unordered_map<uint32_t, PIDBinding> bindings_;
    std::unordered_map<uint32_t, float> current_values_;
};

} // namespace digidash
