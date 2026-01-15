/**
 * @file mock_data.h
 * @brief Mock data generator for simulating vehicle sensor data
 */

#ifndef MOCK_DATA_H
#define MOCK_DATA_H

#include "data_source.h"
#include <cstdint>

class MockData : public DataSource {
public:
    MockData();
    
    /**
     * @brief Update the mock data values (call this periodically)
     */
    void update() override;
    
    /**
     * @brief Get current RPM value
     * @return RPM (0-8000)
     */
    int getRPM() const override { return rpm_; }
    
    /**
     * @brief Get current speed value
     * @return Speed in km/h (0-200)
     */
    int getSpeed() const override { return speed_; }
    
    /**
     * @brief Get current coolant temperature
     * @return Temperature in Celsius (60-120)
     */
    int getCoolantTemp() const override { return coolant_temp_; }
    
    /**
     * @brief Mock data is always connected
     */
    bool isConnected() const override { return true; }
    
    /**
     * @brief Get data source name
     */
    const char* getName() const override { return "Mock Data"; }
    
private:
    uint16_t rpm_;
    uint16_t speed_;
    uint8_t coolant_temp_;
    
    float rpm_phase_;
    float speed_phase_;
    float temp_phase_;
};

#endif // MOCK_DATA_H
