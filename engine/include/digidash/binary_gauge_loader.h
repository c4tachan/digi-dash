#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace digidash {

/**
 * @brief Loads binary gauge asset files
 * 
 * Binary gauge files contain:
 * - Vector path data (Bézier curves)
 * - Animation definitions
 * - PID bindings
 * - Metadata
 */
class BinaryGaugeLoader {
public:
    struct GaugeAsset {
        std::string name;
        uint32_t width;
        uint32_t height;
        std::vector<uint8_t> path_data;
        std::vector<uint8_t> animation_data;
        std::vector<uint8_t> pid_binding_data;
    };

    BinaryGaugeLoader();
    ~BinaryGaugeLoader();

    /**
     * @brief Load a gauge from a binary file
     */
    bool load_from_file(const std::string& filepath, GaugeAsset& asset_out);

    /**
     * @brief Load a gauge from memory buffer
     */
    bool load_from_buffer(const uint8_t* buffer, size_t buffer_size, 
                          GaugeAsset& asset_out);

    /**
     * @brief Validate gauge asset integrity
     */
    bool validate_asset(const GaugeAsset& asset);

private:
    bool parse_header(const uint8_t* buffer, size_t buffer_size, 
                      uint32_t& width_out, uint32_t& height_out);
};

} // namespace digidash
