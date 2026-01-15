/**
 * @file obd2_data_source.cpp
 * @brief OBD II data source implementation
 */

#include "obd2_data_source.h"
#include <cstring>
#include <cstdio>

OBD2DataSource::OBD2DataSource(const char* port_name)
    : serial_handle_(nullptr), rpm_(0), speed_(0), coolant_temp_(0), connected_(false) {
    
    strncpy(port_name_, port_name, sizeof(port_name_) - 1);
    port_name_[sizeof(port_name_) - 1] = '\0';
    
    // Build name string
    snprintf(name_buffer_, sizeof(name_buffer_), "OBD II (%s)", port_name_);
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
        // TODO: Query actual OBD II values
        // rpm_ = queryOBD2(0x0C);      // Engine RPM
        // speed_ = queryOBD2(0x0D);    // Vehicle Speed
        // coolant_temp_ = queryOBD2(0x05);  // Coolant Temperature
        
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

int OBD2DataSource::queryOBD2(unsigned char pid) {
    // TODO: Implement OBD II protocol
    // - Send "01 <pid>" command to adapter
    // - Parse response (e.g. "41 0C xx yy")
    // - Convert hex to decimal
    // - Return value or -1 on error
    return -1;
}

const char* OBD2DataSource::getName() const {
    return name_buffer_;
}
