#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_qemu_rgb.h"
#include "esp_log.h"
#include "lvgl.h"
#include "soc/syscon_reg.h"
#include "soc/soc.h"
#include <inttypes.h>

static const char *TAG = "digi-dash";

#define RGB_QEMU_ORIGIN 0x51454d55
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240
#define LVGL_TICK_PERIOD_MS 2

static SemaphoreHandle_t lvgl_mux = NULL;

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, (uint16_t *)px_map);
    lv_display_flush_ready(disp);
}

static void example_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

bool example_lvgl_lock(int timeout_ms)
{
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void example_lvgl_unlock(void)
{
    xSemaphoreGiveRecursive(lvgl_mux);
}

static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    while (1) {
        if (example_lvgl_lock(500)) {
            lv_timer_handler();
            example_lvgl_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Digi-Dash ESP32-S3 Application");

    // Create LVGL mutex
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);

    // Initialize QEMU RGB panel
    REG_WRITE(SYSCON_DATE_REG - 4, RGB_QEMU_ORIGIN);
    uint32_t qemu_origin = REG_READ(SYSCON_DATE_REG - 4);
    ESP_LOGI(TAG, "QEMU origin register: 0x%08" PRIx32, qemu_origin);
    
    ESP_LOGI(TAG, "Install RGB LCD panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_qemu_config_t panel_config = {
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
        .bpp = RGB_QEMU_BPP_16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_qemu(&panel_config, &panel_handle));

    ESP_LOGI(TAG, "Initialize RGB LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    
    // Allocate draw buffer (10 horizontal lines for LVGL 9)
    void *buf1 = malloc(DISPLAY_WIDTH * 10 * sizeof(uint16_t));
    assert(buf1);

    // Create display with LVGL 9 API
    ESP_LOGI(TAG, "Create LVGL display");
    static lv_display_t *disp = NULL;
    disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    lv_display_set_buffers(disp, buf1, NULL, DISPLAY_WIDTH * 10 * sizeof(uint16_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, example_lvgl_flush_cb);
    lv_display_set_user_data(disp, panel_handle);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Create LVGL port task");
    xTaskCreate(example_lvgl_port_task, "LVGL", 4096, NULL, 2, NULL);

    // Create UI
    if (example_lvgl_lock(-1)) {
        lv_obj_t *scr = lv_screen_active();
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
        
        // Add "Hello World!" label
        lv_obj_t *label = lv_label_create(scr);
        lv_label_set_text(label, "Hello World!");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        
        // Force a refresh
        lv_obj_invalidate(scr);
        
        example_lvgl_unlock();
    }
    
    ESP_LOGI(TAG, "Hello World! displayed on screen");

    // Main loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
