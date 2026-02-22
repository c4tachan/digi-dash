#include "application.h"
#include "platform/display/qualia_display_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <array>
#include <cstring>
#include <cmath>
#include <algorithm>

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
static constexpr uint32_t LIVE_PCLK_STEP_INTERVAL_MS = 1200;

// NV3052C RGB interface AC timing minima from datasheet section 7.3.2
static constexpr uint32_t NV3052C_PCLK_CYCLE_MIN_NS = 28;   // tCYCD
static constexpr uint32_t NV3052C_PCLK_HIGH_MIN_NS = 13;    // PWDH
static constexpr uint32_t NV3052C_PCLK_LOW_MIN_NS = 13;     // PWDL

static constexpr uint32_t H_TOTAL = QUALIA_LCD_H_RES +
                                    QUALIA_LCD_HSYNC_FRONT_PORCH +
                                    QUALIA_LCD_HSYNC_BACK_PORCH +
                                    QUALIA_LCD_HSYNC_PULSE_WIDTH;
static constexpr uint32_t V_TOTAL = QUALIA_LCD_V_RES +
                                    QUALIA_LCD_VSYNC_FRONT_PORCH +
                                    QUALIA_LCD_VSYNC_BACK_PORCH +
                                    QUALIA_LCD_VSYNC_PULSE_WIDTH;

static constexpr std::array<uint32_t, 13> THEORETICAL_SWEEP_HZ = {
    12000000UL, 14000000UL, 16000000UL, 18000000UL, 20000000UL,
    22000000UL, 24000000UL, 26000000UL, 28000000UL, 30000000UL,
    32000000UL, 34000000UL, 35700000UL
};

static size_t find_closest_sweep_index(uint32_t target_hz) {
    size_t best_index = 0;
    uint32_t best_diff = (THEORETICAL_SWEEP_HZ[0] > target_hz)
                             ? (THEORETICAL_SWEEP_HZ[0] - target_hz)
                             : (target_hz - THEORETICAL_SWEEP_HZ[0]);

    for (size_t index = 1; index < THEORETICAL_SWEEP_HZ.size(); ++index) {
        uint32_t candidate = THEORETICAL_SWEEP_HZ[index];
        uint32_t diff = (candidate > target_hz) ? (candidate - target_hz) : (target_hz - candidate);
        if (diff < best_diff) {
            best_diff = diff;
            best_index = index;
        }
    }

    return best_index;
}

// Alignment pattern colors (RGB565)
static constexpr uint16_t COLOR_BLACK = 0x0000;
static constexpr uint16_t COLOR_CYAN = 0x07FF;
static constexpr uint16_t COLOR_RED = 0xF800;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;

// Pattern thicknesses in pixels
static constexpr int32_t BORDER_THICKNESS = 16;
static constexpr int32_t CROSSHAIR_THICKNESS = 12;

static void fill_rect(uint16_t* buffer, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
    int32_t x0 = std::max<int32_t>(0, x);
    int32_t y0 = std::max<int32_t>(0, y);
    int32_t x1 = std::min<int32_t>(static_cast<int32_t>(DISPLAY_WIDTH), x + w);
    int32_t y1 = std::min<int32_t>(static_cast<int32_t>(DISPLAY_HEIGHT), y + h);

    for (int32_t py = y0; py < y1; ++py) {
        for (int32_t px = x0; px < x1; ++px) {
            buffer[py * DISPLAY_WIDTH + px] = color;
        }
    }
}

static void draw_digit_7seg(uint16_t* buffer, int digit, int32_t x, int32_t y, int32_t scale, uint16_t color) {
    static const uint8_t seg_map[10] = {
        0b1111110, // 0
        0b0110000, // 1
        0b1101101, // 2
        0b1111001, // 3
        0b0110011, // 4
        0b1011011, // 5
        0b1011111, // 6
        0b1110000, // 7
        0b1111111, // 8
        0b1111011  // 9
    };

    if (digit < 0 || digit > 9) {
        return;
    }

    int32_t seg_thick = std::max<int32_t>(2, scale / 4);
    int32_t seg_len = scale;
    uint8_t mask = seg_map[digit];

    // Segments: A B C D E F G
    if (mask & (1 << 6)) fill_rect(buffer, x + seg_thick, y, seg_len, seg_thick, color); // A
    if (mask & (1 << 5)) fill_rect(buffer, x + seg_thick + seg_len, y + seg_thick, seg_thick, seg_len, color); // B
    if (mask & (1 << 4)) fill_rect(buffer, x + seg_thick + seg_len, y + 2 * seg_thick + seg_len, seg_thick, seg_len, color); // C
    if (mask & (1 << 3)) fill_rect(buffer, x + seg_thick, y + 2 * (seg_thick + seg_len), seg_len, seg_thick, color); // D
    if (mask & (1 << 2)) fill_rect(buffer, x, y + 2 * seg_thick + seg_len, seg_thick, seg_len, color); // E
    if (mask & (1 << 1)) fill_rect(buffer, x, y + seg_thick, seg_thick, seg_len, color); // F
    if (mask & (1 << 0)) fill_rect(buffer, x + seg_thick, y + seg_thick + seg_len, seg_len, seg_thick, color); // G
}

