#include "application/application.h"
#include "esp_log.h"

static const char* TAG = "main";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Digi-Dash ESP32-S3 Firmware starting...");
    
    // Create and initialize application
    digidash::Application app;
    
    if (!app.initialize()) {
        ESP_LOGE(TAG, "Application initialization failed!");
        return;
    }
    
    // Run the application (this will loop forever)
    app.run();
}
