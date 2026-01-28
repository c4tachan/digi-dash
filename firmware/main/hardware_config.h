#pragma once

// Hardware configuration for NV3052C display
// This panel uses 3-wire SPI for commands + RGB parallel interface for data

// Display resolution  
#define HW_LCD_WIDTH  720
#define HW_LCD_HEIGHT 720

// 3-Wire SPI pins (for sending commands to NV3052C)
#define HW_LCD_SPI_CS_GPIO   9   // CS pin
#define HW_LCD_SPI_SCK_GPIO  10  // SCK pin
#define HW_LCD_SPI_SDO_GPIO  11  // SDO/MOSI pin

// RGB parallel interface pins (Adafruit Qualia ESP32-S3 RGB666 standard pinout)
#define HW_LCD_RGB_DE_GPIO    40  // Data Enable
#define HW_LCD_RGB_PCLK_GPIO  42  // Pixel Clock
#define HW_LCD_RGB_VSYNC_GPIO 41  // Vertical Sync
#define HW_LCD_RGB_HSYNC_GPIO 39  // Horizontal Sync
#define HW_LCD_RGB_DISP_GPIO  38  // Display Enable / Backlight

// RGB data pins (RGB666 - 6 bits per color, using upper 6 bits of each byte)
// Red[5:0] on DATA0-5, Green[5:0] on DATA8-13, Blue[5:0] on DATA16-21
#define HW_LCD_RGB_DATA0_GPIO  1   // R0
#define HW_LCD_RGB_DATA1_GPIO  2   // R1
#define HW_LCD_RGB_DATA2_GPIO  3   // R2
#define HW_LCD_RGB_DATA3_GPIO  4   // R3
#define HW_LCD_RGB_DATA4_GPIO  5   // R4
#define HW_LCD_RGB_DATA5_GPIO  6   // R5
#define HW_LCD_RGB_DATA6_GPIO  7   // R6
#define HW_LCD_RGB_DATA7_GPIO  8   // R7

// Reset pin
#define HW_LCD_RST_GPIO  14  // Reset pin

// Timing configuration (typical for NV3052C at 30Hz)
#define HW_LCD_REFRESH_RATE_HZ  30
