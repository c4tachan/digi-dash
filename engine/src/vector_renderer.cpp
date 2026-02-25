#include "digidash/vector_renderer.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <utility>
#include <cstdio>

namespace digidash {

namespace {
inline void blend_pixel_src_over(uint8_t* px, uint8_t src_r, uint8_t src_g, uint8_t src_b, uint8_t src_a) {
    if (src_a == 0) {
        return;
    }
    if (src_a == 255) {
        px[0] = src_r;
        px[1] = src_g;
        px[2] = src_b;
        px[3] = 255;
        return;
    }

    const uint32_t dst_a = px[3];
    const uint32_t inv_src_a = 255u - src_a;
    const uint32_t out_a = src_a + ((dst_a * inv_src_a + 127u) / 255u);
    if (out_a == 0) {
        px[0] = 0;
        px[1] = 0;
        px[2] = 0;
        px[3] = 0;
        return;
    }

    const uint32_t src_r_p = src_r * src_a;
    const uint32_t src_g_p = src_g * src_a;
    const uint32_t src_b_p = src_b * src_a;

    const uint32_t dst_r_p = px[0] * dst_a;
    const uint32_t dst_g_p = px[1] * dst_a;
    const uint32_t dst_b_p = px[2] * dst_a;

    const uint32_t out_r_p = src_r_p + ((dst_r_p * inv_src_a + 127u) / 255u);
    const uint32_t out_g_p = src_g_p + ((dst_g_p * inv_src_a + 127u) / 255u);
    const uint32_t out_b_p = src_b_p + ((dst_b_p * inv_src_a + 127u) / 255u);

    px[0] = static_cast<uint8_t>((out_r_p + (out_a / 2u)) / out_a);
    px[1] = static_cast<uint8_t>((out_g_p + (out_a / 2u)) / out_a);
    px[2] = static_cast<uint8_t>((out_b_p + (out_a / 2u)) / out_a);
    px[3] = static_cast<uint8_t>(out_a);
}
}

VectorRenderer::VectorRenderer() : quality_level_(2) {}

VectorRenderer::~VectorRenderer() {}

void VectorRenderer::render_path(const BezierPath& path, uint8_t* target_buffer,
                                 int width, int height, int stride, int y_offset) {
    if (!target_buffer || path.control_points.empty()) {
        return;
    }
    
    // Convert color from uint32_t to RGBA components
    uint32_t color = path.color;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = (color >> 0) & 0xFF;
    uint8_t a = (color >> 24) & 0xFF;
    
    #ifndef ESP_PLATFORM
    // Simulator uses BGR pixel order, swap R and B
    std::swap(r, b);
    #endif
    
    if (path.is_filled) {
        // Draw filled shape - simple polygon fill
        draw_filled_path(path.control_points, target_buffer, width, height, 
                        stride, r, g, b, a, y_offset);
    } else {
        // Draw stroked path - polyline
        draw_stroked_path(path.control_points, target_buffer, width, height,
                         stride, r, g, b, a, path.stroke_width, path.stroke_cap, y_offset);
    }
}

void VectorRenderer::render_paths(const std::vector<BezierPath>& paths,
                                   uint8_t* target_buffer, int width, int height,
                                   int stride) {
    // Optional per-path profiling. Enable by defining PATH_PROFILING=1 at compile-time.
#if defined(PATH_PROFILING) && PATH_PROFILING
    using clock = std::chrono::steady_clock;

    // Aggregated stats: key -> (total_us, count)
    static std::unordered_map<size_t, std::pair<uint64_t, uint32_t>> stats;
    static uint32_t frame_counter = 0;
    const uint32_t AGG_WINDOW = 120; // print top N every AGG_WINDOW calls

    for (const auto& path : paths) {
        // Build a simple hash for this path (color + fill + size + sample points)
        size_t h = 1469598103934665603uULL;
        auto mix = [&](uint64_t v) {
            h ^= v;
            h *= 1099511628211uULL;
        };
        mix(path.color);
        mix(path.is_filled ? 0xF00D : 0xC0DE);
        mix(path.control_points.size());
        // include up to first 3 control points quantized
        for (size_t i = 0; i < path.control_points.size() && i < 3; ++i) {
            int xi = (int)std::lround(path.control_points[i].x);
            int yi = (int)std::lround(path.control_points[i].y);
            mix(((uint64_t)xi << 32) ^ (uint64_t)yi);
        }

        auto t0 = clock::now();
        render_path(path, target_buffer, width, height, stride);
        auto t1 = clock::now();
        uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

        auto &entry = stats[h];
        entry.first += us;
        entry.second += 1;
    }

    // Periodically print aggregate top-N slow paths
    frame_counter++;
    if (frame_counter >= AGG_WINDOW) {
        frame_counter = 0;
        // move stats into a vector and sort by total time
        std::vector<std::pair<size_t, std::pair<uint64_t, uint32_t>>> vec;
        vec.reserve(stats.size());
        for (auto &p : stats) vec.emplace_back(p.first, p.second);
        std::sort(vec.begin(), vec.end(), [](auto &a, auto &b){ return a.second.first > b.second.first; });

        // Print top 5
        printf("VectorRenderer: Top %zu slow paths (aggregate over %u frames):\n", vec.size() < 5 ? vec.size() : 5, AGG_WINDOW);
        for (size_t i = 0; i < vec.size() && i < 5; ++i) {
            uint64_t total_us = vec[i].second.first;
            uint32_t cnt = vec[i].second.second;
            double avg = cnt ? ((double)total_us / cnt) : 0.0;
            printf("  [%zu] total=%.2fms count=%u avg=%.2fus\n", i, total_us / 1000.0, cnt, avg);
        }

        // Keep the stats but decay them slightly to emphasize recent behavior
        for (auto &p : stats) {
            p.second.first = p.second.first / 2;
            p.second.second = p.second.second / 2;
        }
    }
#else
    for (const auto& path : paths) {
        render_path(path, target_buffer, width, height, stride);
    }
#endif
}

void VectorRenderer::set_quality(int quality_level) {
    quality_level_ = quality_level;
}

void VectorRenderer::draw_filled_path(const std::vector<Point>& points,
                                      uint8_t* buffer, int width, int height,
                                      int stride, uint8_t r, uint8_t g,
                                      uint8_t b, uint8_t a, int y_offset) {
    // Simple flood-fill using scanline algorithm
    if (points.empty()) return;
    
    // Find bounding box
    float min_x = points[0].x, max_x = points[0].x;
    float min_y = points[0].y, max_y = points[0].y;
    
    for (const auto& p : points) {
        min_x = std::min(min_x, p.x);
        max_x = std::max(max_x, p.x);
        min_y = std::min(min_y, p.y);
        max_y = std::max(max_y, p.y);
    }
    
    // Clip to tile bounds (accounting for y_offset)
    int start_y = std::max(0, (int)min_y - y_offset);
    int end_y = std::min(height - 1, (int)max_y - y_offset + 1);
    
    // For each scanline in bounding box
    for (int y = start_y; y <= end_y; ++y) {
        std::vector<float> intersections;
        intersections.reserve(points.size());
        
        // Actual y coordinate in full space
        float actual_y = y + y_offset;
        
        // Find all intersections with edges
        for (size_t i = 0; i < points.size(); ++i) {
            size_t j = (i + 1) % points.size();
            const Point& p1 = points[i];
            const Point& p2 = points[j];
            
            // Check if edge crosses scanline at actual_y
            if ((p1.y <= actual_y && p2.y > actual_y) || (p2.y <= actual_y && p1.y > actual_y)) {
                float dy = p2.y - p1.y;
                if (dy != 0.0f) {  // Avoid divide by zero
                    float x_intersect = p1.x + (actual_y - p1.y) / dy * (p2.x - p1.x);
                    intersections.push_back(x_intersect);
                }
            }
        }
        
        // Sort intersections
        std::sort(intersections.begin(), intersections.end());
        
        // Fill between pairs of intersections
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            int x1 = std::max(0, (int)intersections[i]);
            int x2 = std::min(width - 1, (int)intersections[i + 1] + 1);

            for (int x = x1; x < x2; ++x) {
                int pixel_offset = y * stride + x * 4;
                buffer[pixel_offset + 0] = r;
                buffer[pixel_offset + 1] = g;
                buffer[pixel_offset + 2] = b;
                buffer[pixel_offset + 3] = a;
            }
        }
    }
}

