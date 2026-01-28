#include <stdio.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "soc/syscon_reg.h"
#include "soc/soc.h"

#include "digidash/gauge_scene.h"
#include "digidash/binary_gauge_loader.h"
#include "hardware_config.h"

// Conditional includes based on build configuration
#ifdef CONFIG_DIGI_DASH_TARGET_QEMU
#include "esp_lcd_qemu_rgb.h"
#define USE_QEMU 1
#else
#include "esp_lcd_nv3052.h"
#include "esp_lcd_panel_io_additions.h"
#define USE_QEMU 0
#endif

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

// Initialize display panel (QEMU or hardware)
static esp_lcd_panel_handle_t init_display_panel(void) {
    esp_lcd_panel_handle_t panel_handle = NULL;
    
#if USE_QEMU
    // QEMU RGB panel setup
    REG_WRITE(SYSCON_DATE_REG - 4, RGB_QEMU_ORIGIN);
    uint32_t qemu_origin = REG_READ(SYSCON_DATE_REG - 4);
    ESP_LOGI(TAG, "QEMU origin register: 0x%08lx", (unsigned long)qemu_origin);
    
    ESP_LOGI(TAG, "Install QEMU RGB LCD panel driver");
    esp_lcd_rgb_qemu_config_t panel_config = {
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
        .bpp = RGB_QEMU_BPP_16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_qemu(&panel_config, &panel_handle));
    
#else
    // Hardware NV3052C panel setup
    ESP_LOGI(TAG, "Installing NV3052C RGB LCD panel driver");
    
    // Check if pins are configured
    if (HW_LCD_SPI_CS_GPIO == -1 || HW_LCD_RGB_PCLK_GPIO == -1) {
        ESP_LOGE(TAG, "Hardware pins not configured! Please update hardware_config.h");
        ESP_LOGE(TAG, "See NV3052C datasheet for your specific panel pinout");
        return NULL;
    }
    
    // Install 3-wire SPI panel IO for sending commands
    ESP_LOGI(TAG, "Install 3-wire SPI panel IO");
    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO,
        .cs_gpio_num = HW_LCD_SPI_CS_GPIO,
        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = HW_LCD_SPI_SCK_GPIO,
        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = HW_LCD_SPI_SDO_GPIO,
        .io_expander = NULL,
    };
    
    esp_lcd_panel_io_3wire_spi_config_t io_config = NV3052_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));
    
    // Configure RGB panel timing
    ESP_LOGI(TAG, "Configure RGB panel");
    esp_lcd_rgb_panel_config_t rgb_config = {};
    rgb_config.clk_src = LCD_CLK_SRC_DEFAULT;
    rgb_config.psram_trans_align = 64;
    rgb_config.data_width = 8;
    rgb_config.bits_per_pixel = 24;
    rgb_config.de_gpio_num = HW_LCD_RGB_DE_GPIO;
    rgb_config.pclk_gpio_num = HW_LCD_RGB_PCLK_GPIO;
    rgb_config.vsync_gpio_num = HW_LCD_RGB_VSYNC_GPIO;
    rgb_config.hsync_gpio_num = HW_LCD_RGB_HSYNC_GPIO;
    rgb_config.disp_gpio_num = HW_LCD_RGB_DISP_GPIO;
    rgb_config.data_gpio_nums[0] = HW_LCD_RGB_DATA0_GPIO;
    rgb_config.data_gpio_nums[1] = HW_LCD_RGB_DATA1_GPIO;
    rgb_config.data_gpio_nums[2] = HW_LCD_RGB_DATA2_GPIO;
    rgb_config.data_gpio_nums[3] = HW_LCD_RGB_DATA3_GPIO;
    rgb_config.data_gpio_nums[4] = HW_LCD_RGB_DATA4_GPIO;
    rgb_config.data_gpio_nums[5] = HW_LCD_RGB_DATA5_GPIO;
    rgb_config.data_gpio_nums[6] = HW_LCD_RGB_DATA6_GPIO;
    rgb_config.data_gpio_nums[7] = HW_LCD_RGB_DATA7_GPIO;
    // Use PSRAM for frame buffers since we have 8MB available
    rgb_config.flags.fb_in_psram = 1;
    // Timing from NV3052_720_1280_PANEL_30HZ_RGB_TIMING macro
    rgb_config.timings.pclk_hz = 18000000;
    rgb_config.timings.h_res = 720;
    rgb_config.timings.v_res = 1280;
    rgb_config.timings.hsync_pulse_width = 10;
    rgb_config.timings.hsync_back_porch = 20;
    rgb_config.timings.hsync_front_porch = 10;
    rgb_config.timings.vsync_pulse_width = 10;
    rgb_config.timings.vsync_back_porch = 20;
    rgb_config.timings.vsync_front_porch = 10;
    rgb_config.timings.flags.pclk_active_neg = false;
    
    nv3052_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,
        .flags = {
            .mirror_by_cmd = 1,
            .enable_io_multiplex = 0,
        },
    };
    
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = HW_LCD_RST_GPIO,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 24,
        .vendor_config = &vendor_config,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_nv3052(io_handle, &panel_config, &panel_handle));
