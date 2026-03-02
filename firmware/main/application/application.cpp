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
    , obd2_(nullptr)
    , initialized_(false) {
}

Application::~Application() {
}

void Application::display_hello_world() {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Running test patterns BEFORE LVGL...");
    ESP_LOGI(TAG, "========================================");
    
    // Run solid color test patterns
    ESP_LOGI(TAG, "Starting RED test pattern...");
    display_->test_pattern_solid_red();
    
    ESP_LOGI(TAG, "Starting GREEN test pattern...");
    display_->test_pattern_solid_green();
    
    ESP_LOGI(TAG, "Starting BLUE test pattern...");
    display_->test_pattern_solid_blue();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Test patterns complete!");
    ESP_LOGI(TAG, "If you see RED/GREEN/BLUE on the display,");
    ESP_LOGI(TAG, "the display is working correctly!");
    ESP_LOGI(TAG, "========================================");
    
    // Fill screen with RGB test pattern
    ESP_LOGI(TAG, "Displaying final RGB test pattern...");
    
    // Allocate RGB565 buffer directly
    size_t rgb565_buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2;
    uint16_t* rgb565_buffer = (uint16_t*)malloc(rgb565_buffer_size);
    if (!rgb565_buffer) {
        ESP_LOGE(TAG, "Failed to allocate RGB565 buffer");
        return;
    }
    
    // Fill screen with bright test pattern
    // Top half: RED, Bottom half: GREEN, Middle stripe: BLUE
    for (uint32_t y = 0; y < DISPLAY_HEIGHT; y++) {
        for (uint32_t x = 0; x < DISPLAY_WIDTH; x++) {
            uint16_t color;
            if (y < DISPLAY_HEIGHT / 3) {
                // Top third: RED (RGB565: 0xF800)
                color = 0xF800;
            } else if (y < 2 * DISPLAY_HEIGHT / 3) {
                // Middle third: GREEN (RGB565: 0x07E0)
                color = 0x07E0;
            } else {
                // Bottom third: BLUE (RGB565: 0x001F)
                color = 0x001F;
            }
            rgb565_buffer[y * DISPLAY_WIDTH + x] = color;
        }
    }
    
    // Add white border to verify exact display area
    for (uint32_t x = 0; x < DISPLAY_WIDTH; x++) {
        rgb565_buffer[x] = 0xFFFF;  // Top row white
        rgb565_buffer[(DISPLAY_HEIGHT - 1) * DISPLAY_WIDTH + x] = 0xFFFF;  // Bottom row white
    }
    for (uint32_t y = 0; y < DISPLAY_HEIGHT; y++) {
        rgb565_buffer[y * DISPLAY_WIDTH] = 0xFFFF;  // Left column white
        rgb565_buffer[y * DISPLAY_WIDTH + (DISPLAY_WIDTH - 1)] = 0xFFFF;  // Right column white
    }
    
    ESP_LOGI(TAG, "Drawing final test pattern: RED/GREEN/BLUE with white border");
    
    // Display on screen
    display_->draw_bitmap(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, rgb565_buffer);
    
    free(rgb565_buffer);
    ESP_LOGI(TAG, "Final test pattern displayed!");
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

    // Initialize rendering engine
    ESP_LOGI(TAG, "Step 3/4: Initializing rendering engine");
    renderer_ = std::make_unique<RenderEngine>(*display_, TILE_HEIGHT);
    if (!renderer_->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize renderer");
        return false;
    }

    // Load gauge file
    ESP_LOGI(TAG, "Step 4/5: Loading gauge file from SPIFFS");
    
    // Read gauge file from storage
    std::vector<uint8_t> gauge_data;
    if (!storage_->read_file(GAUGE_FILE_PATH, gauge_data)) {
        ESP_LOGE(TAG, "Failed to read gauge file: %s", GAUGE_FILE_PATH);
        return false;
    }
    
    ESP_LOGI(TAG, "Gauge file loaded: %zu bytes", gauge_data.size());
    
    // Load gauge into renderer
    if (!renderer_->load_gauge(gauge_data.data(), gauge_data.size())) {
        ESP_LOGE(TAG, "Failed to load gauge into renderer");
        return false;
    }
    
    ESP_LOGI(TAG, "Gauge loaded successfully!");

    ESP_LOGI(TAG, "Step 5/5: Initializing OBD2 UART (ELM327 on A1/A0)");
    obd2_ = std::make_unique<OBD2UartManager>();

    obd2_->set_state_callback([this](OBD2UartManager::ConnectionState state, const char* message) {
        ESP_LOGI(TAG, "OBD2 state=%d: %s", static_cast<int>(state), message ? message : "");

        if (renderer_) {
            renderer_->set_obd_connected(state == OBD2UartManager::ConnectionState::CONNECTED);
        }
    });

    obd2_->set_pid_callback([this](uint32_t pid_id, float value) {
        if (renderer_) {
            renderer_->set_pid_value(pid_id, value);
        }
    });

    if (!obd2_->initialize()) {
        ESP_LOGW(TAG, "OBD2 UART init did not complete at boot; background retries enabled");
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

    ESP_LOGI(TAG, "Starting main loop...");
    
    // Clear to black before rendering gauge
    display_->clear(0x000000);
    
    ESP_LOGI(TAG, "Rendering gauge...");
    
    ESP_LOGI(TAG, "Starting animated render loop at %u FPS", static_cast<unsigned>(TARGET_FPS));

    // Animated render loop
    while (true) {
        if (obd2_) {
            obd2_->update();
        }
        renderer_->render_frame();
        vTaskDelay(FRAME_DELAY_MS / portTICK_PERIOD_MS);
    }
}

} // namespace digidash
