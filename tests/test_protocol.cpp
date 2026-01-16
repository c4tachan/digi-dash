#include <gtest/gtest.h>
#include "config.h"

class ProtocolTest : public ::testing::Test {
protected:
    std::string createConfigWithProtocol(const std::string& type, int baudrate = 500000) {
        return "{\"protocol\":{\"type\":\"" + type + "\",\"baudrate\":" + std::to_string(baudrate) + ",\"description\":\"Test protocol\"},\"units\":{\"system\":\"metric\"},\"pids\":{},\"gauges\":{},\"display\":{\"width\":480,\"height\":320,\"elements\":[]}}";
    }
};

// Test FR-2.1.1: System shall support CAN protocol
TEST_F(ProtocolTest, LoadCANProtocol) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithProtocol("CAN"));
    EXPECT_EQ(config.protocol.type, "CAN");
    EXPECT_EQ(config.protocol.baudrate, 500000);
}

// Test FR-2.1.2: System shall support KWP2000 protocol for older vehicles
TEST_F(ProtocolTest, LoadKWP2000Protocol) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithProtocol("KWP2000", 10400));
    EXPECT_EQ(config.protocol.type, "KWP2000");
    EXPECT_EQ(config.protocol.baudrate, 10400);
}

// Test FR-2.1.3: System shall support ISO 9141-2 protocol (legacy)
TEST_F(ProtocolTest, LoadISO9141Protocol) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithProtocol("ISO9141", 10400));
    EXPECT_EQ(config.protocol.type, "ISO9141");
    EXPECT_EQ(config.protocol.baudrate, 10400);
}

// Test FR-2.1.4: System shall support configurable baud rates
TEST_F(ProtocolTest, ConfigurableCANBaudrate) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithProtocol("CAN", 250000));
    EXPECT_EQ(config.protocol.type, "CAN");
    EXPECT_EQ(config.protocol.baudrate, 250000);
}

TEST_F(ProtocolTest, StandardCANBaudrate) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithProtocol("CAN", 500000));
    EXPECT_EQ(config.protocol.baudrate, 500000);
}

TEST_F(ProtocolTest, HighSpeedCANBaudrate) {
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithProtocol("CAN", 1000000));
    EXPECT_EQ(config.protocol.baudrate, 1000000);
}

// Test default protocol
TEST_F(ProtocolTest, DefaultProtocol) {
    std::string config_no_protocol = R"({
  "units": {"system": "metric"},
  "pids": {},
  "gauges": {},
  "display": {"width": 480, "height": 320, "elements": []}
})";
    DashboardConfig config = DashboardConfig::loadFromString(config_no_protocol);
    EXPECT_EQ(config.protocol.type, "CAN");
    EXPECT_EQ(config.protocol.baudrate, 500000);
}

// Test protocol description field
TEST_F(ProtocolTest, ProtocolDescription) {
    std::string config_str = R"({
  "protocol": {
    "type": "CAN",
    "baudrate": 500000,
    "description": "Controller Area Network for 2012 Mazda 2"
  },
  "units": {"system": "metric"},
  "pids": {},
  "gauges": {},
  "display": {"width": 480, "height": 320, "elements": []}
})";
    DashboardConfig config = DashboardConfig::loadFromString(config_str);
    EXPECT_EQ(config.protocol.description, "Controller Area Network for 2012 Mazda 2");
}

// Test multiple protocol types available
TEST_F(ProtocolTest, MultipleProtocolSupport) {
    // Test that we can load different protocols independently
    DashboardConfig can_config = DashboardConfig::loadFromString(createConfigWithProtocol("CAN", 500000));
    DashboardConfig kwp_config = DashboardConfig::loadFromString(createConfigWithProtocol("KWP2000", 10400));
    DashboardConfig iso_config = DashboardConfig::loadFromString(createConfigWithProtocol("ISO9141", 10400));
    
    EXPECT_NE(can_config.protocol.type, kwp_config.protocol.type);
    EXPECT_NE(kwp_config.protocol.type, iso_config.protocol.type);
    EXPECT_EQ(kwp_config.protocol.baudrate, iso_config.protocol.baudrate);
}

// Test protocol baudrate validation (realistic ranges)
TEST_F(ProtocolTest, KWP2000StandardBaudrate) {
    // KWP2000 typically uses 10.4 kbaud for diagnostic communication
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithProtocol("KWP2000", 10400));
    EXPECT_EQ(config.protocol.baudrate, 10400);
}

TEST_F(ProtocolTest, LegacyISO9141StandardBaudrate) {
    // ISO 9141-2 also typically uses 10.4 kbaud
    DashboardConfig config = DashboardConfig::loadFromString(createConfigWithProtocol("ISO9141", 10400));
    EXPECT_EQ(config.protocol.baudrate, 10400);
}

// Test that protocol configuration is non-null
TEST_F(ProtocolTest, ProtocolAlwaysConfigured) {
    std::string config_str = R"({
  "protocol": {"type": "CAN", "baudrate": 500000},
  "units": {"system": "metric"},
  "pids": {},
  "gauges": {},
  "display": {"width": 480, "height": 320, "elements": []}
})";
    DashboardConfig config = DashboardConfig::loadFromString(config_str);
    // Protocol should exist and be non-empty
    EXPECT_FALSE(config.protocol.type.empty());
    EXPECT_GT(config.protocol.baudrate, 0);
}
