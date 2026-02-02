#include "esp_stubs.h"
#include <cstring>

static int g_width = 0;
static int g_height = 0;
static std::vector<uint16_t> g_framebuffer;
static uint32_t g_reg_store = 0;

void esp_stub_set_panel_size(int width, int height) {
    g_width = width;
    g_height = height;
    g_framebuffer.assign(width * height, 0);
}

const std::vector<uint16_t>& esp_stub_get_framebuffer() { return g_framebuffer; }
void esp_stub_clear_framebuffer() { std::fill(g_framebuffer.begin(), g_framebuffer.end(), 0); }

esp_err_t esp_lcd_new_rgb_qemu(const esp_lcd_rgb_qemu_config_t* cfg, esp_lcd_panel_handle_t* out) {
    if (!cfg || !out) return -1;
    esp_stub_set_panel_size(cfg->width, cfg->height);
    *out = reinterpret_cast<esp_lcd_panel_handle_t>(0x1);
    return ESP_OK;
}

esp_err_t esp_lcd_panel_del(esp_lcd_panel_handle_t panel) { (void)panel; return ESP_OK; }
esp_err_t esp_lcd_panel_reset(esp_lcd_panel_handle_t panel) { (void)panel; return ESP_OK; }
esp_err_t esp_lcd_panel_init(esp_lcd_panel_handle_t panel) { (void)panel; return ESP_OK; }

esp_err_t esp_lcd_panel_draw_bitmap(esp_lcd_panel_handle_t panel, uint32_t x_start, uint32_t y_start, uint32_t x_end, uint32_t y_end, const void* color_data) {
    (void)panel;
    if (!color_data) return -1;
    if (x_end <= x_start || y_end <= y_start) return -1;
    uint32_t w = x_end - x_start;
    uint32_t h = y_end - y_start;
    const uint16_t* src = reinterpret_cast<const uint16_t*>(color_data);
    for (uint32_t yy = 0; yy < h; ++yy) {
        for (uint32_t xx = 0; xx < w; ++xx) {
            int dst_x = x_start + xx;
            int dst_y = y_start + yy;
            if (dst_x < 0 || dst_x >= g_width || dst_y < 0 || dst_y >= g_height) continue;
            g_framebuffer[dst_y * g_width + dst_x] = src[yy * w + xx];
        }
    }
    return ESP_OK;
}

esp_err_t esp_lcd_rgb_qemu_refresh(esp_lcd_panel_handle_t panel) { (void)panel; return ESP_OK; }

const char* esp_err_to_name(esp_err_t) { return "ESP_OK"; }

void esp_stub_reg_write(uint32_t val) { g_reg_store = val; }
uint32_t esp_stub_reg_read() { return g_reg_store; }
