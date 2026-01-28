#include "application.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

    while (true) {
        renderer_->render_frame();
        vTaskDelay(FRAME_DELAY_MS / portTICK_PERIOD_MS);
    }
}

} // namespace digidash
