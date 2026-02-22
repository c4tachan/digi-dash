#include "tile_height_renderer.h"
#include "digidash/binary_gauge_loader.h"
#include "platform/display/display_driver.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <functional>
#include <cstdlib>

// Provide fallbacks for ESP-IDF heap helpers when building tests on host
#ifndef MALLOC_CAP_8BIT
#define MALLOC_CAP_8BIT 0
#endif
#ifndef MALLOC_CAP_INTERNAL
#define MALLOC_CAP_INTERNAL 0
#endif
#ifndef heap_caps_malloc
#define heap_caps_malloc(sz, caps) malloc(sz)
#endif

static const char* TAG = "TileHeightRenderer";

namespace digidash {

TileHeightRenderer::TileHeightRenderer(DisplayDriver& display, uint32_t tile_height)
    : display_(display)
    , tile_height_(tile_height)
    , num_tiles_(0)
    , gauge_scene_(nullptr)
    , rgba_tile_buffer_(nullptr)
    , rgb565_tile_buffer_(nullptr)
    , frame_count_(0)
    , initialized_(false) {
}

TileHeightRenderer::~TileHeightRenderer() {
    if (rgba_tile_buffer_) {
        free(rgba_tile_buffer_);
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
    
    ESP_LOGI(TAG, "Gauge loaded successfully");
    return true;
}

void TileHeightRenderer::render_frame() {
    if (!initialized_ || (!gauge_scene_ && !test_render_cb_)) {
        return;
    }

    uint32_t width = display_.get_width();
    uint32_t height = display_.get_height();
    
    // Render frame tile by tile
    for (uint32_t tile = 0; tile < num_tiles_; tile++) {
        uint32_t tile_y = tile * tile_height_;
        uint32_t tile_h = (tile_y + tile_height_ > height) ? (height - tile_y) : tile_height_;
        
        // Clear RGBA tile buffer
        std::memset(rgba_tile_buffer_, 0, width * tile_h * 4);

        // Render this tile
        if (gauge_scene_) {
            gauge_scene_->render(rgba_tile_buffer_, width, tile_h, width * 4, tile_y);
        } else if (test_render_cb_) {
            test_render_cb_(rgba_tile_buffer_, width, tile_h, width * 4, tile_y);
        }
        
        // Convert RGBA to RGB565
        convert_rgba_to_rgb565(rgba_tile_buffer_, rgb565_tile_buffer_, width * tile_h);
        
        // Send tile to display
        display_.draw_bitmap(0, tile_y, width, tile_y + tile_h, rgb565_tile_buffer_);
        
        // Reset watchdog to prevent timeout during long rendering
        vTaskDelay(1);
    }
    
    frame_count_++;
    
    if (frame_count_ % 30 == 0) {
        ESP_LOGI(TAG, "Frame %lu rendered", (unsigned long)frame_count_);
    }
}

#include "digidash/color_utils.h"

void TileHeightRenderer::convert_rgba_to_rgb565(const uint8_t* rgba_buffer, uint16_t* rgb565_buffer, size_t pixel_count) {
    digidash::convert_rgba_buffer_to_rgb565(rgba_buffer, rgb565_buffer, pixel_count);
}

} // namespace digidash
