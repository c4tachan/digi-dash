#include "application.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

static const char* TAG = "Application";

namespace digidash {

// Display configuration
static constexpr uint32_t DISPLAY_WIDTH = 720;
static constexpr uint32_t DISPLAY_HEIGHT = 720;
static constexpr uint32_t TILE_HEIGHT = 60;

// Gauge file path
static constexpr const char* GAUGE_FILE_PATH = "/spiffs/dashboard_tiny.gauge";

// Frame rate
static constexpr uint32_t TARGET_FPS = 30;
static constexpr uint32_t FRAME_DELAY_MS = 1000 / TARGET_FPS;

Application::Application()
    : display_(nullptr)
    , storage_(nullptr)
    , renderer_(nullptr)
    , initialized_(false) {
}

Application::~Application() {
}

void Application::display_hello_world() {
    ESP_LOGI(TAG, "Displaying 'Hello World'...");
    
    // Allocate RGBA buffer for full display
    size_t buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * 4;
    uint8_t* rgba_buffer = (uint8_t*)malloc(buffer_size);
    if (!rgba_buffer) {
        ESP_LOGE(TAG, "Failed to allocate buffer for Hello World");
        return;
    }
    
    // Clear buffer to black
    std::memset(rgba_buffer, 0, buffer_size);
    
    // Draw "Hello World" text in white, centered
    TextRenderer::draw_text("Hello World",
                           DISPLAY_WIDTH / 2 - 32,  // Center X (64 pixels wide for "Hello World")
                           DISPLAY_HEIGHT / 2 - 4,   // Center Y
                           rgba_buffer,
                           DISPLAY_WIDTH, DISPLAY_HEIGHT,
                           DISPLAY_WIDTH * 4,
                           255, 255, 255);  // White color
    
    // Convert RGBA to RGB565 and display
    size_t rgb565_buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2;
    uint16_t* rgb565_buffer = (uint16_t*)malloc(rgb565_buffer_size);
    if (rgb565_buffer) {
        // Convert RGBA to RGB565
        for (size_t i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; ++i) {
            uint8_t r = rgba_buffer[i * 4 + 0];
            uint8_t g = rgba_buffer[i * 4 + 1];
            uint8_t b = rgba_buffer[i * 4 + 2];
            rgb565_buffer[i] = static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
        }
        
        // Display on screen
        display_->draw_bitmap(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, rgb565_buffer);
        
        free(rgb565_buffer);
    }
    
    free(rgba_buffer);
    ESP_LOGI(TAG, "Hello World displayed!");
}

bool Application::initialize() {
    if (initialized_) {
        ESP_LOGW(TAG, "Application already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing Digi-Dash Application...");

    // Initialize display
    ESP_LOGI(TAG, "Step 1/4: Initializing display subsystem");
    display_ = std::make_unique<DisplayDriver>(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!display_->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize display");
        return false;
    }

    // Initialize storage
    ESP_LOGI(TAG, "Step 2/4: Initializing storage subsystem");
    storage_ = std::make_unique<StorageManager>();
    if (!storage_->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize storage");
        return false;
    }

    // Initialize renderer
    ESP_LOGI(TAG, "Step 3/4: Initializing rendering subsystem");
    renderer_ = std::make_unique<RenderEngine>(*display_, TILE_HEIGHT);
    if (!renderer_->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize renderer");
        return false;
    }

    // Load gauge file
    ESP_LOGI(TAG, "Step 4/4: Loading gauge asset");
    std::vector<uint8_t> gauge_data;
    if (!storage_->read_file(GAUGE_FILE_PATH, gauge_data)) {
        ESP_LOGE(TAG, "Failed to load gauge file: %s", GAUGE_FILE_PATH);
        return false;
    }

    if (!renderer_->load_gauge(gauge_data.data(), gauge_data.size())) {
        ESP_LOGE(TAG, "Failed to load gauge into renderer");
        return false;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "Application initialized successfully!");
    return true;
}

void Application::run() {
    if (!initialized_) {
        ESP_LOGE(TAG, "Cannot run: application not initialized");
        return;
    }

    ESP_LOGI(TAG, "Starting main render loop...");
    
    // Display "Hello World" on startup
    display_hello_world();

    while (true) {
        renderer_->render_frame();
        vTaskDelay(FRAME_DELAY_MS / portTICK_PERIOD_MS);
    }
}

} // namespace digidash
