#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_qemu_rgb.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "lvgl.h"
#include "soc/syscon_reg.h"
#include "soc/soc.h"
#include <sys/stat.h>
#include <inttypes.h>
// SVG canvas test
#include "dashboard_svg.h"
#include "libs/svg/lv_svg.h"
// Forward declare FS driver init
void lv_fs_spiffs_init(void);

static const char *TAG = "digi-dash";

#define RGB_QEMU_ORIGIN 0x51454d55
#define DISPLAY_WIDTH  720
#define DISPLAY_HEIGHT 720
#define LVGL_TICK_PERIOD_MS 2

static SemaphoreHandle_t lvgl_mux = NULL;
static void example_app_task(void *arg);

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

    // Create the main application task with plenty of stack for LVGL/ThorVG
    const UBaseType_t app_task_stack = 16384;
    const UBaseType_t app_task_prio = 5;
    BaseType_t res = xTaskCreate(example_app_task, "app", app_task_stack, NULL, app_task_prio, NULL);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to start app task (err=%ld)", (long)res);
    }
    vTaskDelete(NULL);
}

// Actual application entry (moved off the default main task to give more stack headroom)
static void example_app_task(void *arg)
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
    
    // Allocate draw buffer: 4 lines worth of RGB565 pixels
    const uint32_t draw_buf_lines = 4;
    const uint32_t bytes_per_px = lv_color_format_get_size(LV_COLOR_FORMAT_RGB565);
    const uint32_t buf_width = DISPLAY_WIDTH;  // Buffer width matches display width
    const uint32_t buf_size_bytes = buf_width * bytes_per_px * draw_buf_lines;
    void *buf1 = malloc(buf_size_bytes);
    assert(buf1);
    memset(buf1, 0, buf_size_bytes);  // Initialize buffer to black

    ESP_LOGI(TAG, "Display config: %dx%d, buf_width=%lu, buf=%lu bytes", 
             DISPLAY_WIDTH, DISPLAY_HEIGHT, buf_width, buf_size_bytes);

    // Create display with LVGL 9 API
    ESP_LOGI(TAG, "Create LVGL display");
    static lv_display_t *disp = NULL;
    disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_display_set_default(disp);

    // Match LVGL color format to the RGB565 panel
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    // Use standard buffer mode without explicit stride
    lv_display_set_buffers(disp, buf1, NULL, buf_size_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
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

    // Initialize SPIFFS for loading SVG files
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    } else {
        size_t total = 0, used = 0;
        ret = esp_spiffs_info("storage", &total, &used);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "SPIFFS: %d KB total, %d KB used", total / 1024, used / 1024);
        }
    }

    // Register LVGL FS driver mapping 'S:' to SPIFFS
    ESP_LOGI(TAG, "[CHECKPOINT] Before lv_fs_spiffs_init");
    lv_fs_spiffs_init();
    ESP_LOGI(TAG, "[CHECKPOINT] After lv_fs_spiffs_init");

    // Create UI
    if (example_lvgl_lock(-1)) {
        ESP_LOGI(TAG, "[CHECKPOINT] Inside LVGL lock");
        lv_obj_t *scr = lv_screen_active();
        // Dark background
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
        
        #if LV_USE_THORVG
            ESP_LOGI(TAG, "ThorVG is ENABLED");
        #else
            ESP_LOGE(TAG, "ThorVG is DISABLED - SVG will not render!");
        #endif

        #if LV_USE_SVG
            ESP_LOGI(TAG, "SVG support is ENABLED");
        #else
            ESP_LOGE(TAG, "SVG support is DISABLED!");
        #endif

        #if LV_USE_VECTOR_GRAPHIC
            ESP_LOGI(TAG, "Vector graphics is ENABLED");
        #else
            ESP_LOGE(TAG, "Vector graphics is DISABLED!");
        #endif

        // Load PNG logo from SPIFFS via LVGL FS driver
        ESP_LOGI(TAG, "Loading PNG from S:/scca_400.png");

        struct stat st;
        if (stat("/spiffs/scca_400.png", &st) == 0) {
            ESP_LOGI(TAG, "scca_400.png size: %ld bytes", (long)st.st_size);
        } else {
            ESP_LOGE(TAG, "scca_400.png stat failed");
        }
        
        lv_obj_t *img = lv_image_create(scr);
        assert(img);

        // Set size to match display first
        lv_obj_set_size(img, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        lv_obj_set_pos(img, 0, 0);
        
        // Add bright border to visualize image location
        lv_obj_set_style_border_color(img, lv_color_hex(0xFF00FF), 0);  // Bright magenta
        lv_obj_set_style_border_width(img, 10, 0);
        
        // Set the PNG file path - LVGL will load it automatically
        ESP_LOGI(TAG, "Setting image source to S:/scca_400.png");
        lv_image_set_src(img, "S:/scca_400.png");
        
        // Disable transformations
        lv_obj_set_style_transform_pivot_x(img, 0, 0);
        lv_obj_set_style_transform_pivot_y(img, 0, 0);
        lv_obj_set_style_transform_angle(img, 0, 0);
        lv_obj_set_style_transform_zoom(img, 256, 0); // 256 = 1.0x zoom
        
        lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_clip_corner(img, false, 0);
        
        lv_image_set_inner_align(img, LV_IMAGE_ALIGN_TOP_LEFT);
        lv_image_set_scale(img, 256); // 256 = 1.0x scale
        
        // Log image position and size
        lv_area_t area;
        lv_obj_get_coords(img, &area);
        ESP_LOGI(TAG, "Image position after load: x1=%d, y1=%d, x2=%d, y2=%d", area.x1, area.y1, area.x2, area.y2);
        ESP_LOGI(TAG, "Image size: w=%d, h=%d", lv_obj_get_width(img), lv_obj_get_height(img));
        
        // Log decoded image descriptor
        const lv_image_dsc_t *img_dsc = lv_image_get_src(img);
        if(img_dsc) {
            ESP_LOGI(TAG, "Image descriptor: header.w=%d, header.h=%d, format=%d", img_dsc->header.w, img_dsc->header.h, (int)img_dsc->header.cf);
        } else {
            ESP_LOGE(TAG, "Image descriptor is NULL - decode failed!");
        }
        
        lv_obj_invalidate(img);

        // Add a visible red square to verify rendering pipeline
        lv_obj_t *rect = lv_obj_create(scr);
        lv_obj_set_size(rect, 80, 80);
        lv_obj_set_pos(rect, 10, 10);
        lv_obj_set_style_bg_color(rect, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);

        // Render embedded SVG onto a canvas to validate SVG pipeline
        #if LV_USE_SVG
        {
            lv_obj_t *canvas = lv_canvas_create(scr);
            lv_obj_set_size(canvas, DISPLAY_WIDTH, DISPLAY_HEIGHT);
            lv_obj_set_pos(canvas, 0, 0);

            lv_draw_buf_t *canvas_buf = lv_draw_buf_create(DISPLAY_WIDTH, DISPLAY_HEIGHT, LV_COLOR_FORMAT_ARGB8888, 0);
            lv_canvas_set_draw_buf(canvas, canvas_buf);
            lv_canvas_fill_bg(canvas, lv_color_black(), 255);

            lv_layer_t layer;
            lv_canvas_init_layer(canvas, &layer);

            lv_svg_node_t *svg = lv_svg_load_data(dashboard_svg_data, (uint32_t)dashboard_svg_size);
            if(svg) {
                lv_draw_svg(&layer, svg);
                lv_canvas_finish_layer(canvas, &layer);
                lv_svg_node_delete(svg);
                ESP_LOGI(TAG, "Embedded SVG rendered to canvas");
            } else {
                ESP_LOGE(TAG, "Failed to load embedded SVG data");
            }
        }
        #endif
        
        example_lvgl_unlock();
    }

    ESP_LOGI(TAG, "Create LVGL port task");
    // ThorVG vector rendering requires larger stack; increase LVGL task stack
    xTaskCreate(example_lvgl_port_task, "LVGL", 16384, NULL, 2, NULL);
    
    ESP_LOGI(TAG, "Dashboard SVG displayed");
    // Main loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
