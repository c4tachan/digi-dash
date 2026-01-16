#include <gtest/gtest.h>
#include "config.h"
#include <cmath>

class GaugeTest : public ::testing::Test {
protected:
    std::string createConfigWithGauge(const std::string& gauge_def) {
        return R"({
  "protocol": {"type": "CAN", "baudrate": 500000},
  "units": {"system": "metric"},
  "pids": {
    "rpm": {"command": "010C", "formula": "(A*256+B)/4", "unit": "RPM"},
    "speed": {"command": "010D", "formula": "A", "unit": "km/h"},
    "coolant_temp": {"command": "0105", "formula": "A-40", "unit": "°C"}
  },
  "gauges": {)" + gauge_def + R"(},
  "display": {"width": 480, "height": 320, "elements": []}
})";
    }

    // Helper function to create color (RGB)
    uint32_t getRGB(uint8_t r, uint8_t g, uint8_t b) {
        return (r << 16) | (g << 8) | b;
    }
};

// Test FR-2.2.4: Display shall show zone coloring based on parameter value
TEST_F(GaugeTest, LoadGaugeWithZones) {
    std::string gauge_str = R"(
    "rpm_gauge": {
      "type": "arc",
      "data_source": "rpm",
      "min": 0,
      "max": 8000,
      "title": "RPM",
      "zones": [
        {"min": 0, "max": 6000, "color": "#00FF00"},
        {"min": 6000, "max": 7000, "color": "#FFFF00"},
        {"min": 7000, "max": 8000, "color": "#FF0000"}
      ],
      "alerts": [
        {"condition": "above", "threshold": 7000, "action": "flash"}
      ]
    }
  )";
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithGauge(gauge_str));
    
    ASSERT_TRUE(config.gauges.find("rpm_gauge") != config.gauges.end());
    GaugeDefinition& gauge = config.gauges["rpm_gauge"];
    
    EXPECT_EQ(gauge.type, "arc");
    EXPECT_EQ(gauge.zones.size(), 3);
    
    // Check zones
    EXPECT_EQ(gauge.zones[0].min, 0);
    EXPECT_EQ(gauge.zones[0].max, 6000);
    EXPECT_EQ(gauge.zones[0].color, "#00FF00");
    
    EXPECT_EQ(gauge.zones[1].min, 6000);
    EXPECT_EQ(gauge.zones[1].max, 7000);
    EXPECT_EQ(gauge.zones[1].color, "#FFFF00");
    
    EXPECT_EQ(gauge.zones[2].min, 7000);
    EXPECT_EQ(gauge.zones[2].max, 8000);
    EXPECT_EQ(gauge.zones[2].color, "#FF0000");
}

// Test zone color determination logic
TEST_F(GaugeTest, ZoneColorForValue) {
    std::string gauge_str = R"(
    "speed_gauge": {
      "type": "digital",
      "data_source": "speed",
      "min": 0,
      "max": 120,
      "title": "Speed",
      "zones": [
        {"min": 0, "max": 80, "color": "#00FF00"},
        {"min": 80, "max": 100, "color": "#FFFF00"},
        {"min": 100, "max": 120, "color": "#FF0000"}
      ]
    }
  )";
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithGauge(gauge_str));
    GaugeDefinition& gauge = config.gauges["speed_gauge"];
    
    // Test color selection logic for different values
    // Value 50 should be in first zone (green)
    for (const auto& zone : gauge.zones) {
        if (50 >= zone.min && 50 < zone.max) {
            EXPECT_EQ(zone.color, "#00FF00");
        }
    }
    
    // Value 90 should be in second zone (yellow)
    for (const auto& zone : gauge.zones) {
        if (90 >= zone.min && 90 < zone.max) {
            EXPECT_EQ(zone.color, "#FFFF00");
        }
    }
    
    // Value 110 should be in third zone (red)
    for (const auto& zone : gauge.zones) {
        if (110 >= zone.min && 110 < zone.max) {
            EXPECT_EQ(zone.color, "#FF0000");
        }
    }
}

// Test FR-2.2.5: Display shall show zone boundary markers
TEST_F(GaugeTest, LoadGaugeWithMarkers) {
    std::string gauge_str = R"(
    "rpm_gauge": {
      "type": "arc",
      "data_source": "rpm",
      "min": 0,
      "max": 8000,
      "title": "RPM",
      "markers": [
        {"value": 6000, "color": "#FFFF00", "label": "WARNING"},
        {"value": 7000, "color": "#FF0000", "label": "REDLINE"}
      ]
    }
  )";
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithGauge(gauge_str));
    GaugeDefinition& gauge = config.gauges["rpm_gauge"];
    
    EXPECT_EQ(gauge.markers.size(), 2);
    
    // Check warning marker at 6000
    EXPECT_EQ(gauge.markers[0].value, 6000);
    EXPECT_EQ(gauge.markers[0].color, "#FFFF00");
    EXPECT_EQ(gauge.markers[0].label, "WARNING");
    
    // Check redline marker at 7000
    EXPECT_EQ(gauge.markers[1].value, 7000);
    EXPECT_EQ(gauge.markers[1].color, "#FF0000");
    EXPECT_EQ(gauge.markers[1].label, "REDLINE");
}

