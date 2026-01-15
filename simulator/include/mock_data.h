/**
 * @file mock_data.h
 * @brief Mock data generator for simulating vehicle sensor data
 */

#ifndef MOCK_DATA_H
#define MOCK_DATA_H

#include <cstdint>

class MockData {
public:
    MockData();
    
    /**
     * @brief Update the mock data values (call this periodically)
     */
    void update();
    
    /**
     * @brief Get current RPM value
     * @return RPM (0-8000)
     */
    uint16_t getRPM() const { return rpm_; }
    
    /**
     * @brief Get current speed value
     * @return Speed in km/h (0-200)
     */
    uint16_t getSpeed() const { return speed_; }
    
    /**
     * @brief Get current coolant temperature
     * @return Temperature in Celsius (60-120)
     */
    uint8_t getCoolantTemp() const { return coolant_temp_; }
    
private:
    uint16_t rpm_;
    uint16_t speed_;
    uint8_t coolant_temp_;
    
    float rpm_phase_;
    float speed_phase_;
    float temp_phase_;
};

#endif // MOCK_DATA_H