void VectorRenderer::draw_stroked_path(const std::vector<Point>& points,
                                       uint8_t* buffer, int width, int height,
                                       int stride, uint8_t r, uint8_t g,
                                       uint8_t b, uint8_t a,
                                       float stroke_width, StrokeLineCap cap, int y_offset) {
    if (points.empty()) return;
    
    // Draw line segments between consecutive points
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        draw_line(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y,
                 buffer, width, height, stride, r, g, b, a, stroke_width, y_offset);
    }
    
    // Draw caps at endpoints if round
    if (cap == StrokeLineCap::Round && points.size() >= 2) {
        // Round cap radius equals half the stroke width (standard SVG behavior)
        float radius = stroke_width / 2.0f;
        
        // Start cap at first point - render in stroke color
        draw_round_cap(points[0].x, points[0].y, buffer, width, height,
                      stride, r, g, b, a, radius, y_offset);
        
        // End cap at last point - render in stroke color
        draw_round_cap(points.back().x, points.back().y, buffer, width, height,
                      stride, r, g, b, a, radius, y_offset);
    } else if (cap == StrokeLineCap::Round && points.size() == 1) {
        // Single point: just draw a circle at that location
        float radius = stroke_width / 2.0f;
        draw_round_cap(points[0].x, points[0].y, buffer, width, height,
                      stride, r, g, b, a, radius, y_offset);
    }
}
void VectorRenderer::draw_line(float x0, float y0, float x1, float y1,
                                uint8_t* buffer, int width, int height,
                                int stride, uint8_t r, uint8_t g, uint8_t b,
                                uint8_t a, float stroke_width, int y_offset) {
    // Draw a true thick line segment (distance-to-segment coverage),
    // avoiding circle-stamp overdraw artifacts.
    float dx = x1 - x0;
    float dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);

    float radius = stroke_width / 2.0f;

    if (len < 0.001f) {
        draw_round_cap(x0, y0, buffer, width, height, stride, r, g, b, a, radius, y_offset);
        return;
    }

    const float line_min_y = std::min(y0, y1) - radius - 1.0f;
    const float line_max_y = std::max(y0, y1) + radius + 1.0f;
    const float tile_min_y = static_cast<float>(y_offset);
    const float tile_max_y = static_cast<float>(y_offset + height - 1);
    if (line_max_y < tile_min_y || line_min_y > tile_max_y) {
        return;
    }

    const float inv_len_sq = 1.0f / (len * len);
    const float aa_outer = radius + 0.75f;
    const float aa_inner = std::max(0.0f, radius - 0.75f);

    const int min_x = std::max(0, static_cast<int>(std::floor(std::min(x0, x1) - aa_outer)));
    const int max_x = std::min(width - 1, static_cast<int>(std::ceil(std::max(x0, x1) + aa_outer)));
    const int min_y = std::max(y_offset, static_cast<int>(std::floor(std::min(y0, y1) - aa_outer)));
    const int max_y = std::min(y_offset + height - 1, static_cast<int>(std::ceil(std::max(y0, y1) + aa_outer)));

    for (int py = min_y; py <= max_y; ++py) {
        const int tile_y = py - y_offset;
        const float fy = static_cast<float>(py) + 0.5f;
        for (int px = min_x; px <= max_x; ++px) {
            const float fx = static_cast<float>(px) + 0.5f;

            float t = ((fx - x0) * dx + (fy - y0) * dy) * inv_len_sq;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            const float proj_x = x0 + t * dx;
            const float proj_y = y0 + t * dy;
            const float ddx = fx - proj_x;
            const float ddy = fy - proj_y;
            const float dist = std::sqrt(ddx * ddx + ddy * ddy);

            if (dist > aa_outer) {
                continue;
            }

            int offset = tile_y * stride + px * 4;
            if (offset + 3 >= stride * height) {
                continue;
            }

            if (dist <= aa_inner) {
                blend_pixel_src_over(&buffer[offset], r, g, b, a);
            } else {
                const float edge = (aa_outer - dist) / (aa_outer - aa_inner);
                const uint8_t edge_a = static_cast<uint8_t>(static_cast<float>(a) * std::max(0.0f, std::min(1.0f, edge)));
                blend_pixel_src_over(&buffer[offset], r, g, b, edge_a);
            }
        }
    }
}

