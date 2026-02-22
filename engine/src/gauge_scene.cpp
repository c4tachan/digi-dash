#include "digidash/gauge_scene.h"
#include <cstring>
#include <algorithm>

#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char* GAUGE_TAG = "gauge_scene";
#endif

namespace digidash {

GaugeScene::GaugeScene()
    : renderer_(std::make_unique<VectorRenderer>()),
      animation_engine_(std::make_unique<AnimationEngine>()),
      pid_system_(std::make_unique<PIDBindingSystem>()),
      width_(0),
    height_(0),
    viewport_width_(0),
    viewport_height_(0) {}

GaugeScene::~GaugeScene() {}

bool GaugeScene::load_gauge(const BinaryGaugeLoader::GaugeAsset& asset) {
    current_asset_ = asset;
    width_ = asset.width;
    height_ = asset.height;
    
    // Convert Path structure to BezierPath for rendering
    paths_.clear();
    
    for (size_t i = 0; i < asset.paths.size(); ++i) {
        const auto& path = asset.paths[i];
        VectorRenderer::BezierPath bezier_path;
        
        // Convert color from Color struct to uint32
        uint32_t color = (path.stroke.color.a << 24) |
                        (path.stroke.color.r << 16) |
                        (path.stroke.color.g << 8) |
                        path.stroke.color.b;
        
        bezier_path.color = color;
        bezier_path.stroke_width = path.stroke.width;
        bezier_path.is_filled = path.fill.enabled;
        bezier_path.stroke_cap = path.stroke.cap;
        
        // If filled, use fill color instead
        if (path.fill.enabled) {
            bezier_path.color = (path.fill.color.a << 24) |
                               (path.fill.color.r << 16) |
                               (path.fill.color.g << 8) |
                               path.fill.color.b;
            #ifdef ESP_PLATFORM
            ESP_LOGI(GAUGE_TAG, "Path %d: FILLED RGBA(%d,%d,%d,%d)", 
                     (int)i, path.fill.color.r, path.fill.color.g, path.fill.color.b, path.fill.color.a);
            #endif
        } else {
            #ifdef ESP_PLATFORM
            ESP_LOGI(GAUGE_TAG, "Path %d: STROKED RGBA(%d,%d,%d,%d) width=%.1f", 
                     (int)i, path.stroke.color.r, path.stroke.color.g, path.stroke.color.b, path.stroke.color.a,
                     (double)path.stroke.width);
            #endif
        }
        
        // Flatten PathCommands to points for rendering
        float current_x = 0.0f, current_y = 0.0f;
        
        for (const auto& cmd : path.commands) {
            switch (cmd.type) {
                case PathCommand::Type::MoveTo: {
                    current_x = cmd.x1;
                    current_y = cmd.y1;
                    bezier_path.control_points.push_back({current_x, current_y});
                }
                break;
                    
                case PathCommand::Type::LineTo: {
                    current_x = cmd.x1;
                    current_y = cmd.y1;
                    bezier_path.control_points.push_back({current_x, current_y});
                }
                break;
                    
                case PathCommand::Type::CubicTo:
                    // Tessellate cubic bezier into line segments
                    {
                        const int segments = 50;  // Increased from 10 for better arc accuracy
                        float start_x = current_x;
                        float start_y = current_y;
                        
                        for (int i = 1; i <= segments; ++i) {
                            float t = i / (float)segments;
                            float t2 = t * t;
                            float t3 = t2 * t;
                            float mt = 1.0f - t;
                            float mt2 = mt * mt;
                            float mt3 = mt2 * mt;
                            
                            float x = mt3 * start_x + 
                                     3.0f * mt2 * t * cmd.x1 +
                                     3.0f * mt * t2 * cmd.x2 +
                                     t3 * cmd.x3;
                            float y = mt3 * start_y +
                                     3.0f * mt2 * t * cmd.y1 +
                                     3.0f * mt * t2 * cmd.y2 +
                                     t3 * cmd.y3;
                            
                            bezier_path.control_points.push_back({x, y});
                        }
                        
                        current_x = cmd.x3;
                        current_y = cmd.y3;
                    }
                    break;
                    
                case PathCommand::Type::Close:
                    // Close path by adding first point again if needed
                    if (!bezier_path.control_points.empty()) {
                        const auto& first = bezier_path.control_points[0];
                        bezier_path.control_points.push_back(first);
                    }
                    break;
            }
        }
        
        if (!bezier_path.control_points.empty()) {
            paths_.push_back(bezier_path);
        }
    }

    rebuild_transformed_paths();
    
    return true;
}

void GaugeScene::set_viewport(uint32_t viewport_width, uint32_t viewport_height) {
    viewport_width_ = viewport_width;
    viewport_height_ = viewport_height;
    rebuild_transformed_paths();
}

void GaugeScene::rebuild_transformed_paths() {
    transformed_paths_.clear();

    if (paths_.empty()) {
        return;
    }

    if (width_ == 0 || height_ == 0 || viewport_width_ == 0 || viewport_height_ == 0) {
        transformed_paths_ = paths_;
        return;
    }

    float min_x = 0.0f;
    float min_y = 0.0f;
    float max_x = static_cast<float>(width_);
    float max_y = static_cast<float>(height_);
    bool has_points = false;

    for (const auto& path : paths_) {
        for (const auto& point : path.control_points) {
            if (!has_points) {
                min_x = max_x = point.x;
                min_y = max_y = point.y;
                has_points = true;
            } else {
                min_x = std::min(min_x, point.x);
                min_y = std::min(min_y, point.y);
                max_x = std::max(max_x, point.x);
                max_y = std::max(max_y, point.y);
            }
        }
    }

    if (!has_points) {
        transformed_paths_ = paths_;
        return;
    }

    const float source_width = std::max(1.0f, max_x - min_x);
    const float source_height = std::max(1.0f, max_y - min_y);

    const float scale_x = static_cast<float>(viewport_width_) / source_width;
    const float scale_y = static_cast<float>(viewport_height_) / source_height;
    const float uniform_scale = std::min(scale_x, scale_y);

    const float draw_width = source_width * uniform_scale;
    const float draw_height = source_height * uniform_scale;
    const float offset_x = (static_cast<float>(viewport_width_) - draw_width) * 0.5f;
    const float offset_y = (static_cast<float>(viewport_height_) - draw_height) * 0.5f;

    transformed_paths_.reserve(paths_.size());
    for (const auto& path : paths_) {
        VectorRenderer::BezierPath transformed = path;
        for (auto& point : transformed.control_points) {
            point.x = (point.x - min_x) * uniform_scale + offset_x;
            point.y = (point.y - min_y) * uniform_scale + offset_y;
        }
        transformed_paths_.push_back(std::move(transformed));
    }
}

void GaugeScene::update(uint32_t delta_ms) {
    animation_engine_->update(delta_ms);
}

void GaugeScene::render(uint8_t* target_buffer, int width, int height,
                        int stride, int y_offset) {
    if (!renderer_ || transformed_paths_.empty()) return;
    
    // Render all paths with y_offset for tiled rendering
    for (const auto& path : transformed_paths_) {
        renderer_->render_path(path, target_buffer, width, height, stride, y_offset);
    }
}

void GaugeScene::set_pid_value(uint32_t pid_id, float value) {
    pid_system_->set_pid_value(pid_id, value);
}

} // namespace digidash
