#pragma once

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
    };

    VectorRenderer();
    ~VectorRenderer();

    /**
     * @brief Render a single Bézier path to a target buffer
     */
    void render_path(const BezierPath& path, uint8_t* target_buffer, 
                     int width, int height, int stride);

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
                         uint8_t b, uint8_t a);
    
    /**
     * @brief Draw path outline with strokes
     */
    void draw_stroked_path(const std::vector<Point>& points, uint8_t* buffer,
                          int width, int height, int stride, uint8_t r, uint8_t g,
                          uint8_t b, uint8_t a, float stroke_width);
    
    /**
     * @brief Draw a single line with thickness
     */
    void draw_line(float x0, float y0, float x1, float y1, uint8_t* buffer,
                  int width, int height, int stride, uint8_t r, uint8_t g,
                  uint8_t b, uint8_t a, float stroke_width);
};

} // namespace digidash