static void draw_clock_mhz_overlay(uint16_t* buffer, uint32_t pclk_hz) {
    uint32_t mhz = pclk_hz / 1000000;
    int hundreds = static_cast<int>((mhz / 100) % 10);
    int tens = static_cast<int>((mhz / 10) % 10);
    int ones = static_cast<int>(mhz % 10);

    int32_t scale = 36;
    int32_t digit_w = scale + 2 * std::max<int32_t>(2, scale / 4);
    int32_t gap = 10;
    int32_t origin_x = 32;
    int32_t origin_y = 32;

    fill_rect(buffer, origin_x - 12, origin_y - 12, 3 * digit_w + 2 * gap + 24, 2 * scale + 32, COLOR_BLACK);
    draw_digit_7seg(buffer, hundreds, origin_x, origin_y, scale, COLOR_WHITE);
    draw_digit_7seg(buffer, tens, origin_x + digit_w + gap, origin_y, scale, COLOR_WHITE);
    draw_digit_7seg(buffer, ones, origin_x + 2 * (digit_w + gap), origin_y, scale, COLOR_WHITE);
}

static inline void draw_thick_pixel(uint16_t* buffer, int32_t x, int32_t y, int32_t half_thickness, uint16_t color) {
    for (int32_t oy = -half_thickness; oy <= half_thickness; ++oy) {
        int32_t py = y + oy;
        if (py < 0 || py >= static_cast<int32_t>(DISPLAY_HEIGHT)) {
            continue;
        }
        for (int32_t ox = -half_thickness; ox <= half_thickness; ++ox) {
            int32_t px = x + ox;
            if (px < 0 || px >= static_cast<int32_t>(DISPLAY_WIDTH)) {
                continue;
            }
            buffer[py * DISPLAY_WIDTH + px] = color;
        }
    }
}

