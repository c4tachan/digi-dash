#include "display_driver.h"
#include "qualia_display_config.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdlib>
#include <cstring>
#include <inttypes.h>

static const char* TAG = "DisplayDriver";

namespace digidash {

DisplayDriver::DisplayDriver(uint32_t width, uint32_t height)
    : width_(width)
    , height_(height)
    , panel_handle_(nullptr)
    , i2c_bus_handle_(nullptr)
    , initialized_(false)
    , framebuffer0_(nullptr)
    , framebuffer1_(nullptr)
    , framebuffer2_(nullptr)
    , framebuffer_(nullptr)
    , num_framebuffers_(0)
    , render_fb_index_(0) {
}

DisplayDriver::~DisplayDriver() {
    if (panel_handle_) {
        esp_lcd_panel_del(panel_handle_);
    }
    if (i2c_bus_handle_) {
        i2c_del_master_bus(i2c_bus_handle_);
    }
}

bool DisplayDriver::init_i2c_bus() {
    ESP_LOGI(TAG, "Initializing I2C bus (SDA=%d, SCL=%d)", 
             QUALIA_PIN_NUM_I2C_SDA, QUALIA_PIN_NUM_I2C_SCL);
    
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = static_cast<gpio_num_t>(QUALIA_PIN_NUM_I2C_SDA),
        .scl_io_num = static_cast<gpio_num_t>(QUALIA_PIN_NUM_I2C_SCL),
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = 1,
        },
    };
    
    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "I2C bus initialized successfully");
    return true;
}

bool DisplayDriver::init_pca9554() {
    ESP_LOGI(TAG, "Initializing PCA9554 GPIO expander");
    
    pca_expander_ = std::make_unique<Pca9554Expander>(i2c_bus_handle_, 0x3F);
    
    esp_err_t ret = pca_expander_->initialize();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PCA9554");
        return false;
    }
    
    ESP_LOGI(TAG, "PCA9554 initialized successfully");
    return true;
}

bool DisplayDriver::init_tft_controller() {
    ESP_LOGI(TAG, "Initializing NV3052C TFT controller");
    
    tft_init_ = std::make_unique<Nv3052cTftInit>(*pca_expander_);
    
    esp_err_t ret = tft_init_->initialize();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TFT controller");
        return false;
    }
    
    ESP_LOGI(TAG, "TFT controller initialized successfully");
    return true;
}

