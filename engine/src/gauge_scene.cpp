#include "digidash/gauge_scene.h"
#include <cstring>

namespace digidash {

GaugeScene::GaugeScene()
    : renderer_(std::make_unique<VectorRenderer>()),
      animation_engine_(std::make_unique<AnimationEngine>()),
      pid_system_(std::make_unique<PIDBindingSystem>()),
      width_(0),
      height_(0) {}

GaugeScene::~GaugeScene() {}

bool GaugeScene::load_gauge(const BinaryGaugeLoader::GaugeAsset& asset) {
    current_asset_ = asset;
    width_ = asset.width;
    height_ = asset.height;
    
    // Parse path data from asset
    size_t offset = 0;
    paths_.clear();
    
    while (offset < asset.path_data.size()) {
        if (offset + 10 > asset.path_data.size()) break;
        
        // Read path header
        uint16_t point_count = (asset.path_data[offset] |
                                (asset.path_data[offset + 1] << 8));
        offset += 2;
        
        uint32_t color = (asset.path_data[offset] |
                         (asset.path_data[offset + 1] << 8) |
                         (asset.path_data[offset + 2] << 16) |
                         (asset.path_data[offset + 3] << 24));
        offset += 4;
        
        uint32_t stroke_width_bits = (asset.path_data[offset] |
                                     (asset.path_data[offset + 1] << 8) |
                                     (asset.path_data[offset + 2] << 16) |
                                     (asset.path_data[offset + 3] << 24));
        float stroke_width;
        std::memcpy(&stroke_width, &stroke_width_bits, sizeof(float));
        offset += 4;
        
        uint8_t is_filled = asset.path_data[offset];
        offset += 1;
        offset += 3;  // Skip padding
        
        // Read points
        VectorRenderer::BezierPath path;
        path.color = color;
        path.stroke_width = stroke_width;
        path.is_filled = is_filled != 0;
        
        for (int i = 0; i < point_count; ++i) {
            if (offset + 8 > asset.path_data.size()) break;
            
            uint32_t x_bits = (asset.path_data[offset] |
                              (asset.path_data[offset + 1] << 8) |
                              (asset.path_data[offset + 2] << 16) |
                              (asset.path_data[offset + 3] << 24));
            float x;
            std::memcpy(&x, &x_bits, sizeof(float));
            offset += 4;
            
            uint32_t y_bits = (asset.path_data[offset] |
                              (asset.path_data[offset + 1] << 8) |
                              (asset.path_data[offset + 2] << 16) |
                              (asset.path_data[offset + 3] << 24));
            float y;
            std::memcpy(&y, &y_bits, sizeof(float));
            offset += 4;
            
            path.control_points.push_back({x, y});
        }
        
        if (!path.control_points.empty()) {
            paths_.push_back(path);
        }
    }
    
    return true;
}

void GaugeScene::update(uint32_t delta_ms) {
    animation_engine_->update(delta_ms);
}

void GaugeScene::render(uint8_t* target_buffer, int width, int height,
                        int stride) {
    if (!renderer_ || paths_.empty()) return;
    
    // Render all paths
    for (const auto& path : paths_) {
        renderer_->render_path(path, target_buffer, width, height, stride);
    }
}

void GaugeScene::set_pid_value(uint32_t pid_id, float value) {
    pid_system_->set_pid_value(pid_id, value);
}

} // namespace digidash
