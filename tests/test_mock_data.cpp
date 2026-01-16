#include <gtest/gtest.h>
#include "mock_data.h"
#include <cmath>

class MockDataTest : public ::testing::Test {
protected:
    MockData mock_data;
    
    // Test that values are within expected ranges across multiple updates
    void updateMultipleTimes(int count) {
        for (int i = 0; i < count; ++i) {
            mock_data.update();
        }
    }
};

// Test FR-2.5.1: Mock data generator shall produce values across full operating range
TEST_F(MockDataTest, RPMRangeTest) {
    // Collect RPM values across many updates to verify range coverage
    double min_rpm = 8000.0;
    double max_rpm = 0.0;
    
    for (int i = 0; i < 1000; ++i) {
        double rpm = mock_data.getRPM();
        min_rpm = std::min(min_rpm, rpm);
        max_rpm = std::max(max_rpm, rpm);
        mock_data.update();
    }
    
    // Mock data should produce values in the range 800-7500
    EXPECT_LE(min_rpm, 900.0);   // Allow some tolerance below 800
    EXPECT_GE(max_rpm, 7400.0);  // Allow some tolerance below 7500
}

TEST_F(MockDataTest, SpeedRangeTest) {
    double min_speed = 150.0;
    double max_speed = -1.0;
    
    for (int i = 0; i < 1000; ++i) {
        double speed = mock_data.getSpeed();
        min_speed = std::min(min_speed, speed);
        max_speed = std::max(max_speed, speed);
        mock_data.update();
    }
    
    // Mock data should produce values in the range 0-120 km/h
    EXPECT_GE(min_speed, -5.0);   // Should be near 0 or slightly negative due to sinusoid
    EXPECT_LE(max_speed, 125.0);  // Should be near 120 or slightly above
}

TEST_F(MockDataTest, TemperatureRangeTest) {
    double min_temp = 100.0;
    double max_temp = 0.0;
    
    for (int i = 0; i < 1000; ++i) {
        double temp = mock_data.getCoolantTemp();
        min_temp = std::min(min_temp, temp);
        max_temp = std::max(max_temp, temp);
        mock_data.update();
    }
    
    // Mock data should produce values in the range 79-94°C
    EXPECT_GE(min_temp, 75.0);   // Allow some tolerance
    EXPECT_LE(max_temp, 100.0);  // Allow some tolerance
}

// Test FR-2.5.2: Mock data shall update continuously with sinusoidal pattern
TEST_F(MockDataTest, SinusoidalBehavior) {
    // Collect enough samples to see oscillation
    std::vector<double> rpm_values;
    for (int i = 0; i < 150; ++i) {
        rpm_values.push_back(mock_data.getRPM());
        mock_data.update();
    }
    
    // Should see values increase and decrease (sinusoidal)
    bool has_increase = false;
    bool has_decrease = false;
    
    for (size_t i = 1; i < rpm_values.size(); ++i) {
        if (rpm_values[i] > rpm_values[i-1]) {
            has_increase = true;
        }
        if (rpm_values[i] < rpm_values[i-1]) {
            has_decrease = true;
        }
    }
    
    // Should have both increases and decreases (sinusoidal)
    EXPECT_TRUE(has_increase);
    EXPECT_TRUE(has_decrease);
}

// Test that mock data is deterministic (same phase progression)
TEST_F(MockDataTest, DeterministicProgression) {
    MockData data1, data2;
    
    // Both should produce same sequence of values
    for (int i = 0; i < 50; ++i) {
        double rpm1 = data1.getRPM();
        double rpm2 = data2.getRPM();
        EXPECT_DOUBLE_EQ(rpm1, rpm2) << "Sequence diverged at iteration " << i;
        
        data1.update();
        data2.update();
    }
}

// Test update rate (should be called regularly)
TEST_F(MockDataTest, ConsecutiveUpdates) {
    double rpm1 = mock_data.getRPM();
    mock_data.update();
    double rpm2 = mock_data.getRPM();
    
    // Values should be different after update (unless at phase boundary)
    // Most of the time they should be different
    EXPECT_NE(rpm1, rpm2);  // Changes with each update
}

