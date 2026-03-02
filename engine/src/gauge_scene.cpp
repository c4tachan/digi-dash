#include "digidash/gauge_scene.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <unordered_set>

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
    (void)pid_name;
    return false;
}

float distance_to_line(float px, float py, float x0, float y0, float x1, float y1) {
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float len_sq = dx * dx + dy * dy;
    if (len_sq < 1e-6f) {
        const float ddx = px - x0;
        const float ddy = py - y0;
        return std::sqrt(ddx * ddx + ddy * ddy);
    }

    const float t = ((px - x0) * dx + (py - y0) * dy) / len_sq;
    const float proj_x = x0 + t * dx;
    const float proj_y = y0 + t * dy;
    const float ddx = px - proj_x;
    const float ddy = py - proj_y;
    return std::sqrt(ddx * ddx + ddy * ddy);
}

int estimate_cubic_segments(float x0, float y0,
                            float x1, float y1,
                            float x2, float y2,
                            float x3, float y3) {
    const float chord = std::hypot(x3 - x0, y3 - y0);
    const float control_net = std::hypot(x1 - x0, y1 - y0)
                            + std::hypot(x2 - x1, y2 - y1)
                            + std::hypot(x3 - x2, y3 - y2);

    const float flatness = std::max(
        distance_to_line(x1, y1, x0, y0, x3, y3),
        distance_to_line(x2, y2, x0, y0, x3, y3)
    );

    const float by_length = std::max(chord, control_net) / 4.0f;
    const float by_flatness = flatness * 1.5f;
    const int segments = static_cast<int>(std::ceil(std::max(by_length, by_flatness)));
    return std::clamp(segments, 12, 128);
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
        viewport_height_(0) {
    if (renderer_) renderer_->set_quality(2);
}

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
                        float start_x = current_x;
                        float start_y = current_y;
                        const int segments = estimate_cubic_segments(
                            start_x, start_y,
                            cmd.x1, cmd.y1,
                            cmd.x2, cmd.y2,
                            cmd.x3, cmd.y3
                        );
                        
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

    rebuild_animation_lookup();
    rebuild_transformed_paths();
    prepare_frame_paths();
    
    return true;
}

void GaugeScene::set_viewport(uint32_t viewport_width, uint32_t viewport_height) {
    viewport_width_ = viewport_width;
    viewport_height_ = viewport_height;
    rebuild_transformed_paths();
    prepare_frame_paths();
}

void GaugeScene::rebuild_transformed_paths() {
    transformed_paths_.clear();
    transformed_min_y_.clear();
    transformed_max_y_.clear();
    prepared_paths_.clear();
    prepared_min_y_.clear();
    prepared_max_y_.clear();

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

    compute_path_y_bounds(transformed_paths_, transformed_min_y_, transformed_max_y_);
}

void GaugeScene::update(uint32_t delta_ms) {
    animation_engine_->update(delta_ms);
    animation_time_ms_ += delta_ms;
    prepare_frame_paths();
}

void GaugeScene::rebuild_animation_lookup() {
    animation_index_by_path_.assign(paths_.size(), -1);
    for (size_t index = 0; index < runtime_animations_.size(); ++index) {
        const size_t path_index = runtime_animations_[index].path_index;
        if (path_index < animation_index_by_path_.size()) {
            animation_index_by_path_[path_index] = static_cast<int>(index);
        }
    }
}

void GaugeScene::compute_path_y_bounds(const std::vector<VectorRenderer::BezierPath>& paths,
                                       std::vector<float>& min_y,
                                       std::vector<float>& max_y) const {
    min_y.assign(paths.size(), 0.0f);
    max_y.assign(paths.size(), 0.0f);

    for (size_t index = 0; index < paths.size(); ++index) {
        const auto& path = paths[index];
        if (path.control_points.empty()) {
            continue;
        }

        float p_min_y = path.control_points[0].y;
        float p_max_y = path.control_points[0].y;
        for (const auto& point : path.control_points) {
            p_min_y = std::min(p_min_y, point.y);
            p_max_y = std::max(p_max_y, point.y);
        }

        float margin = 0.0f;
        if (!path.is_filled) {
            // Expand bounds for stroke radius + antialiasing fringe so tile culling
            // does not miss edge pixels near tile boundaries.
            margin = (path.stroke_width * 0.5f) + 2.0f;
        }

        min_y[index] = p_min_y - margin;
        max_y[index] = p_max_y + margin;
    }
}

void GaugeScene::prepare_frame_paths() {
    if (transformed_paths_.empty()) {
        prepared_paths_.clear();
        prepared_min_y_.clear();
        prepared_max_y_.clear();
        return;
    }

    prepared_paths_.resize(transformed_paths_.size());
    for (size_t index = 0; index < transformed_paths_.size(); ++index) {
        const auto& path = transformed_paths_[index];
        int animation_index = (index < animation_index_by_path_.size()) ? animation_index_by_path_[index] : -1;

        if (animation_index < 0 || path.is_filled) {
            prepared_paths_[index] = path;
            continue;
        }

        const auto& animation = runtime_animations_[static_cast<size_t>(animation_index)];
        const float value = get_runtime_animation_value(animation);
        const float range = std::max(0.0001f, animation.max_value - animation.min_value);
        const float ratio = std::clamp((value - animation.min_value) / range, 0.0f, 1.0f);
        prepared_paths_[index] = trim_path_by_ratio(path, ratio, animation.reverse);
    }

    compute_path_y_bounds(prepared_paths_, prepared_min_y_, prepared_max_y_);
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

    return min_value;
}

