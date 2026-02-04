#pragma once

// Adafruit Qualia ESP32-S3 RGB666 Display Configuration
// Based on https://learn.adafruit.com/adafruit-qualia-esp32-s3-for-rgb666-displays
// Panel: HD40015C 0-Y (NV3052C controller), 720x720 RGB666 TTL
// Configuration matches working esprgbqualia2 example

// I2C pins for PCA9554 GPIO expander (controls TFT initialization)
#define QUALIA_PIN_NUM_I2C_SDA  (8)
#define QUALIA_PIN_NUM_I2C_SCL  (18)

// RGB interface timing signals
#define QUALIA_PIN_NUM_HSYNC    (41)
#define QUALIA_PIN_NUM_VSYNC    (42)
#define QUALIA_PIN_NUM_DE       (2)
#define QUALIA_PIN_NUM_PCLK     (1)

// NOTE: NV3052C controller initialization (reset, CS, SCK, MOSI) is handled 
// via PCA9554 I2C GPIO expander (address 0x3F), not direct GPIO
// Backlight is also controlled via PCA9554 pin 4

// RGB Data pins - Using RGB565 mode (16 bits)
// Pin mapping from working example (data_gpio_nums array order):
// B1-B5 (5 bits blue, B0 unused), G0-G5 (6 bits green), R1-R5 (5 bits red, R0 unused)
#define QUALIA_PIN_NUM_DATA0    (40)  // B1
#define QUALIA_PIN_NUM_DATA1    (39)  // B2
#define QUALIA_PIN_NUM_DATA2    (38)  // B3
#define QUALIA_PIN_NUM_DATA3    (0)   // B4
#define QUALIA_PIN_NUM_DATA4    (45)  // B5
#define QUALIA_PIN_NUM_DATA5    (48)  // G0
#define QUALIA_PIN_NUM_DATA6    (47)  // G1
#define QUALIA_PIN_NUM_DATA7    (21)  // G2
#define QUALIA_PIN_NUM_DATA8    (14)  // G3
#define QUALIA_PIN_NUM_DATA9    (13)  // G4
#define QUALIA_PIN_NUM_DATA10   (12)  // G5
#define QUALIA_PIN_NUM_DATA11   (11)  // R1
#define QUALIA_PIN_NUM_DATA12   (10)  // R2
#define QUALIA_PIN_NUM_DATA13   (9)   // R3
#define QUALIA_PIN_NUM_DATA14   (46)  // R4
#define QUALIA_PIN_NUM_DATA15   (3)   // R5

// Display timings for 720x720 NV3052C displays
// Timings from working esprgbqualia2 example
#define QUALIA_LCD_H_RES        (720)
#define QUALIA_LCD_V_RES        (720)
#define QUALIA_LCD_PIXEL_CLOCK  (12000000)  // 12MHz pixel clock (from working example)

// Horizontal timing (adjusted for 5mm left shift)
#define QUALIA_LCD_HSYNC_BACK_PORCH    (90)
#define QUALIA_LCD_HSYNC_FRONT_PORCH   (46)
#define QUALIA_LCD_HSYNC_PULSE_WIDTH   (2)

// Vertical timing (from working example)
#define QUALIA_LCD_VSYNC_BACK_PORCH    (16)
#define QUALIA_LCD_VSYNC_FRONT_PORCH   (50)
#define QUALIA_LCD_VSYNC_PULSE_WIDTH   (16)

// Polarity settings (from working example)
#define QUALIA_LCD_HSYNC_POLARITY      (1)  // hsync_idle_low = true
#define QUALIA_LCD_VSYNC_POLARITY      (1)  // vsync_idle_low = true  
#define QUALIA_LCD_DE_POLARITY         (0)  // de_idle_high = false
#define QUALIA_LCD_PCLK_ACTIVE_NEG     (0)  // pclk_active_neg = false