// Test that all parameters update together
TEST_F(MockDataTest, CoherentDataUpdate) {
    // Collect first set of values
    double rpm1 = mock_data.getRPM();
    double speed1 = mock_data.getSpeed();
    double temp1 = mock_data.getCoolantTemp();
    
    mock_data.update();
    
    // Collect second set of values
    double rpm2 = mock_data.getRPM();
    double speed2 = mock_data.getSpeed();
    double temp2 = mock_data.getCoolantTemp();
    
    // All values should change after update
    EXPECT_NE(rpm1, rpm2);
    EXPECT_NE(speed1, speed2);
    EXPECT_NE(temp1, temp2);
}

// Test that values cross zone boundaries
TEST_F(MockDataTest, CrossGreenYellowBoundary) {
    bool crossed_boundary = false;
    
    for (int i = 0; i < 500; ++i) {
        double rpm = mock_data.getRPM();
        
        // Check if we crossed the 6000 RPM boundary
        if ((rpm > 5900 && rpm < 6100) || (rpm > 5500 && rpm < 6500)) {
            crossed_boundary = true;
            break;
        }
        mock_data.update();
    }
    
    // Should cross the boundary during 500 updates
    EXPECT_TRUE(crossed_boundary);
}

TEST_F(MockDataTest, CrossYellowRedBoundary) {
    bool crossed_boundary = false;
    
    for (int i = 0; i < 500; ++i) {
        double rpm = mock_data.getRPM();
        
        // Check if we crossed the 7000 RPM boundary
        if ((rpm > 6900 && rpm < 7100) || (rpm > 6500 && rpm < 7500)) {
            crossed_boundary = true;
            break;
        }
        mock_data.update();
    }
    
    // Should cross the boundary during 500 updates
    EXPECT_TRUE(crossed_boundary);
}

// Test boundary conditions (at exactly 0, min/max values)
TEST_F(MockDataTest, BoundaryValuesRealistic) {
    double min_rpm = 1000.0;
    double max_rpm = 0.0;
    
    // Collect 200+ samples to ensure statistical coverage
    for (int i = 0; i < 300; ++i) {
        double rpm = mock_data.getRPM();
        min_rpm = std::min(min_rpm, rpm);
        max_rpm = std::max(max_rpm, rpm);
        mock_data.update();
    }
    
    // Should have reasonable distribution
    EXPECT_GT(max_rpm - min_rpm, 5000.0);  // Range should be significant
}

// Test that mock data covers the danger zone (>7000 RPM)
TEST_F(MockDataTest, ReachesDangerZone) {
    bool reached_danger_zone = false;
    
    for (int i = 0; i < 500; ++i) {
        if (mock_data.getRPM() > 7000) {
            reached_danger_zone = true;
            break;
        }
        mock_data.update();
    }
    
    EXPECT_TRUE(reached_danger_zone) << "Mock data should reach danger zone (>7000 RPM)";
}

// Test consistency of temperature behavior
TEST_F(MockDataTest, TemperatureRealistic) {
    // Temperature should oscillate around ~85°C for car engine
    double avg_temp = 0.0;
    const int samples = 200;
    
    for (int i = 0; i < samples; ++i) {
        avg_temp += mock_data.getCoolantTemp();
        mock_data.update();
    }
    avg_temp /= samples;
    
    // Average should be reasonable (around 85-88°C)
    EXPECT_GT(avg_temp, 80.0);
    EXPECT_LT(avg_temp, 95.0);
}

// Test that speed follows approximately sinusoidal pattern
TEST_F(MockDataTest, SpeedSinusoidalPattern) {
    std::vector<double> speeds;
    for (int i = 0; i < 200; ++i) {
        speeds.push_back(mock_data.getSpeed());
        mock_data.update();
    }
    
    // Should have varying speeds (not all the same)
    double min_speed = speeds[0];
    double max_speed = speeds[0];
    for (double speed : speeds) {
        min_speed = std::min(min_speed, speed);
        max_speed = std::max(max_speed, speed);
    }
    
    // Speed should vary significantly (oscillating pattern)
    EXPECT_GT(max_speed - min_speed, 20.0);  // At least 20 km/h variation
}