VectorRenderer::BezierPath GaugeScene::trim_path_by_ratio(const VectorRenderer::BezierPath& path, float ratio, bool reverse) const {
    VectorRenderer::BezierPath trimmed = path;
    trimmed.control_points.clear();

    std::vector<VectorRenderer::Point> points = path.control_points;

    // For trim-sweep on stroked paths, treat closed polylines as open by
    // removing the duplicated closing point. Otherwise trim can wrap onto the
    // closing segment and create detached artifacts.
    if (!path.is_filled && points.size() >= 3) {
        const auto& first = points.front();
        const auto& last = points.back();
        const float dx = last.x - first.x;
        const float dy = last.y - first.y;
        if ((dx * dx + dy * dy) <= 1e-4f) {
            points.pop_back();
        }
    }

    // Drop consecutive duplicates to avoid zero-length segments that can
    // become isolated round-cap dots during trimming.
    if (!points.empty()) {
        std::vector<VectorRenderer::Point> cleaned;
        cleaned.reserve(points.size());
        cleaned.push_back(points.front());
        for (size_t i = 1; i < points.size(); ++i) {
            const float dx = points[i].x - cleaned.back().x;
            const float dy = points[i].y - cleaned.back().y;
            if ((dx * dx + dy * dy) > 1e-6f) {
                cleaned.push_back(points[i]);
            }
        }
        points = std::move(cleaned);
    }

    if (reverse) {
        std::reverse(points.begin(), points.end());
    }

    if (points.empty()) {
        return trimmed;
    }

    if (ratio <= 0.0f) {
        return trimmed;
    }

    if (ratio >= 1.0f) {
        trimmed.control_points = std::move(points);
        return trimmed;
    }

    if (points.size() < 2) {
        return trimmed;
    }

    float total_length = 0.0f;
    for (size_t i = 1; i < points.size(); ++i) {
        const float dx = points[i].x - points[i - 1].x;
        const float dy = points[i].y - points[i - 1].y;
        total_length += std::sqrt(dx * dx + dy * dy);
    }

    if (total_length <= 0.0f) {
        return trimmed;
    }

    const float target_length = total_length * std::clamp(ratio, 0.0f, 1.0f);
    if (target_length <= 0.5f) {
        return trimmed;
    }
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
    render_path_set(target_buffer, width, height, stride, y_offset, true, true);
}

void GaugeScene::render_static(uint8_t* target_buffer, int width, int height,
                               int stride, int y_offset) {
    render_path_set(target_buffer, width, height, stride, y_offset, true, false);
}

void GaugeScene::render_dynamic(uint8_t* target_buffer, int width, int height,
                                int stride, int y_offset) {
    render_path_set(target_buffer, width, height, stride, y_offset, false, true);
}

void GaugeScene::render_path_set(uint8_t* target_buffer, int width, int height,
                                 int stride, int y_offset,
                                 bool render_static_paths,
                                 bool render_dynamic_paths) {
    if (!renderer_ || transformed_paths_.empty()) return;
    if (render_dynamic_paths && prepared_paths_.empty()) {
        prepare_frame_paths();
        if (prepared_paths_.empty()) {
            return;
        }
    }

    const float tile_min_y = static_cast<float>(y_offset);
    const float tile_max_y = static_cast<float>(y_offset + height - 1);
    
    // Render only paths intersecting current tile
    for (size_t index = 0; index < prepared_paths_.size(); ++index) {
        if (index < prepared_min_y_.size() && index < prepared_max_y_.size()) {
            if (prepared_max_y_[index] < tile_min_y || prepared_min_y_[index] > tile_max_y) {
                continue;
            }
        }

        const bool has_animation = (index < animation_index_by_path_.size()) && (animation_index_by_path_[index] >= 0);
        const bool is_dynamic_path = has_animation;
        if ((is_dynamic_path && !render_dynamic_paths) || (!is_dynamic_path && !render_static_paths)) {
            continue;
        }

        const auto& path = is_dynamic_path ? prepared_paths_[index] : transformed_paths_[index];
        if (path.control_points.empty()) {
            continue;
        }

        if (!path.is_filled && path.control_points.size() < 2) {
            continue;
        }

        renderer_->render_path(path, target_buffer, width, height, stride, y_offset);
    }
}

bool GaugeScene::has_dynamic_in_region(int y_offset, int height) const {
    if (transformed_paths_.empty()) return false;
    if (prepared_paths_.empty()) {
        const_cast<GaugeScene*>(this)->prepare_frame_paths();
        if (prepared_paths_.empty()) return false;
    }
    const float region_min = static_cast<float>(y_offset);
    const float region_max = static_cast<float>(y_offset + height - 1);
    for (size_t index = 0; index < prepared_paths_.size(); ++index) {
        bool has_animation = (index < animation_index_by_path_.size()) && (animation_index_by_path_[index] >= 0);
        if (!has_animation) continue;
        if (index < prepared_min_y_.size() && index < prepared_max_y_.size()) {
            if (prepared_max_y_[index] < region_min || prepared_min_y_[index] > region_max) {
                continue;
            }
            return true;
        }
    }
    return false;
}

void GaugeScene::set_pid_value(uint32_t pid_id, float value) {
    seen_pid_ids_.insert(pid_id);
    pid_system_->set_pid_value(pid_id, value);
}

void GaugeScene::set_render_quality(int quality_level) {
    if (renderer_) {
        renderer_->set_quality(quality_level);
    }
}

} // namespace digidash
