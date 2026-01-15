/**
 * @file data_source.h
 * @brief Abstract interface for vehicle data sources (mock or real OBD II)
 */

#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

/**
 * @brief Abstract base class for vehicle data sources
 * Provides an interface for reading vehicle parameters.
 * Implementations can be mock data, OBD II real data, or file-based data.
 */
class DataSource {
public:
    virtual ~DataSource() = default;
    
    /**
     * @brief Update/fetch latest data from source
     * Should be called periodically in the main loop
     */
    virtual void update() = 0;
    
    /**
     * @brief Get current RPM value
     * @return RPM value (0-8000 range typical)
     */
    virtual int getRPM() const = 0;
    
    /**
     * @brief Get current speed
     * @return Speed in km/h or mph depending on implementation
     */
    virtual int getSpeed() const = 0;
    
    /**
     * @brief Get coolant temperature
     * @return Temperature in Celsius
     */
    virtual int getCoolantTemp() const = 0;
    
    /**
     * @brief Check if data source is connected/available
     * @return true if actively providing data, false if disconnected
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief Get human-readable name of data source
     * @return String like "Mock Data" or "OBD II (Bluetooth)"
     */
    virtual const char* getName() const = 0;
};

#endif // DATA_SOURCE_H
