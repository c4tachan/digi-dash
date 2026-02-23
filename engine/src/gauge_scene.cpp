#include "digidash/gauge_scene.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <unordered_set>

#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char* GAUGE_TAG = "gauge_scene";
#endif
namespace digidash {

namespace {

uint32_t pid_id_for_name(const std::string& pid_name) {
    if (pid_name == "engine_rpm") {
        return 0;
    }
    if (pid_name == "vehicle_speed") {
        return 1;
    }
    if (pid_name == "throttle_position") {
        return 2;
    }
    if (pid_name == "coolant_temp") {
        return 3;
    }
    return UINT32_MAX;
}

bool should_reverse_for_pid(const std::string& pid_name) {
    return pid_name == "coolant_temp";
}

} // namespace

GaugeScene::GaugeScene()
    : renderer_(std::make_unique<VectorRenderer>()),
      animation_engine_(std::make_unique<AnimationEngine>()),
      pid_system_(std::make_unique<PIDBindingSystem>()),
            animation_time_ms_(0),
      width_(0),
            height_(0),
            viewport_width_(0),
            viewport_height_(0) {}

GaugeScene::~GaugeScene() {}

bool GaugeScene::load_gauge(const BinaryGaugeLoader::GaugeAsset& asset) {
    current_asset_ = asset;
    width_ = asset.width;
    height_ = asset.height;
    pid_name_to_id_.clear();
    seen_pid_ids_.clear();
    
    // Convert Path structure to BezierPath for rendering
    paths_.clear();
    path_ids_.clear();
    runtime_animations_.clear();
    
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
            path_ids_.push_back(path.id);
            paths_.push_back(bezier_path);
        }
    }

    std::unordered_set<size_t> animated_path_indices;

    for (const auto& path_animation : asset.path_animations) {
        if (path_animation.type != PathAnimationBinding::Type::TrimSweep) {
            continue;
        }

        bool assigned = false;
        for (size_t index = 0; index < path_ids_.size(); ++index) {
            if (path_ids_[index] == path_animation.path_id) {
                RuntimePathAnimation runtime_animation;
                runtime_animation.path_index = index;
                runtime_animation.min_value = path_animation.min_value;
                runtime_animation.max_value = path_animation.max_value;
                runtime_animation.pid_name = path_animation.pid_name;
                runtime_animation.pid_id = pid_id_for_name(path_animation.pid_name);
                runtime_animation.uses_pid = (runtime_animation.pid_id != UINT32_MAX);
                runtime_animation.reverse = should_reverse_for_pid(path_animation.pid_name);

                if (runtime_animation.uses_pid) {
                    pid_name_to_id_[runtime_animation.pid_name] = runtime_animation.pid_id;
                    PIDBindingSystem::PIDBinding binding{};
                    binding.pid_id = runtime_animation.pid_id;
                    binding.type = PIDBindingSystem::PIDType::CUSTOM;
                    binding.unit = "raw";
                    binding.min_value = runtime_animation.min_value;
                    binding.max_value = runtime_animation.max_value;
                    binding.scale = 1.0f;
                    binding.offset = 0.0f;
                    pid_system_->register_binding(binding);
                }

                runtime_animations_.push_back(std::move(runtime_animation));
                animated_path_indices.insert(index);
                assigned = true;
                break;
            }
        }

        if (assigned) {
            continue;
        }

        // Fallback: bind in order to stroked paths when IDs from sidecar JSON
        // don't match serialized ThorVG path IDs (e.g. path_0/path_1/...)
        auto select_color_match = [&](const std::string& pid_name) -> size_t {
            for (size_t index = 0; index < paths_.size(); ++index) {
                if (animated_path_indices.find(index) != animated_path_indices.end()) {
                    continue;
                }
                if (paths_[index].is_filled) {
                    continue;
                }

                const uint32_t color = paths_[index].color;
                const uint8_t red = static_cast<uint8_t>((color >> 16) & 0xFF);
                const uint8_t green = static_cast<uint8_t>((color >> 8) & 0xFF);
                const uint8_t blue = static_cast<uint8_t>(color & 0xFF);

                if (pid_name == "engine_rpm") {
                    if (red > blue && red > green) {
                        return index;
                    }
                } else if (pid_name == "coolant_temp") {
                    if (blue > red && blue > green) {
                        return index;
                    }
                }
            }
            return SIZE_MAX;
        };

        size_t matched_index = select_color_match(path_animation.pid_name);

        if (matched_index == SIZE_MAX) {
            for (size_t index = 0; index < paths_.size(); ++index) {
                if (animated_path_indices.find(index) != animated_path_indices.end()) {
                    continue;
                }
                if (paths_[index].is_filled) {
                    continue;
                }
                matched_index = index;
                break;
            }
        }

        if (matched_index != SIZE_MAX) {
            RuntimePathAnimation runtime_animation;
            runtime_animation.path_index = matched_index;
            runtime_animation.min_value = path_animation.min_value;
            runtime_animation.max_value = path_animation.max_value;
            runtime_animation.pid_name = path_animation.pid_name;
            runtime_animation.pid_id = pid_id_for_name(path_animation.pid_name);
            runtime_animation.uses_pid = (runtime_animation.pid_id != UINT32_MAX);
            runtime_animation.reverse = should_reverse_for_pid(path_animation.pid_name);

            if (runtime_animation.uses_pid) {
                pid_name_to_id_[runtime_animation.pid_name] = runtime_animation.pid_id;
                PIDBindingSystem::PIDBinding binding{};
                binding.pid_id = runtime_animation.pid_id;
                binding.type = PIDBindingSystem::PIDType::CUSTOM;
                binding.unit = "raw";
                binding.min_value = runtime_animation.min_value;
                binding.max_value = runtime_animation.max_value;
                binding.scale = 1.0f;
                binding.offset = 0.0f;
                pid_system_->register_binding(binding);
            }

            runtime_animations_.push_back(std::move(runtime_animation));
            animated_path_indices.insert(matched_index);
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
    animation_time_ms_ += delta_ms;
}

const GaugeScene::RuntimePathAnimation* GaugeScene::find_runtime_animation(size_t path_index) const {
    for (const auto& animation : runtime_animations_) {
        if (animation.path_index == path_index) {
            return &animation;
        }
    }
    return nullptr;
}

float GaugeScene::get_runtime_animation_value(const RuntimePathAnimation& animation) const {
    if (animation.uses_pid && seen_pid_ids_.find(animation.pid_id) != seen_pid_ids_.end()) {
        return pid_system_->get_pid_value(animation.pid_id);
    }

    const float min_value = animation.min_value;
    const float max_value = animation.max_value;
    if (max_value <= min_value) {
        return min_value;
    }

    const float seconds = static_cast<float>(animation_time_ms_) / 1000.0f;
    const float phase = static_cast<float>(animation.path_index) * 0.8f;
    const float normalized = 0.5f + 0.5f * static_cast<float>(std::sin(seconds * 0.9f + phase));
    return min_value + (max_value - min_value) * normalized;
}

VectorRenderer::BezierPath GaugeScene::trim_path_by_ratio(const VectorRenderer::BezierPath& path, float ratio, bool reverse) const {
    VectorRenderer::BezierPath trimmed = path;
    trimmed.control_points.clear();

    std::vector<VectorRenderer::Point> points = path.control_points;
    if (reverse) {
        std::reverse(points.begin(), points.end());
    }

    if (points.empty()) {
        return trimmed;
    }

    if (ratio <= 0.0f) {
        trimmed.control_points.push_back(points.front());
        return trimmed;
    }

    if (ratio >= 1.0f || points.size() < 2) {
        trimmed.control_points = std::move(points);
        return trimmed;
    }

    float total_length = 0.0f;
    for (size_t i = 1; i < points.size(); ++i) {
        const float dx = points[i].x - points[i - 1].x;
        const float dy = points[i].y - points[i - 1].y;
        total_length += std::sqrt(dx * dx + dy * dy);
    }

    if (total_length <= 0.0f) {
        trimmed.control_points.push_back(points.front());
        return trimmed;
    }

    const float target_length = total_length * std::clamp(ratio, 0.0f, 1.0f);
    float accumulated = 0.0f;
    trimmed.control_points.push_back(points.front());

    for (size_t i = 1; i < points.size(); ++i) {
        const auto& prev = points[i - 1];
        const auto& curr = points[i];
        const float dx = curr.x - prev.x;
        const float dy = curr.y - prev.y;
        const float seg_length = std::sqrt(dx * dx + dy * dy);

        if (seg_length <= 0.0f) {
            continue;
        }

        if (accumulated + seg_length < target_length) {
            trimmed.control_points.push_back(curr);
            accumulated += seg_length;
            continue;
        }

        const float remain = target_length - accumulated;
        const float t = std::clamp(remain / seg_length, 0.0f, 1.0f);
        VectorRenderer::Point cut_point;
        cut_point.x = prev.x + dx * t;
        cut_point.y = prev.y + dy * t;
        trimmed.control_points.push_back(cut_point);
        break;
    }

    return trimmed;
}

void GaugeScene::render(uint8_t* target_buffer, int width, int height,
                        int stride, int y_offset) {
    if (!renderer_ || transformed_paths_.empty()) return;
    
    // Render all paths with y_offset for tiled rendering
    for (size_t index = 0; index < transformed_paths_.size(); ++index) {
        const auto* animation = find_runtime_animation(index);
        const auto& path = transformed_paths_[index];

        if (!animation || path.is_filled) {
            renderer_->render_path(path, target_buffer, width, height, stride, y_offset);
            continue;
        }

        const float value = get_runtime_animation_value(*animation);
        const float range = std::max(0.0001f, animation->max_value - animation->min_value);
        const float ratio = std::clamp((value - animation->min_value) / range, 0.0f, 1.0f);
        VectorRenderer::BezierPath trimmed = trim_path_by_ratio(path, ratio, animation->reverse);
        if (trimmed.control_points.size() >= 2) {
            renderer_->render_path(trimmed, target_buffer, width, height, stride, y_offset);
        }
    }
}

void GaugeScene::set_pid_value(uint32_t pid_id, float value) {
    seen_pid_ids_.insert(pid_id);
    pid_system_->set_pid_value(pid_id, value);
}

} // namespace digidash