void VectorRenderer::draw_round_cap(float x, float y, uint8_t* buffer,
                                    int width, int height, int stride,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                                    float radius, int y_offset) {
    // Draw a filled circle at the endpoint with anti-aliasing
    float cx = x;
    float cy = y;
    float rad = radius;
    float rad_sq = rad * rad;
    float edge_rad = rad + 1.5f;
    float edge_rad_sq = edge_rad * edge_rad;
    int rad_int = (int)(rad + 2.0f);
    
    if (rad_int <= 0) return;
    
    // Draw filled circle scanline by scanline with anti-aliasing
    for (int dy = -rad_int; dy <= rad_int; ++dy) {
        int py = cy + dy;
        if (py < y_offset || py >= y_offset + height) continue;
        int tile_y = py - y_offset;
        
        float y_dist = std::abs((float)dy);
        if (y_dist > rad + 1.0f) continue;
        
        // Calculate the width of the circle at this y position
        float dx_max_sq = rad * rad - y_dist * y_dist;
        if (dx_max_sq < 0) continue;
        
        float dx_max = std::sqrt(dx_max_sq) + 1.0f;
        int dx_max_int = (int)(dx_max + 1.0f);
        
        for (int dx = -(int)dx_max_int; dx <= (int)dx_max_int; ++dx) {
            int px = cx + dx;
            if (px < 0 || px >= width) continue;
            
            // Calculate distance from center
            float dist_sq = dx * dx + dy * dy;
            
            // Use distance-based blending for smooth edges
            if (dist_sq <= rad_sq) {
                int offset = tile_y * stride + px * 4;
                if (offset + 3 < stride * height) {
                    buffer[offset + 0] = r;
                    buffer[offset + 1] = g;
                    buffer[offset + 2] = b;
                    buffer[offset + 3] = a;
                }
            } else if (dist_sq <= edge_rad_sq) {
                // Anti-aliased edge pixels
                float dist = std::sqrt(dist_sq);
                float blend = 1.0f - (dist - rad) / 1.5f;
                int offset = tile_y * stride + px * 4;
                if (offset + 3 < stride * height) {
                    if (blend > 0.0f) {
                        uint8_t edge_a = static_cast<uint8_t>(a * std::min(1.0f, std::max(0.0f, blend)));
                        blend_pixel_src_over(&buffer[offset], r, g, b, edge_a);
                    }
                }
            }
        }
    }
}

} // namespace digidash
