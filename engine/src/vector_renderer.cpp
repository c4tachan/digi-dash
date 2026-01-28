#include "digidash/vector_renderer.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace digidash {

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
    for (const auto& path : paths) {
        render_path(path, target_buffer, width, height, stride);
    }
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
    
    #ifdef ESP_PLATFORM
    static int render_count = 0;
    bool do_log = (render_count++ < 3); // Log first 3 renders
    #endif
    
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
    
    #ifdef ESP_PLATFORM
    if (do_log) {
        printf("draw_filled_path: bbox y=[%.1f,%.1f] y_offset=%d clipped=[%d,%d] RGBA(%d,%d,%d,%d) tile_h=%d\n",
               min_y, max_y, y_offset, start_y, end_y, r, g, b, a, height);
    }
    #endif
    
    // For each scanline in bounding box
    for (int y = start_y; y <= end_y; ++y) {
        std::vector<float> intersections;
        
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
            
            static int pixel_write_count = 0;
            for (int x = x1; x < x2; ++x) {
                int pixel_offset = y * stride + x * 4;
                if (pixel_offset + 3 < stride * height) {
                    buffer[pixel_offset + 0] = r;
                    buffer[pixel_offset + 1] = g;
                    buffer[pixel_offset + 2] = b;
                    buffer[pixel_offset + 3] = a;
                    if (pixel_write_count < 5) {
                        printf("  Pixel write [%d]: offset=%d y=%d x=%d RGBA(%d,%d,%d,%d)\n", 
                               pixel_write_count, pixel_offset, y, x, r, g, b, a);
                        pixel_write_count++;
                    }
                }
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
    
    #ifdef ESP_PLATFORM
    static int stroke_count = 0;
    if (stroke_count < 2) {
        printf("draw_stroked_path: %zu points, RGBA(%d,%d,%d,%d), width=%.1f, y_offset=%d, tile_h=%d\n",
               points.size(), r, g, b, a, stroke_width, y_offset, height);
        if (points.size() > 0) {
            printf("  First point: (%.1f, %.1f)\n", points[0].x, points[0].y);
            printf("  Last point: (%.1f, %.1f)\n", points.back().x, points.back().y);
        }
        stroke_count++;
    }
    #endif
    
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
                      stride, 0, 255, 0, 255, radius, y_offset);
    }
}
void VectorRenderer::draw_line(float x0, float y0, float x1, float y1,
                                uint8_t* buffer, int width, int height,
                                int stride, uint8_t r, uint8_t g, uint8_t b,
                                uint8_t a, float stroke_width, int y_offset) {
    // Draw line with circular stroke (not square)
    float dx = x1 - x0;
    float dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);
    
    if (len < 0.001f) {
        // Very short or zero-length line, just draw a circle
        float radius = stroke_width / 2.0f;
        int rad_int = (int)(radius + 1.0f);
        for (int cy = -rad_int; cy <= rad_int; ++cy) {
            int py = (int)y0 + cy;
            if (py < y_offset || py >= y_offset + height) continue;
            int tile_y = py - y_offset;
            
            for (int cx = -rad_int; cx <= rad_int; ++cx) {
                int px = (int)x0 + cx;
                if (px < 0 || px >= width) continue;
                
                float dist = std::sqrt(cx * cx + cy * cy);
                if (dist <= radius) {
                    int offset = tile_y * stride + px * 4;
                    buffer[offset + 0] = r;
                    buffer[offset + 1] = g;
                    buffer[offset + 2] = b;
                    buffer[offset + 3] = a;
                }
            }
        }
        return;
    }
    
    // Normalize direction
    float nx = dx / len;
    float ny = dy / len;
    
    // Perpendicular direction for stroke width
    float px_dir = -ny;
    float py_dir = nx;
    
    float radius = stroke_width / 2.0f;
    
    // Draw segments along the line
    float segments = len + 1.0f;
    for (int i = 0; i <= (int)segments; ++i) {
        float t = (segments > 0) ? i / segments : 0;
        float cx = x0 + t * dx;
        float cy = y0 + t * dy;
        
        // Draw circle at this point
        int rad_int = (int)(radius + 1.5f);
        for (int dy_pix = -rad_int; dy_pix <= rad_int; ++dy_pix) {
            int py = (int)cy + dy_pix;
            if (py < y_offset || py >= y_offset + height) continue;
            int tile_y = py - y_offset;
            
            for (int dx_pix = -rad_int; dx_pix <= rad_int; ++dx_pix) {
                int px = (int)cx + dx_pix;
                if (px < 0 || px >= width) continue;
                
                float dist = std::sqrt(dx_pix * dx_pix + dy_pix * dy_pix);
                if (dist <= radius) {
                    int offset = tile_y * stride + px * 4;
                    buffer[offset + 0] = r;
                    buffer[offset + 1] = g;
                    buffer[offset + 2] = b;
                    buffer[offset + 3] = a;
                }
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
            float dist = std::sqrt(dist_sq);
            
            // Use distance-based blending for smooth edges
            if (dist <= rad) {
                int offset = tile_y * stride + px * 4;
                if (offset + 3 < stride * height) {
                    buffer[offset + 0] = r;
                    buffer[offset + 1] = g;
                    buffer[offset + 2] = b;
                    buffer[offset + 3] = a;
                }
            } else if (dist <= rad + 1.5f) {
                // Anti-aliased edge pixels
                float blend = 1.0f - (dist - rad) / 1.5f;
                int offset = tile_y * stride + px * 4;
                if (offset + 3 < stride * height) {
                    buffer[offset + 0] = (uint8_t)(buffer[offset + 0] * (1.0f - blend) + r * blend);
                    buffer[offset + 1] = (uint8_t)(buffer[offset + 1] * (1.0f - blend) + g * blend);
                    buffer[offset + 2] = (uint8_t)(buffer[offset + 2] * (1.0f - blend) + b * blend);
                    buffer[offset + 3] = a;
                }
            }
        }
    }
}

} // namespace digidash
