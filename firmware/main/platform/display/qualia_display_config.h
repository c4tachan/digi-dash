#pragma once

// Adafruit Qualia ESP32-S3 RGB666 Display Configuration
// Based on https://learn.adafruit.com/adafruit-qualia-esp32-s3-for-rgb666-displays

#define QUALIA_PIN_NUM_HSYNC    (39)
#define QUALIA_PIN_NUM_VSYNC    (40)
#define QUALIA_PIN_NUM_DE       (41)
#define QUALIA_PIN_NUM_PCLK     (42)

// RGB Data pins (RGB666 uses 6 bits per color: R5-R0, G5-G0, B5-B0)
#define QUALIA_PIN_NUM_DATA0    (8)   // B0
#define QUALIA_PIN_NUM_DATA1    (3)   // B1
#define QUALIA_PIN_NUM_DATA2    (46)  // B2
#define QUALIA_PIN_NUM_DATA3    (9)   // B3
#define QUALIA_PIN_NUM_DATA4    (1)   // B4
#define QUALIA_PIN_NUM_DATA5    (5)   // B5
#define QUALIA_PIN_NUM_DATA6    (6)   // G0
#define QUALIA_PIN_NUM_DATA7    (7)   // G1
#define QUALIA_PIN_NUM_DATA8    (15)  // G2
#define QUALIA_PIN_NUM_DATA9    (16)  // G3
#define QUALIA_PIN_NUM_DATA10   (4)   // G4
#define QUALIA_PIN_NUM_DATA11   (45)  // G5
#define QUALIA_PIN_NUM_DATA12   (48)  // R0
#define QUALIA_PIN_NUM_DATA13   (47)  // R1
#define QUALIA_PIN_NUM_DATA14   (21)  // R2
#define QUALIA_PIN_NUM_DATA15   (14)  // R3
// Note: For RGB565, only data0-data15 are used

// Display timings for 720x720 displays
// These are typical values - may need adjustment for specific panels
#define QUALIA_LCD_H_RES        (720)
#define QUALIA_LCD_V_RES        (720)
#define QUALIA_LCD_PIXEL_CLOCK  (16 * 1000 * 1000)  // 16MHz pixel clock

// Horizontal timing
#define QUALIA_LCD_HSYNC_BACK_PORCH    (30)
#define QUALIA_LCD_HSYNC_FRONT_PORCH   (10)
#define QUALIA_LCD_HSYNC_PULSE_WIDTH   (10)

// Vertical timing  
#define QUALIA_LCD_VSYNC_BACK_PORCH    (10)
#define QUALIA_LCD_VSYNC_FRONT_PORCH   (10)
#define QUALIA_LCD_VSYNC_PULSE_WIDTH   (10)

// Flags
#define QUALIA_LCD_HSYNC_POLARITY      (0)  // Active low
#define QUALIA_LCD_VSYNC_POLARITY      (0)  // Active low
#define QUALIA_LCD_DE_POLARITY         (1)  // Active high
#define QUALIA_LCD_PCLK_ACTIVE_NEG     (0)  // Data on rising edge
