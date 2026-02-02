#include "display_driver.h"
#include "qualia_display_config.h"
#include "nv3052c_driver.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_io.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdlib>
#include <cstring>

static const char* TAG = "DisplayDriver";

namespace digidash {

DisplayDriver::DisplayDriver(uint32_t width, uint32_t height)
    : width_(width)
    , height_(height)
    , panel_handle_(nullptr)
    , panel_io_handle_(nullptr)
    , initialized_(false)
    , framebuffer_(nullptr) {
}

DisplayDriver::~DisplayDriver() {
    if (framebuffer_) {
        free(framebuffer_);
    }
    if (panel_handle_) {
        esp_lcd_panel_del(panel_handle_);
    }
    if (panel_io_handle_) {
        esp_lcd_panel_io_del(panel_io_handle_);
    }
}

bool DisplayDriver::init_backlight() {
    ESP_LOGI(TAG, "Initializing backlight GPIO (pin %d)", QUALIA_PIN_NUM_BL);
    
    gpio_config_t bl_gpio_config = {};
    bl_gpio_config.pin_bit_mask = 1ULL << QUALIA_PIN_NUM_BL;
    bl_gpio_config.mode = GPIO_MODE_OUTPUT;
    bl_gpio_config.pull_up_en = GPIO_PULLUP_DISABLE;
    bl_gpio_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    bl_gpio_config.intr_type = GPIO_INTR_DISABLE;
    
    esp_err_t ret = gpio_config(&bl_gpio_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure backlight GPIO: %s", esp_err_to_name(ret));
        return false;
    }
    
    return true;
}

bool DisplayDriver::enable_backlight() {
    ESP_LOGI(TAG, "Enabling backlight (setting GPIO %d HIGH)", QUALIA_PIN_NUM_BL);
    
    esp_err_t ret = gpio_set_level((gpio_num_t)QUALIA_PIN_NUM_BL, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable backlight: %s", esp_err_to_name(ret));
        return false;
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Wait for backlight to stabilize
    ESP_LOGI(TAG, "Backlight enabled successfully");
    return true;
}

bool DisplayDriver::allocate_framebuffer() {
    // Calculate framebuffer size: 720x720 pixels × 2 bytes/pixel (RGB565)
    size_t fb_size = width_ * height_ * 2;
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Attempting framebuffer allocation:");
    ESP_LOGI(TAG, "  Requested: %u bytes (%u KB)", fb_size, fb_size / 1024);
    ESP_LOGI(TAG, "  Total PSRAM free: %u bytes (%u KB)", 
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
    ESP_LOGI(TAG, "  Largest free block: %u bytes (%u KB)",
             heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
             heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024);
    ESP_LOGI(TAG, "========================================");
    
    // Try allocation with SPIRAM only (no 8BIT requirement)
    framebuffer_ = (uint8_t*)heap_caps_malloc(fb_size, MALLOC_CAP_SPIRAM);
    
    if (!framebuffer_) {
        ESP_LOGE(TAG, "Primary allocation (MALLOC_CAP_SPIRAM) failed");
        
        // Try with prefer fallback
        ESP_LOGI(TAG, "Trying heap_caps_malloc_prefer with fallback...");
        framebuffer_ = (uint8_t*)heap_caps_malloc_prefer(
            fb_size,
            2,                      // 2 preferences
            MALLOC_CAP_SPIRAM,      // prefer PSRAM
            MALLOC_CAP_8BIT         // fallback to internal RAM
        );
        
        if (!framebuffer_) {
            ESP_LOGE(TAG, "All allocation attempts failed!");
            ESP_LOGE(TAG, "Cannot proceed without framebuffer");
            return false;
        }
        
        // Check where it landed
        if (esp_ptr_external_ram(framebuffer_)) {
            ESP_LOGI(TAG, "Framebuffer allocated in PSRAM (via fallback)");
        } else {
            ESP_LOGW(TAG, "Framebuffer allocated in INTERNAL RAM (not ideal for size!)");
        }
    } else {
        ESP_LOGI(TAG, "Framebuffer allocated successfully in PSRAM");
    }
    
    ESP_LOGI(TAG, "Framebuffer address: %p", framebuffer_);
    ESP_LOGI(TAG, "Is external RAM: %s", esp_ptr_external_ram(framebuffer_) ? "YES" : "NO");
    
    // Clear framebuffer to black (this will take a moment for 1MB)
    ESP_LOGI(TAG, "Clearing framebuffer...");
    memset(framebuffer_, 0, fb_size);
    ESP_LOGI(TAG, "Framebuffer cleared successfully");
    
    return true;
}

bool DisplayDriver::initialize() {
    if (initialized_) {
        ESP_LOGW(TAG, "Display already initialized");
        return true;
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Starting NV3052C 720x720 RGB666 Display Initialization");
    ESP_LOGI(TAG, "========================================");
    
    // Step 0: Allocate framebuffer FIRST, before any other PSRAM allocations
    // This maximizes chances of getting a large contiguous block
    ESP_LOGI(TAG, "Step 0/8: Allocate framebuffer (EARLY to avoid fragmentation)");
    if (!allocate_framebuffer()) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer - cannot continue");
        return false;
    }
    
    // Step 1: Initialize backlight GPIO
    ESP_LOGI(TAG, "Step 1/8: Initialize backlight GPIO");
    if (!init_backlight()) {
        ESP_LOGE(TAG, "Failed to initialize backlight GPIO");
        return false;
    }
    
    // Step 2: Initialize SPI interface for NV3052C controller
    ESP_LOGI(TAG, "Step 2/8: Initialize SPI interface for NV3052C");
    
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = QUALIA_PIN_NUM_LCD_SCK;
    buscfg.mosi_io_num = QUALIA_PIN_NUM_LCD_MOSI;
    buscfg.miso_io_num = -1;  // Not used
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 32;  // Small transfers for commands
    
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return false;
    }
    
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = QUALIA_PIN_NUM_LCD_CS;
    io_config.dc_gpio_num = QUALIA_PIN_NUM_LCD_DC;
    io_config.spi_mode = 0;
    io_config.pclk_hz = 10 * 1000 * 1000;  // 10MHz for command interface
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &panel_io_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return false;
    }
    
    // Step 3: Hardware reset NV3052C
    ESP_LOGI(TAG, "Step 3/5: Hardware reset NV3052C");
    gpio_config_t rst_cfg = {};
    rst_cfg.pin_bit_mask = (1ULL << QUALIA_PIN_NUM_LCD_RST);
    rst_cfg.mode = GPIO_MODE_OUTPUT;
    gpio_config(&rst_cfg);
    
    gpio_set_level((gpio_num_t)QUALIA_PIN_NUM_LCD_RST, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level((gpio_num_t)QUALIA_PIN_NUM_LCD_RST, 1);
    vTaskDelay(120 / portTICK_PERIOD_MS);
    
    // Step 4: Initialize NV3052C controller
    ESP_LOGI(TAG, "Step 4/5: Initialize NV3052C controller");
    ret = Nv3052cDriver::init(panel_io_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NV3052C: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Step 5: Create RGB panel with correct configuration
    ESP_LOGI(TAG, "Step 5/7: Configure RGB LCD panel driver (%ux%u, %u MHz)", 
             (unsigned)width_, (unsigned)height_, 
             QUALIA_LCD_PIXEL_CLOCK / 1000000);
    
    esp_lcd_rgb_panel_config_t panel_config = {};
    panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
    panel_config.timings.pclk_hz = QUALIA_LCD_PIXEL_CLOCK;
    panel_config.timings.h_res = QUALIA_LCD_H_RES;
    panel_config.timings.v_res = QUALIA_LCD_V_RES;
    
    // Timing parameters (DE mode)
    panel_config.timings.hsync_pulse_width = QUALIA_LCD_HSYNC_PULSE_WIDTH;
    panel_config.timings.hsync_back_porch = QUALIA_LCD_HSYNC_BACK_PORCH;
    panel_config.timings.hsync_front_porch = QUALIA_LCD_HSYNC_FRONT_PORCH;
    panel_config.timings.vsync_pulse_width = QUALIA_LCD_VSYNC_PULSE_WIDTH;
    panel_config.timings.vsync_back_porch = QUALIA_LCD_VSYNC_BACK_PORCH;
    panel_config.timings.vsync_front_porch = QUALIA_LCD_VSYNC_FRONT_PORCH;
    
    // Polarity settings
    panel_config.timings.flags.hsync_idle_low = QUALIA_LCD_HSYNC_POLARITY;
    panel_config.timings.flags.vsync_idle_low = QUALIA_LCD_VSYNC_POLARITY;
    panel_config.timings.flags.de_idle_high = !QUALIA_LCD_DE_POLARITY;
    panel_config.timings.flags.pclk_active_neg = QUALIA_LCD_PCLK_ACTIVE_NEG;
    
    // Data width and frame buffer settings
    panel_config.data_width = 16;  // RGB565 mode
    panel_config.bits_per_pixel = 16;
    panel_config.num_fbs = 0;  // No internal framebuffer - use bounce buffer only
    panel_config.bounce_buffer_size_px = 2 * QUALIA_LCD_H_RES;  // Bounce buffer in internal RAM (2 lines only, ~2.8KB)
    
    // GPIO assignments
    panel_config.hsync_gpio_num = QUALIA_PIN_NUM_HSYNC;
    panel_config.vsync_gpio_num = QUALIA_PIN_NUM_VSYNC;
    panel_config.de_gpio_num = QUALIA_PIN_NUM_DE;
    panel_config.pclk_gpio_num = QUALIA_PIN_NUM_PCLK;
    
    // Data pins (16 pins for RGB565: R3-R0, G5-G0, B5-B0)
    panel_config.data_gpio_nums[0] = QUALIA_PIN_NUM_DATA0;    // B0
    panel_config.data_gpio_nums[1] = QUALIA_PIN_NUM_DATA1;    // B1
    panel_config.data_gpio_nums[2] = QUALIA_PIN_NUM_DATA2;    // B2
    panel_config.data_gpio_nums[3] = QUALIA_PIN_NUM_DATA3;    // B3
    panel_config.data_gpio_nums[4] = QUALIA_PIN_NUM_DATA4;    // B4
    panel_config.data_gpio_nums[5] = QUALIA_PIN_NUM_DATA5;    // B5
    panel_config.data_gpio_nums[6] = QUALIA_PIN_NUM_DATA6;    // G0
    panel_config.data_gpio_nums[7] = QUALIA_PIN_NUM_DATA7;    // G1
    panel_config.data_gpio_nums[8] = QUALIA_PIN_NUM_DATA8;    // G2
    panel_config.data_gpio_nums[9] = QUALIA_PIN_NUM_DATA9;    // G3
    panel_config.data_gpio_nums[10] = QUALIA_PIN_NUM_DATA10;  // G4
    panel_config.data_gpio_nums[11] = QUALIA_PIN_NUM_DATA11;  // G5
    panel_config.data_gpio_nums[12] = QUALIA_PIN_NUM_DATA12;  // R0
    panel_config.data_gpio_nums[13] = QUALIA_PIN_NUM_DATA13;  // R1
    panel_config.data_gpio_nums[14] = QUALIA_PIN_NUM_DATA14;  // R2
    panel_config.data_gpio_nums[15] = QUALIA_PIN_NUM_DATA15;  // R3
    
    // No framebuffer mode - use bounce buffer only to avoid PSRAM allocation issues
    panel_config.flags.fb_in_psram = 0;
    panel_config.flags.no_fb = 1;  // Critical: tell driver not to allocate any framebuffer
    
    ESP_LOGI(TAG, "Creating RGB panel (no-FB mode with bounce buffer)");
    esp_err_t rgb_ret = esp_lcd_new_rgb_panel(&panel_config, &panel_handle_);
    if (rgb_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel: %s", esp_err_to_name(rgb_ret));
        return false;
    }
    
    // Step 7/8: Reset and initialize the RGB panel (starts DMA)
    ESP_LOGI(TAG, "Step 7/8: Reset and initialize RGB LCD panel");
    rgb_ret = esp_lcd_panel_reset(panel_handle_);
    if (rgb_ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel reset failed: %s", esp_err_to_name(rgb_ret));
        return false;
    }
    
    rgb_ret = esp_lcd_panel_init(panel_handle_);
    if (rgb_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RGB panel: %s", esp_err_to_name(rgb_ret));
        return false;
    }
    ESP_LOGI(TAG, "RGB panel initialized, DMA streaming active");
    
    // Step 8/8: Enable backlight
    ESP_LOGI(TAG, "Step 8/8: Enabling backlight");
    if (!enable_backlight()) {
        ESP_LOGE(TAG, "Failed to enable backlight");
        return false;
    }
    
    initialized_ = true;
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Display initialized successfully!");
    ESP_LOGI(TAG, "========================================");
    return true;
}

void DisplayDriver::draw_bitmap(uint32_t x_start, uint32_t y_start, uint32_t x_end, uint32_t y_end, const void* color_data) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Display not initialized");
        return;
    }
    
    esp_lcd_panel_draw_bitmap(panel_handle_, x_start, y_start, x_end, y_end, color_data);
}

void DisplayDriver::refresh() {
    // RGB LCD panel updates automatically via DMA
    // No explicit refresh needed for real hardware
}

void DisplayDriver::unlock_and_update() {
    if (!initialized_ || !framebuffer_) {
        ESP_LOGE(TAG, "Display not initialized or no framebuffer");
        return;
    }
    
    // In no-FB mode with bounce buffer, we need to send the framebuffer data 
    // to the panel in chunks that fit in the bounce buffer
    // Our bounce buffer is 10 lines, so send 10 lines at a time
    const uint32_t lines_per_chunk = 10;
    const size_t bytes_per_line = width_ * 2;  // RGB565 = 2 bytes per pixel
    
    for (uint32_t y = 0; y < height_; y += lines_per_chunk) {
        uint32_t lines_to_send = (y + lines_per_chunk > height_) ? (height_ - y) : lines_per_chunk;
        uint8_t* src = framebuffer_ + (y * bytes_per_line);
        
        esp_err_t ret = esp_lcd_panel_draw_bitmap(panel_handle_, 
                                                   0, y,                    // x_start, y_start
                                                   width_, y + lines_to_send,  // x_end, y_end
                                                   src);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to draw bitmap at line %u: %s", y, esp_err_to_name(ret));
            return;
        }
    }
}

