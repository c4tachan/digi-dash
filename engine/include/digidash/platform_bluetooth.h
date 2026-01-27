#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace digidash {

/**
 * @brief Abstract Bluetooth interface
 * 
 * For receiving OBD2 data and gauge files
 */
class PlatformBluetooth {
public:
    struct BluetoothDevice {
        std::string address;
        std::string name;
        int rssi;
    };

    virtual ~PlatformBluetooth() = default;

    /**
     * @brief Initialize Bluetooth
     */
    virtual bool init() = 0;

    /**
     * @brief Start device discovery
     */
    virtual bool start_scan() = 0;

    /**
     * @brief Stop device discovery
     */
    virtual void stop_scan() = 0;

    /**
     * @brief Get discovered devices
     */
    virtual std::vector<BluetoothDevice> get_discovered_devices() = 0;

    /**
     * @brief Connect to a device by address
     */
    virtual bool connect(const std::string& device_address) = 0;

    /**
     * @brief Disconnect from current device
     */
    virtual void disconnect() = 0;

    /**
     * @brief Send data
     */
    virtual bool send_data(const uint8_t* data, size_t size) = 0;

    /**
     * @brief Receive data (non-blocking)
     * @return bytes received, 0 if none available
     */
    virtual size_t receive_data(uint8_t* buffer, size_t max_size) = 0;

    /**
     * @brief Check if connected
     */
    virtual bool is_connected() = 0;
};

} // namespace digidash
