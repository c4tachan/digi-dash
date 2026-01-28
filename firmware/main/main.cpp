#include <stdio.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_qemu_rgb.h"
#include "soc/syscon_reg.h"
#include "soc/soc.h"

#include "digidash/gauge_scene.h"
#include "digidash/binary_gauge_loader.h"

static const char* TAG = "digi-dash";

#define RGB_QEMU_ORIGIN 0x51454d55

// Display dimensions
static const int DISPLAY_WIDTH = 720;
static const int DISPLAY_HEIGHT = 720;
static const int DISPLAY_STRIDE = DISPLAY_WIDTH * 2;  // RGB565 = 2 bytes per pixel

// Framebuffer for full screen (RGB565)
static uint16_t* framebuffer = nullptr;

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
    
    // Initialize QEMU RGB panel
    REG_WRITE(SYSCON_DATE_REG - 4, RGB_QEMU_ORIGIN);
    uint32_t qemu_origin = REG_READ(SYSCON_DATE_REG - 4);
    ESP_LOGI(TAG, "QEMU origin register: 0x%08lx", (unsigned long)qemu_origin);
    
    ESP_LOGI(TAG, "Install RGB LCD panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_qemu_config_t panel_config = {
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
        .bpp = RGB_QEMU_BPP_16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_qemu(&panel_config, &panel_handle));
    
    ESP_LOGI(TAG, "Initialize RGB LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    
    // Initialize SPIFFS
    init_spiffs();
    
    // Allocate tile buffer for RGB565 (render in tiles, send directly to display)
    const int TILE_HEIGHT = 60;
    size_t tile_size = DISPLAY_WIDTH * TILE_HEIGHT * sizeof(uint16_t);
    ESP_LOGI(TAG, "Allocating tile framebuffer: %zu bytes for %dx%d RGB565", 
             tile_size, DISPLAY_WIDTH, TILE_HEIGHT);
    framebuffer = (uint16_t*)heap_caps_malloc(tile_size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (!framebuffer) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer");
        return;
    }
    
    // Allocate RGBA tile buffer for rendering
    uint8_t* rgba_buffer = (uint8_t*)heap_caps_malloc(DISPLAY_WIDTH * TILE_HEIGHT * 4, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (!rgba_buffer) {
        ESP_LOGE(TAG, "Failed to allocate RGBA tile buffer");
        free(framebuffer);
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
        ESP_LOGI(TAG, "Gauge parsed: %lux%lu with %zu paths", 
                 (unsigned long)asset.width, (unsigned long)asset.height, asset.paths.size());
        
        // Load the gauge into the scene
        gauge_scene.load_gauge(asset);
        
        ESP_LOGI(TAG, "Gauge scene loaded, starting render loop");
        
        // Render loop
        int frame_count = 0;
        const int TILE_HEIGHT = 60;
        const int num_tiles = (DISPLAY_HEIGHT + TILE_HEIGHT - 1) / TILE_HEIGHT;
        ESP_LOGI(TAG, "Entering render loop... (tiles: %d)", num_tiles);
        
        while (1) {
            // Render frame tile by tile and send each tile to display
            for (int tile = 0; tile < num_tiles; tile++) {
                int tile_y = tile * TILE_HEIGHT;
                int tile_h = (tile_y + TILE_HEIGHT > DISPLAY_HEIGHT) ? (DISPLAY_HEIGHT - tile_y) : TILE_HEIGHT;
                
                // Clear RGBA tile buffer
                std::memset(rgba_buffer, 0, DISPLAY_WIDTH * tile_h * 4);
                
                // Render this tile (pass y_offset so it knows which vertical slice to render)
                gauge_scene.render(rgba_buffer, DISPLAY_WIDTH, tile_h, DISPLAY_WIDTH * 4, tile_y);
                
                // Convert RGBA to RGB565 in tile buffer
                for (int y = 0; y < tile_h; y++) {
                    for (int x = 0; x < DISPLAY_WIDTH; x++) {
                        int rgba_idx = (y * DISPLAY_WIDTH + x) * 4;
                        int fb_idx = y * DISPLAY_WIDTH + x;
                        
                        uint8_t r = rgba_buffer[rgba_idx + 0];
                        uint8_t g = rgba_buffer[rgba_idx + 1];
                        uint8_t b = rgba_buffer[rgba_idx + 2];
                        framebuffer[fb_idx] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                    }
                }
                
                // Send this tile to QEMU display
                esp_lcd_panel_draw_bitmap(panel_handle, 0, tile_y, DISPLAY_WIDTH, tile_y + tile_h, framebuffer);
            }
            
            frame_count++;
            if (frame_count % 30 == 0) {
                ESP_LOGI(TAG, "Frame %d rendered", frame_count);
            }
            
            vTaskDelay(33 / portTICK_PERIOD_MS);  // ~30 FPS
        }
    } else {
        ESP_LOGE(TAG, "Failed to parse gauge file");
    }
    
    free(gauge_data);
    free(framebuffer);
}
