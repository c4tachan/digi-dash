#include "digidash/vector_renderer.h"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace digidash {

VectorRenderer::VectorRenderer() : quality_level_(2) {}

VectorRenderer::~VectorRenderer() {}

void VectorRenderer::render_path(const BezierPath& path, uint8_t* target_buffer,
                                 int width, int height, int stride) {
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
                        stride, r, g, b, a);
    } else {
        // Draw stroked path - polyline
        draw_stroked_path(path.control_points, target_buffer, width, height,
                         stride, r, g, b, a, path.stroke_width);
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
                                      uint8_t b, uint8_t a) {
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
    
    int start_y = std::max(0, (int)min_y);
    int end_y = std::min(height - 1, (int)max_y + 1);
    
    // For each scanline in bounding box
    for (int y = start_y; y <= end_y; ++y) {
        std::vector<float> intersections;
        
        // Find all intersections with edges
        for (size_t i = 0; i < points.size(); ++i) {
            size_t j = (i + 1) % points.size();
            const Point& p1 = points[i];
            const Point& p2 = points[j];
            
            // Check if edge crosses scanline y
            if ((p1.y <= y && p2.y > y) || (p2.y <= y && p1.y > y)) {
                float x_intersect = p1.x + (y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x);
                intersections.push_back(x_intersect);
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
                if (pixel_offset + 3 < stride * height) {
                    buffer[pixel_offset + 0] = b;
                    buffer[pixel_offset + 1] = g;
                    buffer[pixel_offset + 2] = r;
                    buffer[pixel_offset + 3] = a;
                }
            }
        }
    }
}

void VectorRenderer::draw_stroked_path(const std::vector<Point>& points,
                                       uint8_t* buffer, int width, int height,
                                       int stride, uint8_t r, uint8_t g,
                                       uint8_t b, uint8_t a,
                                       float stroke_width) {
    // Draw line segments between consecutive points
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        draw_line(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y,
                 buffer, width, height, stride, r, g, b, a, stroke_width);
    }
}

void VectorRenderer::draw_line(float x0, float y0, float x1, float y1,
                                uint8_t* buffer, int width, int height,
                                int stride, uint8_t r, uint8_t g, uint8_t b,
                                uint8_t a, float stroke_width) {
    // Bresenham's line algorithm with thickness
    int x0i = (int)x0, y0i = (int)y0;
    int x1i = (int)x1, y1i = (int)y1;
    int stroke = (int)stroke_width;
    
    int dx = abs(x1i - x0i);
    int dy = abs(y1i - y0i);
    int sx = (x0i < x1i) ? 1 : -1;
    int sy = (y0i < y1i) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        // Draw pixel with thickness
        for (int sy_off = -stroke/2; sy_off <= stroke/2; ++sy_off) {
            for (int sx_off = -stroke/2; sx_off <= stroke/2; ++sx_off) {
                int px = x0i + sx_off;
                int py = y0i + sy_off;
                
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    int offset = py * stride + px * 4;
                    buffer[offset + 0] = b;
                    buffer[offset + 1] = g;
                    buffer[offset + 2] = r;
                    buffer[offset + 3] = a;
                }
            }
        }
        
        if (x0i == x1i && y0i == y1i) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0i += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0i += sy;
        }
    }
}

} // namespace digidash