void DisplayDriver::clear(uint32_t color) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Display not initialized");
        return;
    }

    // Convert 0xRRGGBB color to RGB565
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

    size_t row_bytes = width_ * sizeof(uint16_t);
    uint16_t* row = (uint16_t*)malloc(row_bytes);
    if (!row) {
        ESP_LOGE(TAG, "Failed to allocate row buffer for clear");
        return;
    }

    for (uint32_t i = 0; i < width_; ++i) {
        row[i] = rgb565;
    }

    for (uint32_t y = 0; y < height_; ++y) {
        esp_lcd_panel_draw_bitmap(panel_handle_, 0, y, width_, y + 1, row);
    }

    free(row);
}

void DisplayDriver::test_pattern_solid_red() {
    if (!initialized_ || !framebuffer_) {
        ESP_LOGE(TAG, "Display not initialized for test pattern");
        return;
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "TEST PATTERN: Solid RED");
    ESP_LOGI(TAG, "========================================");
    
    // Fill framebuffer with RED (RGB565: 0xF800)
    uint16_t* fb = (uint16_t*)framebuffer_;
    uint16_t red = 0xF800;
    size_t pixel_count = width_ * height_;
    
    for (size_t i = 0; i < pixel_count; ++i) {
        fb[i] = red;
    }
    
    // Send framebuffer to display
    unlock_and_update();
    
    ESP_LOGI(TAG, "Red test pattern complete - check if display shows RED");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

void DisplayDriver::test_pattern_solid_green() {
    if (!initialized_ || !framebuffer_) {
        ESP_LOGE(TAG, "Display not initialized for test pattern");
        return;
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "TEST PATTERN: Solid GREEN");
    ESP_LOGI(TAG, "========================================");
    
    // Fill framebuffer with GREEN (RGB565: 0x07E0)
    uint16_t* fb = (uint16_t*)framebuffer_;
    uint16_t green = 0x07E0;
    size_t pixel_count = width_ * height_;
    
    for (size_t i = 0; i < pixel_count; ++i) {
        fb[i] = green;
    }
    
    // Send framebuffer to display
    unlock_and_update();
    
    ESP_LOGI(TAG, "Green test pattern complete - check if display shows GREEN");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

void DisplayDriver::test_pattern_solid_blue() {
    if (!initialized_ || !framebuffer_) {
        ESP_LOGE(TAG, "Display not initialized for test pattern");
        return;
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "TEST PATTERN: Solid BLUE");
    ESP_LOGI(TAG, "========================================");
    
    // Fill framebuffer with BLUE (RGB565: 0x001F)
    uint16_t* fb = (uint16_t*)framebuffer_;
    uint16_t blue = 0x001F;
    size_t pixel_count = width_ * height_;
    
    for (size_t i = 0; i < pixel_count; ++i) {
        fb[i] = blue;
    }
    
    // Send framebuffer to display
    unlock_and_update();
    
    ESP_LOGI(TAG, "Blue test pattern complete - check if display shows BLUE");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

} // namespace digidash
