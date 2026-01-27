#pragma once

#include <string>
#include <cstdint>

namespace digidash {

/**
 * @brief Abstract storage interface
 * 
 * For ESP32: Flash memory
 * For simulator: Filesystem
 */
class PlatformStorage {
public:
    virtual ~PlatformStorage() = default;

    /**
     * @brief Read a file completely
     */
    virtual bool read_file(const std::string& path, uint8_t* buffer, 
                          size_t max_size, size_t& bytes_read_out) = 0;

    /**
     * @brief Write a file
     */
    virtual bool write_file(const std::string& path, const uint8_t* buffer, 
                           size_t size) = 0;

    /**
     * @brief Check if file exists
     */
    virtual bool file_exists(const std::string& path) = 0;

    /**
     * @brief Delete a file
     */
    virtual bool delete_file(const std::string& path) = 0;

    /**
     * @brief Get available storage space
     */
    virtual size_t get_available_space() = 0;
};

} // namespace digidash
