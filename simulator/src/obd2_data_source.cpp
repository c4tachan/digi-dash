/**
 * @file obd2_data_source.cpp
 * @brief OBD II data source implementation
 */

#include "obd2_data_source.h"
#include "config.h"
#include <cstring>
#include <cstdio>

OBD2DataSource::OBD2DataSource(const char* port_name, const std::map<std::string, PidConfig>& pid_config)
    : serial_handle_(nullptr), rpm_(0), speed_(0), coolant_temp_(0), connected_(false) {
    
    strncpy(port_name_, port_name, sizeof(port_name_) - 1);
    port_name_[sizeof(port_name_) - 1] = '\0';
    
    // Extract PID commands from config
    if (pid_config.count("rpm")) {
        rpm_pid_ = pid_config.at("rpm").command;
    }
    if (pid_config.count("speed")) {
        speed_pid_ = pid_config.at("speed").command;
    }
    if (pid_config.count("coolant_temp") || pid_config.count("temp")) {
        // Support both "coolant_temp" and "temp" keys
        const auto& key = pid_config.count("coolant_temp") ? "coolant_temp" : "temp";
        coolant_temp_pid_ = pid_config.at(key).command;
    }
    
    // Build name string
    snprintf(name_buffer_, sizeof(name_buffer_), "OBD II (%s)", port_name_);
    
    printf("OBD2DataSource initialized with:\n");
    printf("  RPM PID: %s\n", rpm_pid_.c_str());
    printf("  Speed PID: %s\n", speed_pid_.c_str());
    printf("  CoolantTemp PID: %s\n", coolant_temp_pid_.c_str());
}

OBD2DataSource::~OBD2DataSource() {
    disconnect();
}

void OBD2DataSource::update() {
    // TODO: Implement actual OBD II communication
    // For now, just maintain connection state
    
    if (!connected_) {
        connect();
    }
    
    if (connected_) {
        // TODO: Query actual OBD II values using the configured PIDs
        // rpm_ = queryOBD2(rpm_pid_);
        // speed_ = queryOBD2(speed_pid_);
        // coolant_temp_ = queryOBD2(coolant_temp_pid_);
        
        fprintf(stderr, "OBD2: Connection established but data queries not yet implemented\n");
    }
}

bool OBD2DataSource::connect() {
    // TODO: Implement serial port connection using libserialport or similar
    fprintf(stderr, "OBD2: Attempting to connect to %s...\n", port_name_);
    connected_ = false;  // For now, always fails
    return false;
}

void OBD2DataSource::disconnect() {
    if (serial_handle_) {
        // TODO: Close serial port
        serial_handle_ = nullptr;
    }
    connected_ = false;
}

int OBD2DataSource::queryOBD2(const std::string& pid_command) {
    // TODO: Implement OBD II protocol
    // - Send "01 <pid>" command to adapter
    // - Parse response (e.g. "41 0C xx yy")
    // - Convert hex to decimal
    // - Return value or -1 on error
    (void)pid_command;  // Suppress unused warning
    return -1;
}

const char* OBD2DataSource::getName() const {
    return name_buffer_;
}