static void draw_thick_line(uint16_t* buffer, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t thickness, uint16_t color) {
    int32_t half_thickness = thickness / 2;
    int32_t dx = std::abs(x1 - x0);
    int32_t dy = std::abs(y1 - y0);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx - dy;

    while (true) {
        draw_thick_pixel(buffer, x0, y0, half_thickness, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int32_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

Application::Application()
    : display_(nullptr)
    , storage_(nullptr)
    , renderer_(nullptr)
    , initialized_(false)
    , crosshair_angle_rad_(0.0f)
    , live_pclk_hz_(QUALIA_LCD_PIXEL_CLOCK)
    , live_sweep_index_(find_closest_sweep_index(QUALIA_LCD_PIXEL_CLOCK))
    , last_pclk_step_tick_(0) {
}

Application::~Application() {
}

void Application::display_hello_world(float angle_rad) {
    if (!pattern_base_buffer_) {
        pattern_base_buffer_ = std::make_unique<uint16_t[]>(DISPLAY_WIDTH * DISPLAY_HEIGHT);
        if (!pattern_base_buffer_) {
            ESP_LOGE(TAG, "Failed to allocate base pattern buffer");
            return;
        }

        for (uint32_t y = 0; y < DISPLAY_HEIGHT; y++) {
            for (uint32_t x = 0; x < DISPLAY_WIDTH; x++) {
                bool in_cyan_border =
                    (static_cast<int32_t>(x) < BORDER_THICKNESS) ||
                    (static_cast<int32_t>(x) >= static_cast<int32_t>(DISPLAY_WIDTH) - BORDER_THICKNESS) ||
                    (static_cast<int32_t>(y) < BORDER_THICKNESS) ||
                    (static_cast<int32_t>(y) >= static_cast<int32_t>(DISPLAY_HEIGHT) - BORDER_THICKNESS);

                pattern_base_buffer_[y * DISPLAY_WIDTH + x] = in_cyan_border ? COLOR_CYAN : COLOR_BLACK;
            }
        }
    }

    uint8_t* framebuffer_bytes = display_->lock_framebuffer();
    if (!framebuffer_bytes) {
        ESP_LOGE(TAG, "Failed to lock framebuffer");
        return;
    }

    uint16_t* framebuffer = reinterpret_cast<uint16_t*>(framebuffer_bytes);
    std::memcpy(framebuffer, pattern_base_buffer_.get(), DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t));

    const int32_t center_x = static_cast<int32_t>(DISPLAY_WIDTH / 2);
    const int32_t center_y = static_cast<int32_t>(DISPLAY_HEIGHT / 2);
    const int32_t crosshair_radius = static_cast<int32_t>((DISPLAY_WIDTH < DISPLAY_HEIGHT ? DISPLAY_WIDTH : DISPLAY_HEIGHT) / 2 - BORDER_THICKNESS - 4);

    const float cos_a = std::cos(angle_rad);
    const float sin_a = std::sin(angle_rad);

    const int32_t x0 = center_x - static_cast<int32_t>(std::lround(crosshair_radius * cos_a));
    const int32_t y0 = center_y - static_cast<int32_t>(std::lround(crosshair_radius * sin_a));
    const int32_t x1 = center_x + static_cast<int32_t>(std::lround(crosshair_radius * cos_a));
    const int32_t y1 = center_y + static_cast<int32_t>(std::lround(crosshair_radius * sin_a));

    const int32_t x2 = center_x - static_cast<int32_t>(std::lround(crosshair_radius * (-sin_a)));
    const int32_t y2 = center_y - static_cast<int32_t>(std::lround(crosshair_radius * cos_a));
    const int32_t x3 = center_x + static_cast<int32_t>(std::lround(crosshair_radius * (-sin_a)));
    const int32_t y3 = center_y + static_cast<int32_t>(std::lround(crosshair_radius * cos_a));

    draw_thick_line(framebuffer, x0, y0, x1, y1, CROSSHAIR_THICKNESS, COLOR_RED);
    draw_thick_line(framebuffer, x2, y2, x3, y3, CROSSHAIR_THICKNESS, COLOR_RED);
    draw_clock_mhz_overlay(framebuffer, live_pclk_hz_);

    display_->unlock_and_update();
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

    // Show display alignment patterns before continuing app initialization
    ESP_LOGI(TAG, "Showing startup display alignment patterns");
    constexpr uint32_t cycle_limited_max_hz = 1000000000UL / NV3052C_PCLK_CYCLE_MIN_NS;
    constexpr uint32_t duty_limited_max_hz = 1000000000UL / (NV3052C_PCLK_HIGH_MIN_NS + NV3052C_PCLK_LOW_MIN_NS);
    constexpr uint32_t theoretical_max_hz = (cycle_limited_max_hz < duty_limited_max_hz)
                                                ? cycle_limited_max_hz
                                                : duty_limited_max_hz;
    ESP_LOGI(TAG,
             "NV3052C theoretical PCLK limit %.3f MHz (tCYCD=%u ns, PWDH=%u ns, PWDL=%u ns)",
             static_cast<double>(theoretical_max_hz) / 1000000.0,
             (unsigned)NV3052C_PCLK_CYCLE_MIN_NS,
             (unsigned)NV3052C_PCLK_HIGH_MIN_NS,
             (unsigned)NV3052C_PCLK_LOW_MIN_NS);

    live_sweep_index_ = find_closest_sweep_index(QUALIA_LCD_PIXEL_CLOCK);
    live_pclk_hz_ = THEORETICAL_SWEEP_HZ[live_sweep_index_];
    display_->set_pixel_clock(live_pclk_hz_);
    last_pclk_step_tick_ = static_cast<uint32_t>(xTaskGetTickCount());
    display_hello_world();

    ESP_LOGI(TAG, "Test-pattern-only mode enabled; skipping storage and gauge initialization");
    initialized_ = true;
    ESP_LOGI(TAG, "Application initialized successfully!");
    return true;

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
    ESP_LOGI(TAG, "Step 4/4: Loading gauge file from SPIFFS");
    
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
    
    initialized_ = true;
    ESP_LOGI(TAG, "Application initialized successfully!");
    return true;
}

void Application::run() {
    if (!initialized_) {
        ESP_LOGE(TAG, "Cannot run: application not initialized");
        return;
    }

    ESP_LOGI(TAG, "Starting main loop (test-pattern-only mode)...");
    ESP_LOGI(TAG, "Animating rotating crosshair");

    while (true) {
        uint32_t now_tick = static_cast<uint32_t>(xTaskGetTickCount());
        uint32_t elapsed_ms = (now_tick - last_pclk_step_tick_) * portTICK_PERIOD_MS;
        if (elapsed_ms >= LIVE_PCLK_STEP_INTERVAL_MS) {
            live_sweep_index_ = (live_sweep_index_ + 1) % THEORETICAL_SWEEP_HZ.size();
            live_pclk_hz_ = THEORETICAL_SWEEP_HZ[live_sweep_index_];
            display_->set_pixel_clock(live_pclk_hz_);

            const double pclk_mhz = static_cast<double>(live_pclk_hz_) / 1000000.0;
            const double expected_fps = static_cast<double>(live_pclk_hz_) /
                                        static_cast<double>(H_TOTAL * V_TOTAL);
            const double cycle_ns = 1000000000.0 / static_cast<double>(live_pclk_hz_);
            ESP_LOGI(TAG,
                     "Theoretical sweep: %.3f MHz | expected %.2f FPS | cycle %.2f ns",
                     pclk_mhz,
                     expected_fps,
                     cycle_ns);
            last_pclk_step_tick_ = now_tick;
        }

        display_hello_world(crosshair_angle_rad_);
        crosshair_angle_rad_ += 0.20734512f;  // 1 rotation/sec at 33ms/frame
        if (crosshair_angle_rad_ > 6.2831853f) {
            crosshair_angle_rad_ -= 6.2831853f;
        }
        vTaskDelay(33 / portTICK_PERIOD_MS);
    }
}

} // namespace digidash