bool DisplayDriver::init_rgb_panel() {
    ESP_LOGI(TAG, "Configuring RGB LCD panel driver (%ux%u, %u MHz)", 
             (unsigned)width_, (unsigned)height_, 
             QUALIA_LCD_PIXEL_CLOCK / 1000000);
    
    // Log memory state before panel creation
    ESP_LOGI(TAG, "PSRAM total: %u bytes", (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "PSRAM free: %u bytes", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = QUALIA_LCD_PIXEL_CLOCK,
            .h_res = QUALIA_LCD_H_RES,
            .v_res = QUALIA_LCD_V_RES,
            .hsync_pulse_width = QUALIA_LCD_HSYNC_PULSE_WIDTH,
            .hsync_back_porch = QUALIA_LCD_HSYNC_BACK_PORCH,
            .hsync_front_porch = QUALIA_LCD_HSYNC_FRONT_PORCH,
            .vsync_pulse_width = QUALIA_LCD_VSYNC_PULSE_WIDTH,
            .vsync_back_porch = QUALIA_LCD_VSYNC_BACK_PORCH,
            .vsync_front_porch = QUALIA_LCD_VSYNC_FRONT_PORCH,
            .flags = {
                .hsync_idle_low = static_cast<unsigned int>(QUALIA_LCD_HSYNC_POLARITY),
                .vsync_idle_low = static_cast<unsigned int>(QUALIA_LCD_VSYNC_POLARITY),
                .de_idle_high = static_cast<unsigned int>(QUALIA_LCD_DE_POLARITY),
                .pclk_active_neg = static_cast<unsigned int>(QUALIA_LCD_PCLK_ACTIVE_NEG),
                .pclk_idle_high = 0,
            },
        },
        .data_width = 16,  // RGB565 mode
        .bits_per_pixel = 16,
        .num_fbs = 3,  // Triple framebuffer for smoother producer/consumer decoupling
        .bounce_buffer_size_px = 0,
        .sram_trans_align = 8,
        .psram_trans_align = 64,
        .hsync_gpio_num = static_cast<int>(QUALIA_PIN_NUM_HSYNC),
        .vsync_gpio_num = static_cast<int>(QUALIA_PIN_NUM_VSYNC),
        .de_gpio_num = static_cast<int>(QUALIA_PIN_NUM_DE),
        .pclk_gpio_num = static_cast<int>(QUALIA_PIN_NUM_PCLK),
        .disp_gpio_num = -1,  // Not used
        .data_gpio_nums = {
            QUALIA_PIN_NUM_DATA0,   // B1
            QUALIA_PIN_NUM_DATA1,   // B2
            QUALIA_PIN_NUM_DATA2,   // B3
            QUALIA_PIN_NUM_DATA3,   // B4
            QUALIA_PIN_NUM_DATA4,   // B5
            QUALIA_PIN_NUM_DATA5,   // G0
            QUALIA_PIN_NUM_DATA6,   // G1
            QUALIA_PIN_NUM_DATA7,   // G2
            QUALIA_PIN_NUM_DATA8,   // G3
            QUALIA_PIN_NUM_DATA9,   // G4
            QUALIA_PIN_NUM_DATA10,  // G5
            QUALIA_PIN_NUM_DATA11,  // R1
            QUALIA_PIN_NUM_DATA12,  // R2
            QUALIA_PIN_NUM_DATA13,  // R3
            QUALIA_PIN_NUM_DATA14,  // R4
            QUALIA_PIN_NUM_DATA15,  // R5
        },
        .flags = {
            .disp_active_low = 1,
            .refresh_on_demand = 0,
            .fb_in_psram = 1,      // Framebuffers in PSRAM
            .double_fb = 0,        // Explicitly use num_fbs
            .no_fb = 0,            // Driver manages framebuffers
        },
    };
    
    ESP_LOGI(TAG, "Creating RGB panel with triple framebuffer mode");
    esp_err_t ret = esp_lcd_new_rgb_panel(&panel_config, &panel_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RGB panel: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Get framebuffer pointers from driver
    ret = esp_lcd_rgb_panel_get_frame_buffer(panel_handle_, 3, &framebuffer0_, &framebuffer1_, &framebuffer2_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get framebuffer pointers: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "Framebuffer 0 address: %p", framebuffer0_);
    ESP_LOGI(TAG, "Framebuffer 1 address: %p", framebuffer1_);
    ESP_LOGI(TAG, "Framebuffer 2 address: %p", framebuffer2_);
    
    if (!framebuffer0_ || !framebuffer1_ || !framebuffer2_) {
        ESP_LOGE(TAG, "Invalid framebuffer pointers");
        return false;
    }
    
    num_framebuffers_ = 3;
    render_fb_index_ = 1;
    framebuffer_ = static_cast<uint8_t*>(framebuffer1_);
    
    // Reset and initialize panel
    ESP_LOGI(TAG, "Resetting and initializing RGB panel");
    ret = esp_lcd_panel_reset(panel_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel reset failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = esp_lcd_panel_init(panel_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RGB panel: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "RGB panel initialized successfully");
    return true;
}

void DisplayDriver::enable_backlight() {
    ESP_LOGI(TAG, "Enabling backlight via PCA9554");
    tft_init_->enable_backlight();
    vTaskDelay(pdMS_TO_TICKS(100));
}

bool DisplayDriver::initialize() {
    if (initialized_) {
        ESP_LOGW(TAG, "Display already initialized");
        return true;
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Starting Display Initialization");
    ESP_LOGI(TAG, "========================================");
    
    // Step 1: Initialize I2C bus for PCA9554
    ESP_LOGI(TAG, "Step 1/5: Initialize I2C bus");
    if (!init_i2c_bus()) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus");
        return false;
    }
    
    // Step 2: Initialize PCA9554 GPIO expander
    ESP_LOGI(TAG, "Step 2/5: Initialize PCA9554 GPIO expander");
    if (!init_pca9554()) {
        ESP_LOGE(TAG, "Failed to initialize PCA9554");
        return false;
    }
    
    // Step 3: Initialize TFT controller via bit-banged SPI
    ESP_LOGI(TAG, "Step 3/5: Initialize NV3052C TFT controller");
    if (!init_tft_controller()) {
        ESP_LOGE(TAG, "Failed to initialize TFT controller");
        return false;
    }
    
    // Step 4: Initialize RGB panel with double framebuffer
    ESP_LOGI(TAG, "Step 4/5: Initialize RGB panel");
    if (!init_rgb_panel()) {
        ESP_LOGE(TAG, "Failed to initialize RGB panel");
        return false;
    }
    
    // Step 5: Enable backlight
    ESP_LOGI(TAG, "Step 5/5: Enable backlight");
    enable_backlight();
    
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
    
    // Log any out-of-bounds calls
    if (x_end > width_ || y_end > height_) {
        ESP_LOGW(TAG, "draw_bitmap called with out-of-bounds: x_start=%" PRIu32 ", y_start=%" PRIu32 ", x_end=%" PRIu32 ", y_end=%" PRIu32 " (display is %" PRIu32 "x%" PRIu32 ")",
                 x_start, y_start, x_end, y_end, width_, height_);
    }

    if (num_framebuffers_ < 1) {
        ESP_LOGE(TAG, "No framebuffers available");
        return;
    }

    void* render_fb = nullptr;
    if (render_fb_index_ == 0) {
        render_fb = framebuffer0_;
    } else if (render_fb_index_ == 1) {
        render_fb = framebuffer1_;
    } else {
        render_fb = framebuffer2_;
    }

    uint16_t* dst = static_cast<uint16_t*>(render_fb);
    const uint16_t* src = static_cast<const uint16_t*>(color_data);

    if (x_start >= width_ || y_start >= height_) {
        return;  // Completely off screen
    }

    uint32_t tile_width = x_end - x_start;
    uint32_t tile_height = y_end - y_start;

    if (x_end > width_) {
        tile_width = width_ - x_start;
    }
    if (y_end > height_) {
        tile_height = height_ - y_start;
    }

    for (uint32_t row = 0; row < tile_height; row++) {
        uint32_t y = y_start + row;
        if (y >= height_) break;

        uint32_t fb_offset = y * width_ + x_start;
        uint32_t src_offset = row * (x_end - x_start);

        if (fb_offset + tile_width <= width_ * height_) {
            std::memcpy(&dst[fb_offset], &src[src_offset], tile_width * sizeof(uint16_t));
        }
    }

    esp_err_t ret = esp_lcd_panel_draw_bitmap(panel_handle_, x_start, y_start, x_end, y_end, render_fb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_panel_draw_bitmap failed: %s", esp_err_to_name(ret));
        return;
    }

    render_fb_index_ = (render_fb_index_ + 1) % num_framebuffers_;
    if (render_fb_index_ == 0) {
        framebuffer_ = static_cast<uint8_t*>(framebuffer0_);
    } else if (render_fb_index_ == 1) {
        framebuffer_ = static_cast<uint8_t*>(framebuffer1_);
    } else {
        framebuffer_ = static_cast<uint8_t*>(framebuffer2_);
    }
}

void DisplayDriver::refresh() {
    // RGB LCD panel updates automatically via DMA in continuous mode
}

bool DisplayDriver::set_pixel_clock(uint32_t hz) {
    if (!initialized_ || !panel_handle_) {
        return false;
    }

    esp_err_t ret = esp_lcd_rgb_panel_set_pclk(panel_handle_, hz);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set pixel clock to %u Hz: %s", (unsigned)hz, esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(TAG, "Requested pixel clock change to %u Hz", (unsigned)hz);
    return true;
}

void DisplayDriver::unlock_and_update() {
    if (!initialized_ || !panel_handle_ || !framebuffer_) {
        return;
    }

    esp_err_t ret = esp_lcd_panel_draw_bitmap(panel_handle_, 0, 0, width_, height_, framebuffer_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "unlock_and_update draw failed: %s", esp_err_to_name(ret));
        return;
    }

    render_fb_index_ = (render_fb_index_ + 1) % num_framebuffers_;
    if (render_fb_index_ == 0) {
        framebuffer_ = static_cast<uint8_t*>(framebuffer0_);
    } else if (render_fb_index_ == 1) {
        framebuffer_ = static_cast<uint8_t*>(framebuffer1_);
    } else {
        framebuffer_ = static_cast<uint8_t*>(framebuffer2_);
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

    // Fill all framebuffers
    uint16_t* fb0 = static_cast<uint16_t*>(framebuffer0_);
    uint16_t* fb1 = static_cast<uint16_t*>(framebuffer1_);
    uint16_t* fb2 = static_cast<uint16_t*>(framebuffer2_);
    size_t pixel_count = width_ * height_;
    
    for (size_t i = 0; i < pixel_count; ++i) {
        fb0[i] = rgb565;
        fb1[i] = rgb565;
        fb2[i] = rgb565;
    }

}

void DisplayDriver::test_pattern_solid_red() {
    if (!initialized_) {
        ESP_LOGE(TAG, "Display not initialized for test pattern");
        return;
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "TEST PATTERN: Solid RED (0xF800)");
    ESP_LOGI(TAG, "========================================");
    
    // Fill all framebuffers with RED (RGB565: 0xF800)
    uint16_t* fb0 = static_cast<uint16_t*>(framebuffer0_);
    uint16_t* fb1 = static_cast<uint16_t*>(framebuffer1_);
    uint16_t* fb2 = static_cast<uint16_t*>(framebuffer2_);
    size_t pixel_count = width_ * height_;
    uint16_t red = 0xF800;
    
    ESP_LOGI(TAG, "Filling %zu pixels with 0xF800 (red)", pixel_count);
    for (size_t i = 0; i < pixel_count; ++i) {
        fb0[i] = red;
        fb1[i] = red;
        fb2[i] = red;
    }
    
    ESP_LOGI(TAG, "Framebuffers filled with red");
    
    // Verify
    ESP_LOGI(TAG, "FB0[0] = 0x%04X (should be 0xF800)", fb0[0]);
    ESP_LOGI(TAG, "FB0[%zu] = 0x%04X (middle)", pixel_count/2, fb0[pixel_count/2]);
    ESP_LOGI(TAG, "FB1[0] = 0x%04X (should be 0xF800)", fb1[0]);
    
    ESP_LOGI(TAG, "Red test pattern complete - check display");
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void DisplayDriver::test_pattern_solid_green() {
    if (!initialized_) {
        ESP_LOGE(TAG, "Display not initialized for test pattern");
        return;
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "TEST PATTERN: Solid GREEN (0x07E0)");
    ESP_LOGI(TAG, "========================================");
    
    // Fill all framebuffers with GREEN (RGB565: 0x07E0)
    uint16_t* fb0 = static_cast<uint16_t*>(framebuffer0_);
    uint16_t* fb1 = static_cast<uint16_t*>(framebuffer1_);
    uint16_t* fb2 = static_cast<uint16_t*>(framebuffer2_);
    size_t pixel_count = width_ * height_;
    uint16_t green = 0x07E0;
    
    for (size_t i = 0; i < pixel_count; ++i) {
        fb0[i] = green;
        fb1[i] = green;
        fb2[i] = green;
    }
    
    ESP_LOGI(TAG, "Green test pattern complete - check display");
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void DisplayDriver::test_pattern_solid_blue() {
    if (!initialized_) {
        ESP_LOGE(TAG, "Display not initialized for test pattern");
        return;
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "TEST PATTERN: Solid BLUE (0x001F)");
    ESP_LOGI(TAG, "========================================");
    
    // Fill all framebuffers with BLUE (RGB565: 0x001F)
    uint16_t* fb0 = static_cast<uint16_t*>(framebuffer0_);
    uint16_t* fb1 = static_cast<uint16_t*>(framebuffer1_);
    uint16_t* fb2 = static_cast<uint16_t*>(framebuffer2_);
    size_t pixel_count = width_ * height_;
    uint16_t blue = 0x001F;
    
    for (size_t i = 0; i < pixel_count; ++i) {
        fb0[i] = blue;
        fb1[i] = blue;
        fb2[i] = blue;
    }
    
    ESP_LOGI(TAG, "Blue test pattern complete - check display");
    vTaskDelay(pdMS_TO_TICKS(2000));
}

} // namespace digidash
