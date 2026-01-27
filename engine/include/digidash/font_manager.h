#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace digidash {

/**
 * @brief Manages bitmap fonts for text rendering
 * 
 * Loads preprocessed LVGL bitmap fonts and provides text rendering capabilities.
 * Supports dynamic text in gauge assets.
 */
class FontManager {
public:
    struct FontMetrics {
        uint32_t height;
        uint32_t line_spacing;
        bool monospace;
    };

    FontManager();
    ~FontManager();

    /**
     * @brief Load a bitmap font from file
     */
    bool load_font(const std::string& font_name, const std::string& filepath);

    /**
     * @brief Get metrics for a loaded font
     */
    bool get_metrics(const std::string& font_name, FontMetrics& metrics_out);

    /**
     * @brief Render text with a specific font
     */
    void render_text(const std::string& font_name, const std::string& text,
                     uint8_t* target_buffer, int x, int y, int width, int height);

private:
    std::unordered_map<std::string, FontMetrics> fonts_;
    std::unordered_map<std::string, std::vector<uint8_t>> font_data_;
};

} // namespace digidash
