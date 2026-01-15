/**
 * @file mock_data.cpp
 * @brief Implementation of mock data generator
 */

#include "mock_data.h"
#include <cmath>

MockData::MockData()
    : rpm_(1000)
    , speed_(0)
    , coolant_temp_(85)
    , rpm_phase_(0.0f)
    , speed_phase_(0.0f)
    , temp_phase_(0.0f)
{
}

void MockData::update() {
    // Simulate RPM oscillating between 800 and 6000
    rpm_phase_ += 0.02f;
    rpm_ = static_cast<uint16_t>(3400 + 2600 * std::sin(rpm_phase_));
    
    // Simulate speed oscillating between 0 and 120 km/h
    speed_phase_ += 0.015f;
    speed_ = static_cast<uint16_t>(60 + 60 * std::sin(speed_phase_));
    
    // Simulate coolant temp slowly varying between 80 and 95°C
    temp_phase_ += 0.005f;
    coolant_temp_ = static_cast<uint8_t>(87 + 8 * std::sin(temp_phase_));
}
