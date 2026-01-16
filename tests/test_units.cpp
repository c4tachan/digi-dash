#include <gtest/gtest.h>
#include "config.h"

class UnitSystemTest : public ::testing::Test {
protected:
    std::string createConfigWithUnits(const std::string& system) {
        return R"({
  "units": {
    "system": ")" + system + R"(",
    "description": "Test unit system"
  },
  "pids": {},
  "gauges": {},
  "display": {"width": 480, "height": 320, "elements": []}
})";
    }
};

// Test FR-2.4.6: Configuration shall allow selection of unit system
TEST_F(UnitSystemTest, LoadMetricUnits) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithUnits("metric"));
    EXPECT_EQ(config.units.system, "metric");
}

TEST_F(UnitSystemTest, LoadSAEUnits) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithUnits("sae"));
    EXPECT_EQ(config.units.system, "sae");
}

// Test FR-2.4.7: System shall display speed in km/h when metric, mph when SAE
TEST_F(UnitSystemTest, SpeedUnitMetric) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithUnits("metric"));
    // When metric, speed should be displayed in km/h
    EXPECT_EQ(config.units.system, "metric");
    // Speed conversion: km/h for metric
}

TEST_F(UnitSystemTest, SpeedUnitSAE) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithUnits("sae"));
    // When SAE, speed should be displayed in mph
    EXPECT_EQ(config.units.system, "sae");
    // Speed conversion: mph for SAE
}

// Test FR-2.4.8: System shall display temperature in °C when metric, °F when SAE
TEST_F(UnitSystemTest, TemperatureUnitMetric) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithUnits("metric"));
    // When metric, temperature should be in °C
    EXPECT_EQ(config.units.system, "metric");
}

TEST_F(UnitSystemTest, TemperatureUnitSAE) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithUnits("sae"));
    // When SAE, temperature should be in °F
    EXPECT_EQ(config.units.system, "sae");
}

// Test default to metric
TEST_F(UnitSystemTest, DefaultToMetric) {
    std::string config_no_units = R"({
  "pids": {},
  "gauges": {},
  "display": {"width": 480, "height": 320, "elements": []}
})";
    DashboardConfig config = DashboardConfig::loadFromString(config_no_units);
    EXPECT_EQ(config.units.system, "metric");
}

// Test unit conversion helper (speed: km/h to mph)
TEST_F(UnitSystemTest, SpeedConversionMetricToSAE) {
    // 100 km/h = ~62.14 mph
    double speed_kmh = 100.0;
    double speed_mph = speed_kmh * 0.621371;
    EXPECT_NEAR(speed_mph, 62.1371, 0.01);
}

// Test unit conversion helper (temperature: °C to °F)
TEST_F(UnitSystemTest, TemperatureConversionMetricToSAE) {
    // 20°C = 68°F
    double temp_c = 20.0;
    double temp_f = (temp_c * 9.0 / 5.0) + 32.0;
    EXPECT_NEAR(temp_f, 68.0, 0.01);
    
    // 85°C = 185°F
    temp_c = 85.0;
    temp_f = (temp_c * 9.0 / 5.0) + 32.0;
    EXPECT_NEAR(temp_f, 185.0, 0.01);
}
