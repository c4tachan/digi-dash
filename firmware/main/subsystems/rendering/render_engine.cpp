#include "render_engine.h"
#include "digidash/binary_gauge_loader.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <cstring>

static const char* TAG = "RenderEngine";

namespace digidash {

RenderEngine::RenderEngine(DisplayDriver& display, uint32_t tile_height)
    : display_(display)
    , tile_height_(tile_height)
    , num_tiles_(0)
    , gauge_scene_(nullptr)
    , rgba_tile_buffer_(nullptr)
    , rgb565_tile_buffer_(nullptr)
    , frame_count_(0)
    , initialized_(false) {
}

RenderEngine::~RenderEngine() {
    if (rgba_tile_buffer_) {
        free(rgba_tile_buffer_);
    }
    if (rgb565_tile_buffer_) {
        free(rgb565_tile_buffer_);
    }
}

bool RenderEngine::initialize() {
    if (initialized_) {
        ESP_LOGW(TAG, "Render engine already initialized");
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
    
    ESP_LOGI(TAG, "Render engine initialized: %lux%lu display, %lu tiles of height %lu",
             (unsigned long)width, (unsigned long)height, 
             (unsigned long)num_tiles_, (unsigned long)tile_height_);
    
    initialized_ = true;
    return true;
}

bool RenderEngine::load_gauge(const uint8_t* data, size_t size) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Render engine not initialized");
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
    
    ESP_LOGI(TAG, "Gauge loaded successfully");
    return true;
}

void RenderEngine::render_frame() {
    if (!initialized_ || !gauge_scene_) {
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
        gauge_scene_->render(rgba_tile_buffer_, width, tile_h, width * 4, tile_y);
        
        // Convert RGBA to RGB565
        convert_rgba_to_rgb565(rgba_tile_buffer_, rgb565_tile_buffer_, width * tile_h);
        
        // Send tile to display
        display_.draw_bitmap(0, tile_y, width, tile_y + tile_h, rgb565_tile_buffer_);
    }
    
    frame_count_++;
    
    if (frame_count_ % 30 == 0) {
        ESP_LOGI(TAG, "Frame %lu rendered", (unsigned long)frame_count_);
    }
}

void RenderEngine::convert_rgba_to_rgb565(const uint8_t* rgba_buffer, uint16_t* rgb565_buffer, size_t pixel_count) {
    for (size_t i = 0; i < pixel_count; i++) {
        uint8_t r = rgba_buffer[i * 4 + 0];
        uint8_t g = rgba_buffer[i * 4 + 1];
        uint8_t b = rgba_buffer[i * 4 + 2];
        rgb565_buffer[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
}

} // namespace digidash
