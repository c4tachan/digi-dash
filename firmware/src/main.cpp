#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "digidash/gauge_scene.h"

static const char* TAG = "digi-dash";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Digi-Dash ESP32-S3 Firmware starting...");

    // TODO: Initialize platform abstractions for ESP32
    // TODO: Initialize LVGL display
    // TODO: Initialize Bluetooth
    // TODO: Create gauge scene

    while (1) {
        ESP_LOGI(TAG, "Running...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
