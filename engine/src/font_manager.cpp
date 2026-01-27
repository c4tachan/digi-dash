#include "digidash/font_manager.h"

namespace digidash {

FontManager::FontManager() {}

FontManager::~FontManager() {}

bool FontManager::load_font(const std::string& font_name,
                            const std::string& filepath) {
    // TODO: Implement LVGL font loading
    return false;
}

bool FontManager::get_metrics(const std::string& font_name,
                               FontMetrics& metrics_out) {
    auto it = fonts_.find(font_name);
    if (it == fonts_.end()) {
        return false;
    }
    metrics_out = it->second;
    return true;
}

void FontManager::render_text(const std::string& font_name,
                               const std::string& text,
                               uint8_t* target_buffer, int x, int y,
                               int width, int height) {
    // TODO: Implement text rendering with LVGL fonts
}

} // namespace digidash
