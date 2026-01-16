#include <gtest/gtest.h>
#include "config.h"
#include <cstdio>
#include <cstdlib>

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test config file
        test_config = R"({
  "protocol": {
    "type": "CAN",
    "baudrate": 500000,
    "description": "Test CAN Protocol"
  },
  "units": {
    "system": "metric",
    "description": "Metric units test"
  },
  "pids": {
    "rpm": {
      "command": "010C",
      "formula": "((A*256)+B)/4",
      "unit": "RPM"
    }
  },
  "gauges": {
    "rpm_gauge": {
      "type": "arc",
      "title": "RPM",
      "data_source": "rpm",
      "min": 0,
      "max": 8000,
      "unit": "RPM",
      "visual": {
        "colors": {
          "background": "#1a1a1a",
          "arc_background": "#404040",
          "arc_indicator": "#FF6B00",
          "text": "#FFFFFF"
        },
        "arc_width": 15,
        "font_size": 24,
        "border_width": 0,
        "decimal_places": 0
      },
      "zones": [
        {"min": 0, "max": 6000, "color": "#00FF00", "label": "Safe"},
        {"min": 6000, "max": 7000, "color": "#FFAA00", "label": "Warning"},
        {"min": 7000, "max": 8000, "color": "#FF0000", "label": "Danger"}
      ],
      "alerts": [
        {
          "threshold": 7000,
          "condition": "above",
          "action": "flash",
          "flash_color": "#FF0000",
          "flash_rate": 200
        }
      ]
    }
  },
  "display": {
    "width": 480,
    "height": 320,
    "elements": [
      {
        "gauge_name": "rpm_gauge",
        "x": 0,
        "y": 0,
        "width": 480,
        "height": 320
      }
    ]
  }
})";
    }

    std::string test_config;
};

// Test FR-2.4.1: System shall load configuration from display.json
TEST_F(ConfigTest, LoadConfigurationFromString) {
    DashboardConfig config = DashboardConfig::loadFromString(test_config);
    EXPECT_FALSE(config.display.elements.empty());
    EXPECT_EQ(config.display.width, 480);
    EXPECT_EQ(config.display.height, 320);
}

// Test FR-2.4.2: Configuration shall specify protocol type and baudrate
TEST_F(ConfigTest, LoadProtocolConfiguration) {
    DashboardConfig config = DashboardConfig::loadFromString(test_config);
    EXPECT_EQ(config.protocol.type, "CAN");
    EXPECT_EQ(config.protocol.baudrate, 500000);
    EXPECT_EQ(config.protocol.description, "Test CAN Protocol");
}

// Test FR-2.4.6, 2.4.7, 2.4.8: Unit system configuration
TEST_F(ConfigTest, LoadUnitConfiguration) {
    DashboardConfig config = DashboardConfig::loadFromString(test_config);
    EXPECT_EQ(config.units.system, "metric");
    EXPECT_EQ(config.units.description, "Metric units test");
}

// Test FR-2.4.3: Configuration shall specify PID definitions
TEST_F(ConfigTest, LoadPIDDefinitions) {
    DashboardConfig config = DashboardConfig::loadFromString(test_config);
    EXPECT_EQ(config.pids.size(), 1);
    EXPECT_TRUE(config.pids.count("rpm") > 0);
    EXPECT_EQ(config.pids["rpm"].command, "010C");
    EXPECT_EQ(config.pids["rpm"].formula, "((A*256)+B)/4");
    EXPECT_EQ(config.pids["rpm"].unit, "RPM");
}

// Test FR-2.4.4: Configuration shall specify gauge definitions
TEST_F(ConfigTest, LoadGaugeDefinitions) {
    DashboardConfig config = DashboardConfig::loadFromString(test_config);
    EXPECT_EQ(config.gauges.size(), 1);
    EXPECT_TRUE(config.gauges.count("rpm_gauge") > 0);
    
    const auto& rpm_gauge = config.gauges["rpm_gauge"];
    EXPECT_EQ(rpm_gauge.type, "arc");
    EXPECT_EQ(rpm_gauge.title, "RPM");
    EXPECT_EQ(rpm_gauge.data_source, "rpm");
    EXPECT_EQ(rpm_gauge.min, 0);
    EXPECT_EQ(rpm_gauge.max, 8000);
}

// Test zone loading
TEST_F(ConfigTest, LoadZones) {
    DashboardConfig config = DashboardConfig::loadFromString(test_config);
    const auto& rpm_gauge = config.gauges["rpm_gauge"];
    
    EXPECT_EQ(rpm_gauge.zones.size(), 3);
    EXPECT_EQ(rpm_gauge.zones[0].min, 0);
    EXPECT_EQ(rpm_gauge.zones[0].max, 6000);
    EXPECT_EQ(rpm_gauge.zones[0].color, "#00FF00");
    
    EXPECT_EQ(rpm_gauge.zones[2].min, 7000);
    EXPECT_EQ(rpm_gauge.zones[2].max, 8000);
    EXPECT_EQ(rpm_gauge.zones[2].color, "#FF0000");
}

// Test alert loading (FR-2.3.4)
TEST_F(ConfigTest, LoadAlerts) {
    DashboardConfig config = DashboardConfig::loadFromString(test_config);
    const auto& rpm_gauge = config.gauges["rpm_gauge"];
    
    EXPECT_EQ(rpm_gauge.alerts.size(), 1);
    EXPECT_EQ(rpm_gauge.alerts[0].threshold, 7000);
    EXPECT_EQ(rpm_gauge.alerts[0].condition, "above");
    EXPECT_EQ(rpm_gauge.alerts[0].action, "flash");
}

// Test default protocol when not specified
TEST_F(ConfigTest, DefaultProtocol) {
    std::string config_without_protocol = R"({
  "pids": {},
  "gauges": {},
  "display": {"width": 480, "height": 320, "elements": []}
})";
    
    DashboardConfig config = DashboardConfig::loadFromString(config_without_protocol);
    EXPECT_EQ(config.protocol.type, "CAN");
    EXPECT_EQ(config.protocol.baudrate, 500000);
}

// Test default units when not specified
TEST_F(ConfigTest, DefaultUnits) {
    std::string config_without_units = R"({
  "pids": {},
  "gauges": {},
  "display": {"width": 480, "height": 320, "elements": []}
})";
    
    DashboardConfig config = DashboardConfig::loadFromString(config_without_units);
    EXPECT_EQ(config.units.system, "metric");
}

// Test display element parsing
TEST_F(ConfigTest, LoadDisplayElements) {
    DashboardConfig config = DashboardConfig::loadFromString(test_config);
    EXPECT_EQ(config.display.elements.size(), 1);
    EXPECT_EQ(config.display.elements[0].gauge_name, "rpm_gauge");
    EXPECT_EQ(config.display.elements[0].x, 0);
    EXPECT_EQ(config.display.elements[0].y, 0);
    EXPECT_EQ(config.display.elements[0].width, 480);
    EXPECT_EQ(config.display.elements[0].height, 320);
}
