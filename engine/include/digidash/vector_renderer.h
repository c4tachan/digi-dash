#pragma once

#include "types.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace digidash {

/**
 * @brief Vector rendering engine for cubic Bézier paths and shapes
 * 
 * Renders vector paths at any scale with high quality anti-aliasing.
 * Can output to LVGL framebuffers or other target surfaces.
 */
class VectorRenderer {
public:
    struct Point {
        float x;
        float y;
    };

    struct BezierPath {
        std::vector<Point> control_points;
        uint32_t color;
        float stroke_width;
        bool is_filled;
        StrokeLineCap stroke_cap;
    };

    VectorRenderer();
    ~VectorRenderer();

    /**
     * @brief Render a single Bézier path to a target buffer
     * @param path The path to render
     * @param target_buffer The buffer to render into
     * @param width Width of the target buffer
     * @param height Height of the target buffer (tile height)
     * @param stride Byte stride of the target buffer
     * @param y_offset Y offset in the full coordinate space (for tiled rendering)
     */
    void render_path(const BezierPath& path, uint8_t* target_buffer, 
                     int width, int height, int stride, int y_offset = 0);

    /**
     * @brief Render multiple paths in sequence
     */
    void render_paths(const std::vector<BezierPath>& paths, uint8_t* target_buffer,
                      int width, int height, int stride);

    /**
     * @brief Set rendering quality (affects performance)
     */
    void set_quality(int quality_level);

private:
    int quality_level_;
    
    /**
     * @brief Draw a filled polygon using scanline algorithm
     */
    void draw_filled_path(const std::vector<Point>& points, uint8_t* buffer,
                         int width, int height, int stride, uint8_t r, uint8_t g,
                         uint8_t b, uint8_t a, int y_offset = 0);
    
    /**
     * @brief Draw path outline with strokes
     */
    void draw_stroked_path(const std::vector<Point>& points, uint8_t* buffer,
                          int width, int height, int stride, uint8_t r, uint8_t g,
                          uint8_t b, uint8_t a, float stroke_width, StrokeLineCap cap,
                          int y_offset = 0);
    
    /**
     * @brief Draw a round cap at a point
     */
    void draw_round_cap(float x, float y, uint8_t* buffer, int width, int height,
                       int stride, uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                       float radius, int y_offset = 0);
    
    /**
     * @brief Draw a single line with thickness
     */
    void draw_line(float x0, float y0, float x1, float y1, uint8_t* buffer,
                  int width, int height, int stride, uint8_t r, uint8_t g,
                  uint8_t b, uint8_t a, float stroke_width, int y_offset = 0);
};

} // namespace digidash
