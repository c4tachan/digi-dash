# Hardware Display Setup Guide

This guide explains how to configure and flash the digi-dash firmware for your NV3052C display panel.

## Overview

The NV3052C is an RGB LCD controller that uses:
- **3-Wire SPI** interface for sending commands (initialization, configuration)
- **RGB parallel interface** for sending pixel data (8-bit per color = 24-bit color)

## Prerequisites

- ESP32-S3 development board
- NV3052C display panel (720x720 or 720x1280)
- Connection wires
- ESP-IDF 5.5.2 environment

## Pin Configuration

Before building for hardware, you **must** configure the GPIO pins in [firmware/main/hardware_config.h](firmware/main/hardware_config.h).

### Required Pins

1. **3-Wire SPI pins** (for commands):
   - `HW_LCD_SPI_CS_GPIO` - Chip Select
   - `HW_LCD_SPI_SCK_GPIO` - Serial Clock
   - `HW_LCD_SPI_SDO_GPIO` - Serial Data Out (MOSI)

2. **RGB parallel interface pins**:
   - `HW_LCD_RGB_DE_GPIO` - Data Enable
   - `HW_LCD_RGB_PCLK_GPIO` - Pixel Clock
   - `HW_LCD_RGB_VSYNC_GPIO` - Vertical Sync
   - `HW_LCD_RGB_HSYNC_GPIO` - Horizontal Sync
   - `HW_LCD_RGB_DISP_GPIO` - Display Enable
   - `HW_LCD_RGB_DATA0_GPIO` through `HW_LCD_RGB_DATA7_GPIO` - 8-bit RGB data

3. **Control pins**:
   - `HW_LCD_RST_GPIO` - Reset pin

### Finding Your Panel's Pinout

Refer to your specific display module's datasheet or silkscreen labels. Common pin arrangements:

```
Example NV3052C Module:
┌─────────────────────┐
│  720x720 Display    │
│                     │
└─────────────────────┘
  │││││││││││││││││││
  Pin Headers:
  1. VCC (3.3V)
  2. GND
  3. CS
  4. SCK
  5. MOSI/SDO
  6. RST
  7-14. RGB Data[0:7]
  15. PCLK
  16. DE
  17. HSYNC
  18. VSYNC
  19. DISP_EN
```

## Build Configuration

### Step 1: Configure GPIO Pins

Edit [firmware/main/hardware_config.h](firmware/main/hardware_config.h) and set all GPIO numbers according to your wiring:

```cpp
// Example configuration for custom wiring
#define HW_LCD_SPI_CS_GPIO   10
#define HW_LCD_SPI_SCK_GPIO  11
#define HW_LCD_SPI_SDO_GPIO  12

#define HW_LCD_RGB_DE_GPIO    40
#define HW_LCD_RGB_PCLK_GPIO  41
// ... etc
```

### Step 2: Select Hardware Target

Run `idf.py menuconfig` and navigate to:
```
Digi-Dash Configuration → Target Hardware → Hardware (NV3052C)
```

Or build directly with:
```bash
cd firmware
source ~/esp/esp-idf/export.sh

# Configure for hardware
idf.py menuconfig
# Navigate to: Digi-Dash Configuration -> Select "Hardware (NV3052C)"

# Build
idf.py build
```

### Step 3: Flash to ESP32-S3

```bash
# Find your device
ls /dev/ttyUSB* /dev/ttyACM*

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## Building for QEMU (Simulator)

To switch back to QEMU mode:

```bash
idf.py menuconfig
# Navigate to: Digi-Dash Configuration -> Select "QEMU Simulator"
idf.py build
```

Or use the QEMU script:
```bash
cd /home/catachan/projects/digi-dash
./scripts/rebuild_gauge_and_run_qemu.sh
```

## Troubleshooting

### Display shows nothing

1. **Check power supply**: Ensure display module has proper 3.3V power
2. **Verify pin configuration**: Double-check all GPIO assignments in `hardware_config.h`
3. **Check wiring**: Use multimeter to verify connections
4. **Enable debug logs**: Monitor serial output for initialization errors

### Build fails with "pins not configured"

This is expected! You must edit `hardware_config.h` and set actual GPIO numbers. All pins default to `-1` (unconfigured).

### Display shows garbage/corrupted image

1. **Check pixel clock timing**: May need to adjust refresh rate in `hardware_config.h`
2. **Verify data pins**: Ensure RGB data pins are connected in correct order (D0-D7)
3. **Check signal integrity**: RGB parallel interface is sensitive to wire length/quality

### Panel not initializing

1. **Check CS/SCK/MOSI**: 3-Wire SPI must be connected for panel initialization
2. **Verify reset pin**: RST pin should be properly connected and pulled high
3. **Check timing**: Some panels need specific initialization delays

## Panel Specifications

The NV3052C supports multiple resolutions. Current configuration:
- **Resolution**: 720x720 (square) or 720x1280 (portrait)
- **Color depth**: RGB565 (16-bit) for framebuffer, RGB888 (24-bit) for panel
- **Refresh rate**: 30Hz (configurable)
- **Interface**: RGB parallel (8-bit per channel)

## Next Steps

Once the display is working:
1. Test with the gauge rendering engine
2. Add OBD2 data integration
3. Implement touch input (if your panel supports it)
4. Optimize rendering performance

## Resources

- [NV3052C Component Documentation](https://components.espressif.com/components/espressif/esp_lcd_nv3052)
- [ESP-IDF LCD Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/lcd.html)
- [ESP LCD Panel IO Additions](https://components.espressif.com/components/espressif/esp_lcd_panel_io_additions)
