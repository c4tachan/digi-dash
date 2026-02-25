#include "tile_height_renderer.h"
#include "digidash/binary_gauge_loader.h"
#include "platform/display/display_driver.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <functional>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>
#include "digidash/color_utils.h"

// Provide fallbacks for ESP-IDF heap helpers when building tests on host
#ifndef MALLOC_CAP_8BIT
#define MALLOC_CAP_8BIT 0
#endif
#ifndef MALLOC_CAP_INTERNAL
#define MALLOC_CAP_INTERNAL 0
#endif
#ifndef MALLOC_CAP_SPIRAM
#define MALLOC_CAP_SPIRAM 0
#endif
#ifndef heap_caps_malloc
#define heap_caps_malloc(sz, caps) malloc(sz)
#endif

static const char* TAG = "TileHeightRenderer";

namespace digidash {

namespace {
// Simple helpers to draw basic UI onto an RGB565 framebuffer for diagnostics.
static inline uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static void draw_rect_rgb565(uint16_t* fb, int fb_w, int fb_h, int x, int y, int w, int h, uint16_t color) {
    if (!fb || w <= 0 || h <= 0) return;
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(fb_w, x + w);
    int y1 = std::min(fb_h, y + h);
    for (int yy = y0; yy < y1; ++yy) {
        uint16_t* row = fb + yy * fb_w;
        for (int xx = x0; xx < x1; ++xx) {
            row[xx] = color;
        }
    }
}

// Draw a single 7-segment digit using filled rectangles. x,y is top-left.
static void draw_digit_7seg(uint16_t* fb, int fb_w, int fb_h, int digit, int x, int y, int seg_len, int seg_thick, uint16_t color) {
    static const uint8_t seg_map[10] = {
        // gfedcba
        0b0111111, // 0
        0b0000110, // 1
        0b1011011, // 2
        0b1001111, // 3
        0b1100110, // 4
        0b1101101, // 5
        0b1111101, // 6
        0b0000111, // 7
        0b1111111, // 8
        0b1101111  // 9
    };
    if (digit < 0 || digit > 9) return;
    uint8_t m = seg_map[digit];

    int a_x = x + seg_thick;
    int a_y = y;
    int b_x = x + seg_len + seg_thick;
    int b_y = y + seg_thick;
    int c_x = b_x;
    int c_y = y + seg_len + seg_thick;
    int d_x = a_x;
    int d_y = y + seg_len * 2 + seg_thick;
    int e_x = x;
    int e_y = c_y;
    int f_x = x;
    int f_y = b_y;
    int hor_w = seg_len;
    int hor_h = seg_thick;
    int ver_w = seg_thick;
    int ver_h = seg_len;

    // segment a
    if (m & 0x01) draw_rect_rgb565(fb, fb_w, fb_h, a_x, a_y, hor_w, hor_h, color);
    // segment b
    if (m & 0x02) draw_rect_rgb565(fb, fb_w, fb_h, b_x, b_y, ver_w, ver_h, color);
    // segment c
    if (m & 0x04) draw_rect_rgb565(fb, fb_w, fb_h, c_x, c_y, ver_w, ver_h, color);
    // segment d
    if (m & 0x08) draw_rect_rgb565(fb, fb_w, fb_h, d_x, d_y, hor_w, hor_h, color);
    // segment e
    if (m & 0x10) draw_rect_rgb565(fb, fb_w, fb_h, e_x, e_y, ver_w, ver_h, color);
    // segment f
    if (m & 0x20) draw_rect_rgb565(fb, fb_w, fb_h, f_x, f_y, ver_w, ver_h, color);
    // segment g (center)
    if (m & 0x40) draw_rect_rgb565(fb, fb_w, fb_h, a_x, y + seg_len, hor_w, hor_h, color);
}

static void draw_fps_overlay(uint16_t* fb, int fb_w, int fb_h, int fps) {
    if (!fb) return;
    char buf[8];
    int len = std::snprintf(buf, sizeof(buf), "%d", fps);
    if (len <= 0) return;
    int seg_len = std::max(8, fb_h / 30); // scale with display
    int seg_thick = std::max(2, seg_len / 4);
    int digit_w = seg_len + seg_thick * 2;
    int spacing = seg_thick * 2;
    int total_w = len * digit_w + (len - 1) * spacing;
    int start_x = (fb_w - total_w) / 2;
    int start_y = (fb_h / 2) - (seg_len); // roughly centered vertically

    // background box
    int pad = seg_thick * 2;
    draw_rect_rgb565(fb, fb_w, fb_h, start_x - pad, start_y - pad, total_w + pad * 2, seg_len * 2 + seg_thick * 4, rgb_to_rgb565(0,0,0));

    uint16_t color = rgb_to_rgb565(255, 255, 255);
    for (int i = 0; i < len; ++i) {
        char c = buf[i];
        if (c < '0' || c > '9') continue;
        int d = c - '0';
        int x = start_x + i * (digit_w + spacing);
        draw_digit_7seg(fb, fb_w, fb_h, d, x, start_y, seg_len, seg_thick, color);
    }
}

} // anonymous namespace

TileHeightRenderer::TileHeightRenderer(DisplayDriver& display, uint32_t tile_height)
    : display_(display)
    , tile_height_(tile_height)
    , num_tiles_(0)
    , gauge_scene_(nullptr)
    , rgba_tile_buffer_(nullptr)
    , static_rgba_frame_buffer_(nullptr)
    , static_rgb565_frame_buffer_(nullptr)
    , rgb565_tile_buffer_(nullptr)
    , frame_count_(0)
    , initialized_(false)
    , static_cache_ready_(false) {
}

TileHeightRenderer::~TileHeightRenderer() {
    if (rgba_tile_buffer_) {
        free(rgba_tile_buffer_);
    }
    if (static_rgba_frame_buffer_) {
        free(static_rgba_frame_buffer_);
    }
    if (static_rgb565_frame_buffer_) {
        free(static_rgb565_frame_buffer_);
    }
    if (rgb565_tile_buffer_) {
        free(rgb565_tile_buffer_);
    }
}

bool TileHeightRenderer::initialize() {
    if (initialized_) {
        ESP_LOGW(TAG, "Renderer already initialized");
        return true;
    }

    uint32_t width = display_.get_width();
    uint32_t height = display_.get_height();
    
    // Calculate number of tiles needed
    num_tiles_ = (height + tile_height_ - 1) / tile_height_;
    
    // Allocate RGBA tile buffer for rendering
    size_t rgba_tile_size = width * tile_height_ * 4;
    rgba_tile_buffer_ = (uint8_t*)heap_caps_malloc(rgba_tile_size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (!rgba_tile_buffer_) {
        ESP_LOGE(TAG, "Failed to allocate RGBA tile buffer (%zu bytes)", rgba_tile_size);
        return false;
    }
    
    // Allocate RGB565 tile buffer for display
    size_t rgb565_tile_size = width * tile_height_ * sizeof(uint16_t);
    rgb565_tile_buffer_ = (uint16_t*)heap_caps_malloc(rgb565_tile_size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (!rgb565_tile_buffer_) {
        ESP_LOGE(TAG, "Failed to allocate RGB565 tile buffer (%zu bytes)", rgb565_tile_size);
        free(rgba_tile_buffer_);
        rgba_tile_buffer_ = nullptr;
        return false;
    }

    // Allocate full-frame static RGBA cache (PSRAM preferred)
    size_t static_rgba_size = width * height * 4;
    static_rgba_frame_buffer_ = (uint8_t*)heap_caps_malloc(static_rgba_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (!static_rgba_frame_buffer_) {
        ESP_LOGW(TAG, "Static RGBA cache allocation failed (%zu bytes), running without static cache", static_rgba_size);
    }
    // Allocate full-frame static RGB565 cache (PSRAM preferred)
    size_t static_rgb565_size = width * height * sizeof(uint16_t);
    static_rgb565_frame_buffer_ = (uint16_t*)heap_caps_malloc(static_rgb565_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (!static_rgb565_frame_buffer_) {
        ESP_LOGW(TAG, "Static RGB565 cache allocation failed (%zu bytes), running without static cache", static_rgb565_size);
    }
    
    ESP_LOGI(TAG, "Renderer initialized: %lux%lu display, %lu tiles of height %lu",
             (unsigned long)width, (unsigned long)height, 
             (unsigned long)num_tiles_, (unsigned long)tile_height_);

    initialized_ = true;
    return true;
}

bool TileHeightRenderer::load_gauge(const uint8_t* data, size_t size) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Renderer not initialized");
        return false;
    }

    BinaryGaugeLoader loader;
    BinaryGaugeLoader::GaugeAsset asset;
    
    if (!loader.load_from_buffer(data, size, asset)) {
        ESP_LOGE(TAG, "Failed to parse gauge data");
        return false;
    }
    
    ESP_LOGI(TAG, "Gauge parsed: %lux%lu with %zu paths",
             (unsigned long)asset.width, (unsigned long)asset.height, 
             asset.paths.size());
    
    gauge_scene_ = std::make_unique<GaugeScene>();
    gauge_scene_->load_gauge(asset);
    gauge_scene_->set_viewport(display_.get_width(), display_.get_height());
    build_static_cache(display_.get_width(), display_.get_height());
    
    ESP_LOGI(TAG, "Gauge loaded successfully");
    return true;
}

void TileHeightRenderer::build_static_cache(uint32_t width, uint32_t height) {
    static_cache_ready_ = false;
    if (!gauge_scene_ || !static_rgba_frame_buffer_) {
        return;
    }

    std::memset(static_rgba_frame_buffer_, 0, width * height * 4);
    gauge_scene_->render_static(static_rgba_frame_buffer_, width, height, width * 4, 0);
    // Pre-convert static RGBA into RGB565 to avoid per-frame conversion costs
    if (static_rgb565_frame_buffer_) {
        digidash::convert_rgba_buffer_to_rgb565(static_rgba_frame_buffer_, static_rgb565_frame_buffer_, (size_t)width * height);
    }
    static_cache_ready_ = true;

    // Initialize both hardware framebuffers with the static RGB565 image so
    // subsequent frames only need to write dynamic pixels.
    if (static_rgb565_frame_buffer_) {
        display_.draw_bitmap(0, 0, width, height, static_rgb565_frame_buffer_);
    }

}

void TileHeightRenderer::set_pid_value(uint32_t pid_id, float value) {
    if (gauge_scene_) {
        gauge_scene_->set_pid_value(pid_id, value);
    }
}

void TileHeightRenderer::render_frame() {
    if (!initialized_ || (!gauge_scene_ && !test_render_cb_)) {
        return;
    }

    uint32_t width = display_.get_width();
    uint32_t height = display_.get_height();
    uint16_t* back_buffer = display_.acquire_back_buffer();
    if (!back_buffer) {
        return;
    }

    constexpr int render_quality = 2;

    if (gauge_scene_) {
        gauge_scene_->set_render_quality(render_quality);

        static uint32_t last_tick = xTaskGetTickCount();
        uint32_t now_tick = xTaskGetTickCount();
        uint32_t delta_ms = (now_tick - last_tick) * portTICK_PERIOD_MS;
        if (delta_ms == 0) {
            delta_ms = 1;
        }
        last_tick = now_tick;
        gauge_scene_->update(delta_ms);
    }

    std::vector<uint8_t> tile_has_dynamic_map(num_tiles_, 0);
    uint32_t estimated_dynamic_tiles = 0;
    if (gauge_scene_) {
        for (uint32_t tile = 0; tile < num_tiles_; ++tile) {
            uint32_t tile_y = tile * tile_height_;
            uint32_t tile_h = (tile_y + tile_height_ > height) ? (height - tile_y) : tile_height_;
            if (gauge_scene_->has_dynamic_in_region(tile_y, tile_h)) {
                tile_has_dynamic_map[tile] = 1;
                ++estimated_dynamic_tiles;
            }
        }
    }
    
    // Profiling timers (microseconds)
    uint64_t t_frame_start = esp_timer_get_time();
    uint64_t t_static_copy = 0;
    uint64_t t_render_paths = 0;
    uint64_t t_convert = 0;
    uint64_t t_tile_copy = 0;
    uint32_t dynamic_tiles = 0;
    
    // Render frame tile by tile into the inactive full-frame back buffer
    for (uint32_t tile = 0; tile < num_tiles_; tile++) {
        uint32_t tile_y = tile * tile_height_;
        uint32_t tile_h = (tile_y + tile_height_ > height) ? (height - tile_y) : tile_height_;
        
        // If static cache is ready and this tile has no dynamic content, copy directly
        // from the static RGB565 cache to the back buffer (fast path).
        bool tile_has_dynamic = tile_has_dynamic_map[tile] != 0;

        if (tile_has_dynamic) {
            ++dynamic_tiles;
        }

        if (static_cache_ready_ && static_rgb565_frame_buffer_ && !tile_has_dynamic) {
            uint64_t t0 = esp_timer_get_time();
            for (uint32_t row = 0; row < tile_h; ++row) {
                const uint32_t src_offset = (tile_y + row) * width;
                const uint32_t dst_offset = (tile_y + row) * width;
                std::memcpy(&back_buffer[dst_offset], &static_rgb565_frame_buffer_[src_offset], width * sizeof(uint16_t));
            }
            t_static_copy += (esp_timer_get_time() - t0);
            continue; // move to next tile
        }

        // Two modes:
        // 1) static cache present: hardware framebuffers already contain static image.
        //    - if tile has no dynamic content -> skip (already static)
        //    - if tile has dynamic content -> render dynamic into RGBA and write only dynamic pixels into back_buffer
        // 2) no static cache: render into RGBA, convert full tile -> RGB565 and memcpy whole tile
        if (static_cache_ready_ && static_rgb565_frame_buffer_) {
            // Prepare RGBA tile for dynamic rendering (dynamic only)
            std::memset(rgba_tile_buffer_, 0, width * tile_h * 4);
        } else {
            uint64_t t0 = esp_timer_get_time();
            std::memset(rgba_tile_buffer_, 0, width * tile_h * 4);
            t_static_copy += (esp_timer_get_time() - t0);
            // Ensure rgb565_tile_buffer_ is cleared when no static cache
            std::memset(rgb565_tile_buffer_, 0, width * tile_h * sizeof(uint16_t));
        }

        // Render this tile
        if (gauge_scene_) {
            if (static_cache_ready_) {
                uint64_t t0 = esp_timer_get_time();
                gauge_scene_->render_dynamic(rgba_tile_buffer_, width, tile_h, width * 4, tile_y);
                t_render_paths += (esp_timer_get_time() - t0);
            } else {
                uint64_t t0 = esp_timer_get_time();
                gauge_scene_->render(rgba_tile_buffer_, width, tile_h, width * 4, tile_y);
                t_render_paths += (esp_timer_get_time() - t0);
            }
        } else if (test_render_cb_) {
            uint64_t t0 = esp_timer_get_time();
            test_render_cb_(rgba_tile_buffer_, width, tile_h, width * 4, tile_y);
            t_render_paths += (esp_timer_get_time() - t0);
        }
        
        if (static_cache_ready_ && static_rgb565_frame_buffer_) {
            uint64_t t0 = esp_timer_get_time();
            for (uint32_t row = 0; row < tile_h; ++row) {
                const uint32_t src_offset = (tile_y + row) * width;
                const uint32_t dst_offset = (tile_y + row) * width;
                std::memcpy(&back_buffer[dst_offset], &static_rgb565_frame_buffer_[src_offset], width * sizeof(uint16_t));
            }
            t_static_copy += (esp_timer_get_time() - t0);

            uint64_t t1 = esp_timer_get_time();
            for (uint32_t row = 0; row < tile_h; ++row) {
                const size_t row_base = (size_t)row * width;
                const uint32_t dst_row_offset = (tile_y + row) * width;
                for (uint32_t col = 0; col < width; ++col) {
                    const size_t pi = row_base + col;
                    const uint8_t src_a = rgba_tile_buffer_[pi * 4 + 3];
                    if (src_a == 0) {
                        continue;
                    }

                    const uint8_t r = rgba_tile_buffer_[pi * 4 + 0];
                    const uint8_t g = rgba_tile_buffer_[pi * 4 + 1];
                    const uint8_t b = rgba_tile_buffer_[pi * 4 + 2];

                    if (src_a == 255) {
                        back_buffer[dst_row_offset + col] = digidash::rgba_to_rgb565(r, g, b);
                    } else {
                        const uint16_t dst_rgb565 = back_buffer[dst_row_offset + col];
                        const uint8_t sr = (dst_rgb565 >> 11) & 0x1F;
                        const uint8_t sg = (dst_rgb565 >> 5) & 0x3F;
                        const uint8_t sb = dst_rgb565 & 0x1F;
                        const uint8_t s_r8 = (sr << 3) | (sr >> 2);
                        const uint8_t s_g8 = (sg << 2) | (sg >> 4);
                        const uint8_t s_b8 = (sb << 3) | (sb >> 2);

                        const uint32_t inv_a = 255 - src_a;
                        const uint8_t out_r = static_cast<uint8_t>((src_a * r + inv_a * s_r8) / 255);
                        const uint8_t out_g = static_cast<uint8_t>((src_a * g + inv_a * s_g8) / 255);
                        const uint8_t out_b = static_cast<uint8_t>((src_a * b + inv_a * s_b8) / 255);

                        back_buffer[dst_row_offset + col] = digidash::rgba_to_rgb565(out_r, out_g, out_b);
                    }
                }
            }
            t_convert += (esp_timer_get_time() - t1);
        } else {
            // Convert entire RGBA tile to RGB565 then copy into back buffer
            uint64_t t0 = esp_timer_get_time();
            convert_rgba_to_rgb565(rgba_tile_buffer_, rgb565_tile_buffer_, width * tile_h);
            t_convert += (esp_timer_get_time() - t0);

            uint64_t t1 = esp_timer_get_time();
            for (uint32_t row = 0; row < tile_h; ++row) {
                uint32_t dst_offset = (tile_y + row) * width;
                uint32_t src_offset = row * width;
                std::memcpy(&back_buffer[dst_offset], &rgb565_tile_buffer_[src_offset], width * sizeof(uint16_t));
            }
            t_tile_copy += (esp_timer_get_time() - t1);
        }
        
    }

    // Draw FPS overlay onto final RGB565 back buffer (centered)
    static uint32_t last_present_tick = 0;
    uint32_t now_present = xTaskGetTickCount();
    if (last_present_tick == 0) last_present_tick = now_present;
    uint32_t delta_present_ms = (now_present - last_present_tick) * portTICK_PERIOD_MS;
    if (delta_present_ms == 0) delta_present_ms = 1;
    int fps = (int)(1000u / delta_present_ms);
    last_present_tick = now_present;

    draw_fps_overlay(back_buffer, width, height, fps);

    // Present fully composed frame in one swap to avoid tile-scanning artifacts
    display_.present_back_buffer(back_buffer);

    frame_count_++;

    if (frame_count_ % 60 == 0) {
        uint64_t t_total = esp_timer_get_time() - t_frame_start;
        ESP_LOGI(TAG, "Frame %lu: total=%.2fms render_paths=%.2fms static_copy=%.2fms convert=%.2fms tile_copy=%.2fms dyn_tiles=%lu/%lu q=%d fps=%d",
                 (unsigned long)frame_count_, t_total / 1000.0, t_render_paths / 1000.0, t_static_copy / 1000.0, t_convert / 1000.0, t_tile_copy / 1000.0,
                 (unsigned long)dynamic_tiles, (unsigned long)num_tiles_, render_quality, fps);
    }
}

#include "digidash/color_utils.h"

void TileHeightRenderer::convert_rgba_to_rgb565(const uint8_t* rgba_buffer, uint16_t* rgb565_buffer, size_t pixel_count) {
    digidash::convert_rgba_buffer_to_rgb565(rgba_buffer, rgb565_buffer, pixel_count);
}

} // namespace digidash
