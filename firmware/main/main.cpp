#include <stdio.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_heap_caps.h"

#include "digidash/gauge_scene.h"
#include "digidash/binary_gauge_loader.h"

static const char* TAG = "digi-dash";

// Display dimensions
static const int DISPLAY_WIDTH = 720;
static const int DISPLAY_HEIGHT = 720;
static const int DISPLAY_STRIDE = DISPLAY_WIDTH * 4;

// Tile-based rendering (render in 60-line chunks to save memory)
static const int TILE_HEIGHT = 60;
static const int TILE_SIZE = DISPLAY_WIDTH * TILE_HEIGHT * 4;

// Framebuffer for rendering (tile-sized)
static uint8_t* framebuffer = nullptr;

// Initialize SPIFFS for gauge file storage
static void init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS info (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS partition size: total: %d, used: %d", total, used);
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Digi-Dash ESP32-S3 Firmware starting...");
    
    // Initialize SPIFFS
    init_spiffs();
    
    // Allocate tile-based framebuffer (much smaller)
    ESP_LOGI(TAG, "Allocating tile buffer: %d bytes for %dx%d tile", 
             TILE_SIZE, DISPLAY_WIDTH, TILE_HEIGHT);
    framebuffer = (uint8_t*)malloc(TILE_SIZE);
    if (!framebuffer) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer");
        return;
    }
    
    // Initialize gauge scene
    digidash::GaugeScene gauge_scene;
    
    // Load gauge file
    const char* gauge_path = "/spiffs/dashboard_tiny.gauge";
    FILE* gauge_file = fopen(gauge_path, "rb");
    
    if (!gauge_file) {
        ESP_LOGE(TAG, "Failed to open gauge file: %s", gauge_path);
        free(framebuffer);
        return;
    }
    
    // Read gauge file into memory
    fseek(gauge_file, 0, SEEK_END);
    size_t gauge_size = ftell(gauge_file);
    fseek(gauge_file, 0, SEEK_SET);
    
    uint8_t* gauge_data = (uint8_t*)malloc(gauge_size);
    if (!gauge_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for gauge data");
        fclose(gauge_file);
        free(framebuffer);
        return;
    }
    
    size_t read_size = fread(gauge_data, 1, gauge_size, gauge_file);
    fclose(gauge_file);
    
    if (read_size != gauge_size) {
        ESP_LOGE(TAG, "Failed to read gauge file");
        free(gauge_data);
        free(framebuffer);
        return;
    }
    
    ESP_LOGI(TAG, "Gauge file loaded: %zu bytes", gauge_size);
    
    // Parse and load gauge
    digidash::BinaryGaugeLoader loader;
    digidash::BinaryGaugeLoader::GaugeAsset asset;
    
    if (loader.load_from_buffer(gauge_data, gauge_size, asset)) {
        ESP_LOGI(TAG, "Gauge parsed: %dx%d with %zu paths", 
                 asset.width, asset.height, asset.paths.size());
        
        // Load the gauge into the scene
        gauge_scene.load_gauge(asset);
        
        ESP_LOGI(TAG, "Gauge scene loaded, starting render loop");
        
        // Render loop
        int frame_count = 0;
        
        while (1) {
            // Clear tile buffer
            std::memset(framebuffer, 0, TILE_SIZE);
            
            // Render gauge to framebuffer (tile-sized: 720x60)
            ESP_LOGD(TAG, "Frame %d: Starting render", frame_count);
            gauge_scene.render(framebuffer, DISPLAY_WIDTH, TILE_HEIGHT, DISPLAY_STRIDE);
            ESP_LOGD(TAG, "Frame %d: Render complete", frame_count);
            
            // Log periodically
            if (++frame_count % 30 == 0) {
                ESP_LOGI(TAG, "Rendering... frame %d", frame_count);
            }
            
            vTaskDelay(33 / portTICK_PERIOD_MS);  // ~30 FPS
        }
    } else {
        ESP_LOGE(TAG, "Failed to parse gauge file");
    }
    
    free(gauge_data);
    free(framebuffer);
}
