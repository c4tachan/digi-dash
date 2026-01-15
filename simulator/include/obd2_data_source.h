/**
 * @file obd2_data_source.h
 * @brief OBD II data source for reading real vehicle data
 */

#ifndef OBD2_DATA_SOURCE_H
#define OBD2_DATA_SOURCE_H

#include "data_source.h"
#include <map>
#include <string>

struct PidConfig;

/**
 * @brief Real OBD II data source via ELM327 adapter
 * 
 * Communicates with an OBD II adapter (Bluetooth or USB) to read
 * actual vehicle sensor data using standard OBD II PIDs.
 * 
 * TODO: Implement serial communication and OBD II protocol
 */
class OBD2DataSource : public DataSource {
public:
    /**
     * @brief Create OBD II data source
     * @param port_name Serial port or Bluetooth device (e.g. "/dev/rfcomm0", "COM3")
     * @param pid_config Map of parameter names ("rpm", "speed", etc) to PID configurations
     */
    OBD2DataSource(const char* port_name, const std::map<std::string, PidConfig>& pid_config);
    ~OBD2DataSource();
    
    /**
     * @brief Connect to OBD II adapter and request latest data
     */
    void update() override;
    
    /**
     * @brief Get current RPM
     */
    int getRPM() const override { return rpm_; }
    
    /**
     * @brief Get current speed
     */
    int getSpeed() const override { return speed_; }
    
    /**
     * @brief Get current coolant temperature
     */
    int getCoolantTemp() const override { return coolant_temp_; }
    
    /**
     * @brief Check if OBD II adapter is connected
     */
    bool isConnected() const override { return connected_; }
    
    /**
     * @brief Get data source name with port info
     */
    const char* getName() const override;
    
private:
    char port_name_[256];
    char name_buffer_[256];
    void* serial_handle_;  // Opaque handle for serial port (platform-specific)
    
    int rpm_;
    int speed_;
    int coolant_temp_;
    bool connected_;
    
    // PID configuration stored from initialization
    std::string rpm_pid_;
    std::string speed_pid_;
    std::string coolant_temp_pid_;
    
    /**
     * @brief Connect to serial port
     */
    bool connect();
    
    /**
     * @brief Disconnect from serial port
     */
    void disconnect();
    
    /**
     * @brief Send OBD II command and parse response
     * @param pid Parameter ID (e.g. 0x0C for RPM)
     * @return Decoded value or -1 on error
     */
    int queryOBD2(const std::string& pid_command);
};

#endif // OBD2_DATA_SOURCE_H

