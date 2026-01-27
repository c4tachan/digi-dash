#pragma once

#include "types.h"
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace digidash {

// Path command types matching the preprocessor
struct PathCommand {
    enum class Type : uint8_t { MoveTo = 0, LineTo = 1, CubicTo = 2, Close = 3 };
    Type type;
    float x1, y1;
    float x2, y2;
    float x3, y3;
};

struct StrokeStyle {
    float width;
    Color color;
    StrokeLineCap cap;
};

struct FillStyle {
    bool enabled;
    Color color;
};

struct Path {
    std::string id;
    StrokeStyle stroke;
    FillStyle fill;
    std::vector<PathCommand> commands;
};

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
        std::vector<Path> paths;
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
};

} // namespace digidash
