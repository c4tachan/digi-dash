#pragma once

// Adafruit Qualia ESP32-S3 RGB666 Display Configuration
// Based on https://learn.adafruit.com/adafruit-qualia-esp32-s3-for-rgb666-displays
// Panel: HD40015C 0-Y (NV3052C controller), 720x720 RGB666 TTL

#define QUALIA_PIN_NUM_HSYNC    (39)
#define QUALIA_PIN_NUM_VSYNC    (40)
#define QUALIA_PIN_NUM_DE       (41)
#define QUALIA_PIN_NUM_PCLK     (42)
#define QUALIA_PIN_NUM_BL       (2)   // Backlight enable

// SPI pins for NV3052C controller initialization
#define QUALIA_PIN_NUM_LCD_CS   (44)  // LCD chip select
#define QUALIA_PIN_NUM_LCD_DC   (43)  // Data/Command select
#define QUALIA_PIN_NUM_LCD_SCK  (36)  // SPI clock
#define QUALIA_PIN_NUM_LCD_MOSI (35)  // SPI MOSI
#define QUALIA_PIN_NUM_LCD_RST  (0)   // LCD reset (shared with BOOT button)

// RGB Data pins (RGB666 uses 18 bits: R5-R0, G5-G0, B5-B0)
// For RGB666, we use 18 data pins but can use 16-pin mode with reduced color depth
// Using 16-pin RGB565 mode: R5-R0, G5-G0, B5-B0 (but only 5 bits per color in 16-bit mode)
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
#define QUALIA_PIN_NUM_DATA16   (11)  // R4 (for RGB666 mode)
#define QUALIA_PIN_NUM_DATA17   (10)  // R5 (for RGB666 mode)

// Display timings for 720x720 NV3052C displays (DE-only mode)
// These timings are optimized for the NV3052C controller
#define QUALIA_LCD_H_RES        (720)
#define QUALIA_LCD_V_RES        (720)
#define QUALIA_LCD_PIXEL_CLOCK  (16000000)  // 16MHz pixel clock

// Horizontal timing (DE mode - HSYNC not required but configure anyway)
#define QUALIA_LCD_HSYNC_BACK_PORCH    (20)
#define QUALIA_LCD_HSYNC_FRONT_PORCH   (20)
#define QUALIA_LCD_HSYNC_PULSE_WIDTH   (10)

// Vertical timing (DE mode - VSYNC not required but configure anyway)
#define QUALIA_LCD_VSYNC_BACK_PORCH    (20)
#define QUALIA_LCD_VSYNC_FRONT_PORCH   (20)
#define QUALIA_LCD_VSYNC_PULSE_WIDTH   (10)

// Polarity settings
#define QUALIA_LCD_HSYNC_POLARITY      (0)  // Active low (not used in DE mode)
#define QUALIA_LCD_VSYNC_POLARITY      (0)  // Active low (not used in DE mode)
#define QUALIA_LCD_DE_POLARITY         (1)  // Active high (DE mode only)
#define QUALIA_LCD_PCLK_ACTIVE_NEG     (0)  // Data on rising edge
