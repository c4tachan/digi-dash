#include <stdio.h>
#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("     Digi-Dash ESP32-S3 Unit Tests\n");
    printf("═══════════════════════════════════════════════════════\n");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    unity_run_menu();
}