#endif
    
    ESP_LOGI(TAG, "Initialize LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    
    // Turn on the display
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    
    // Enable backlight/display GPIO (GPIO 38)
    ESP_ERROR_CHECK(gpio_reset_pin((gpio_num_t)HW_LCD_RGB_DISP_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)HW_LCD_RGB_DISP_GPIO, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)HW_LCD_RGB_DISP_GPIO, 1));
    ESP_LOGI(TAG, "Display backlight enabled on GPIO %d", HW_LCD_RGB_DISP_GPIO);
    
    // Start RGB panel to begin data transmission
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_restart(panel_handle));
    ESP_LOGI(TAG, "RGB panel started");
    
    return panel_handle;
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Digi-Dash ESP32-S3 Firmware starting...");
    
#if USE_QEMU
    ESP_LOGI(TAG, "Running in QEMU mode");
#else
    ESP_LOGI(TAG, "Running on hardware with NV3052C display");
#endif
    
    // Initialize display
    esp_lcd_panel_handle_t panel_handle = init_display_panel();
    if (!panel_handle) {
        ESP_LOGE(TAG, "Failed to initialize display panel");
        return;
    }
    
    // Get the framebuffer pointer from the RGB panel
    uint16_t *fb = NULL;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 1, (void**)&fb));
    ESP_LOGI(TAG, "Got RGB panel framebuffer: fb=%p", fb);
    
    // Fill entire screen with white to confirm display is working
    ESP_LOGI(TAG, "Filling display with white");
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        fb[i] = 0xFFFF;  // White in RGB565
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Show white for 2 seconds
    
    // Initialize SPIFFS
    init_spiffs();
    
    // Allocate RGBA tile buffer for rendering to intermediate buffer during conversion
    const int TILE_HEIGHT = 60;
    uint8_t* rgba_buffer = (uint8_t*)heap_caps_malloc(DISPLAY_WIDTH * TILE_HEIGHT * 4, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (!rgba_buffer) {
        ESP_LOGE(TAG, "Failed to allocate RGBA tile buffer");
        return;
    }
    
    // Initialize gauge scene
    digidash::GaugeScene gauge_scene;
    
    // Load gauge file
    const char* gauge_path = "/spiffs/dashboard_tiny.gauge";
    FILE* gauge_file = fopen(gauge_path, "rb");
    
    if (!gauge_file) {
        ESP_LOGE(TAG, "Failed to open gauge file: %s", gauge_path);
        free(rgba_buffer);
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
        free(rgba_buffer);
        return;
    }
    
    size_t read_size = fread(gauge_data, 1, gauge_size, gauge_file);
    fclose(gauge_file);
    
    if (read_size != gauge_size) {
        ESP_LOGE(TAG, "Failed to read gauge file");
        free(gauge_data);
        free(rgba_buffer);
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
            // Render frame directly to the RGB framebuffer
            // First, clear the buffer to black
            memset(fb, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t));
            
            // Render all tiles to the framebuffer
            for (int tile = 0; tile < num_tiles; tile++) {
                int tile_y = tile * TILE_HEIGHT;
                int tile_h = (tile_y + TILE_HEIGHT > DISPLAY_HEIGHT) ? (DISPLAY_HEIGHT - tile_y) : TILE_HEIGHT;
                
                // Clear RGBA tile buffer
                std::memset(rgba_buffer, 0, DISPLAY_WIDTH * tile_h * 4);
                
                // Render this tile
                gauge_scene.render(rgba_buffer, DISPLAY_WIDTH, tile_h, DISPLAY_WIDTH * 4, tile_y);
                
                // Write RGBA data directly to RGB565 in the framebuffer
                for (int y = 0; y < tile_h; y++) {
                    for (int x = 0; x < DISPLAY_WIDTH; x++) {
                        int rgba_idx = (y * DISPLAY_WIDTH + x) * 4;
                        int fb_idx = (tile_y + y) * DISPLAY_WIDTH + x;
                        
                        uint8_t r = rgba_buffer[rgba_idx + 0];
                        uint8_t g = rgba_buffer[rgba_idx + 1];
                        uint8_t b = rgba_buffer[rgba_idx + 2];
                        uint8_t a = rgba_buffer[rgba_idx + 3];
                        
                        // Only write if alpha > 0 (blend with black background)
                        if (a > 0) {
                            fb[fb_idx] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                        }
                    }
                }
            }
            
            frame_count++;
            if (frame_count % 30 == 0) {
                ESP_LOGI(TAG, "Frame %d rendered to framebuffer", frame_count);
            }
            
            vTaskDelay(33 / portTICK_PERIOD_MS);  // ~30 FPS
        }
    } else {
        ESP_LOGE(TAG, "Failed to parse gauge file");
    }
    
    free(gauge_data);
    free(rgba_buffer);
}
