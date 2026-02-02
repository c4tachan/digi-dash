#include "application/application.h"
#include "esp_log.h"
#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "main";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "BOOT: Digi-Dash Starting");
    ESP_LOGI(TAG, "========================================");
    
    digidash::Application app;
    app.initialize();
    app.run();
}