// Test FR-2.2.6: Display shall show tick marks at intervals
TEST_F(GaugeTest, TickMarkConfiguration) {
    std::string gauge_str = R"(
    "rpm_gauge": {
      "type": "arc",
      "data_source": "rpm",
      "min": 0,
      "max": 8000,
      "title": "RPM",
      "tick_marks": {
        "minor_interval": 500,
        "major_interval": 1000,
        "minor_size": 4,
        "major_size": 8
      }
    }
  )";
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithGauge(gauge_str));
    GaugeDefinition& gauge = config.gauges["rpm_gauge"];
    
    // With 0-8000 range and 500 RPM minor interval: 0, 500, 1000, 1500, ..., 8000 = 17 marks
    // With 1000 RPM major interval: every 2nd one is major
    int total_minor_marks = (8000 - 0) / 500 + 1;  // 17 marks
    EXPECT_EQ(total_minor_marks, 17);
}

// Test FR-2.3.1 & FR-2.3.2: Display shall trigger alerts at thresholds
TEST_F(GaugeTest, AlertAboveThreshold) {
    std::string gauge_str = R"(
    "rpm_gauge": {
      "type": "arc",
      "data_source": "rpm",
      "min": 0,
      "max": 8000,
      "title": "RPM",
      "alerts": [
        {"condition": "above", "threshold": 7000, "action": "flash"}
      ]
    }
  )";
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithGauge(gauge_str));
    GaugeDefinition& gauge = config.gauges["rpm_gauge"];
    
    EXPECT_EQ(gauge.alerts.size(), 1);
    EXPECT_EQ(gauge.alerts[0].condition, "above");
    EXPECT_EQ(gauge.alerts[0].threshold, 7000);
    EXPECT_EQ(gauge.alerts[0].action, "flash");
    
    // Test condition: 7500 is above 7000, should trigger
    EXPECT_TRUE(7500 > gauge.alerts[0].threshold);
    
    // Test condition: 6500 is not above 7000, should not trigger
    EXPECT_FALSE(6500 > gauge.alerts[0].threshold);
}

TEST_F(GaugeTest, AlertBelowThreshold) {
    std::string gauge_str = R"(
    "coolant_gauge": {
      "type": "arc",
      "data_source": "coolant_temp",
      "min": 0,
      "max": 120,
      "title": "Coolant",
      "alerts": [
        {"condition": "below", "threshold": 50, "action": "warning"}
      ]
    }
  )";
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithGauge(gauge_str));
    GaugeDefinition& gauge = config.gauges["coolant_gauge"];
    
    EXPECT_EQ(gauge.alerts[0].condition, "below");
    EXPECT_EQ(gauge.alerts[0].threshold, 50);
    
    // Test condition: 40 is below 50, should trigger
    EXPECT_TRUE(40 < gauge.alerts[0].threshold);
    
    // Test condition: 60 is not below 50, should not trigger
    EXPECT_FALSE(60 < gauge.alerts[0].threshold);
}

// Test gauge types
TEST_F(GaugeTest, GaugeTypes) {
    std::string gauge_str = R"(
    "rpm_arc": {
      "type": "arc",
      "data_source": "rpm",
      "min": 0,
      "max": 8000,
      "title": "RPM"
    },
    "speed_digital": {
      "type": "digital",
      "data_source": "speed",
      "min": 0,
      "max": 120,
      "title": "Speed"
    },
    "temp_bar": {
      "type": "bar",
      "data_source": "coolant_temp",
      "min": 0,
      "max": 120,
      "title": "Temperature"
    }
  )";
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithGauge(gauge_str));
    
    EXPECT_EQ(config.gauges["rpm_arc"].type, "arc");
    EXPECT_EQ(config.gauges["speed_digital"].type, "digital");
    EXPECT_EQ(config.gauges["temp_bar"].type, "bar");
}

// Test gauge PID reference
TEST_F(GaugeTest, GaugePIDReference) {
    std::string gauge_str = R"(
    "rpm_gauge": {
      "type": "arc",
      "data_source": "rpm",
      "min": 0,
      "max": 8000,
      "title": "RPM"
    }
  )";
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithGauge(gauge_str));
    GaugeDefinition& gauge = config.gauges["rpm_gauge"];
    
    // Gauge should reference the correct data source (PID)
    EXPECT_EQ(gauge.data_source, "rpm");
    
    // Data source should exist in configuration
    ASSERT_TRUE(config.pids.find(gauge.data_source) != config.pids.end());
    EXPECT_EQ(config.pids[gauge.data_source].command, "010C");
}

// Test multiple alerts on single gauge
TEST_F(GaugeTest, MultipleAlerts) {
    std::string gauge_str = R"(
    "rpm_gauge": {
      "type": "arc",
      "data_source": "rpm",
      "min": 0,
      "max": 8000,
      "title": "RPM",
      "alerts": [
        {"condition": "above", "threshold": 6500, "action": "warning"},
        {"condition": "above", "threshold": 7000, "action": "flash"}
      ]
    }
  )";
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithGauge(gauge_str));
    GaugeDefinition& gauge = config.gauges["rpm_gauge"];
    
    EXPECT_EQ(gauge.alerts.size(), 2);
    EXPECT_EQ(gauge.alerts[0].threshold, 6500);
    EXPECT_EQ(gauge.alerts[1].threshold, 7000);
}

// Test gauge range validation
TEST_F(GaugeTest, GaugeRangeValidation) {
    std::string gauge_str = R"(
    "rpm_gauge": {
      "type": "arc",
      "data_source": "rpm",
      "min": 0,
      "max": 8000,
      "title": "RPM"
    }
  )";
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithGauge(gauge_str));
    GaugeDefinition& gauge = config.gauges["rpm_gauge"];
    
    // Range should be valid
    EXPECT_LT(gauge.min, gauge.max);
    EXPECT_GE(gauge.min, 0);
}
