# Digi-Dash Development Environment - Setup Complete! 🎉

## ✅ What's Been Set Up

### 1. **ESP-IDF v5.5.2**
- Complete ESP32-S3 development framework
- xtensa-esp32s3-elf toolchain (esp-14.2.0)
- Python environment configured
- Location: `~/esp-idf-5.5.2/`

### 2. **QEMU (Espressif Fork) v9.2.2**
- Built from source with ESP32-S3 RGB framebuffer support
- SDL2 display backend enabled
- xtensa-softmmu target compiled
- Location: `~/qemu-esp-develop/build/qemu-system-xtensa`
- Added to PATH via `~/.bashrc`

### 3. **ESP32-S3 Firmware Project**
- LVGL v9.4.0 graphics library
- esp_lcd_qemu_rgb virtual framebuffer driver
- 320×240 RGB565 display configuration
- FreeRTOS task-based LVGL rendering
- Location: `main/`

### 4. **Desktop Simulator** (Optional)
- SDL2-based LVGL simulator
- Quick UI prototyping without ESP-IDF
- Location: `simulator/`

## 🚀 Quick Start Commands

### Build and Run in QEMU with Graphics

```bash
cd ~/projects/digi-dash
. ~/esp-idf-5.5.2/export.sh
idf.py qemu --graphics monitor
```

This will:
- Build the firmware if needed
- Launch QEMU with SDL graphics window (320×240)
- Open serial monitor for logs
- Show your LVGL dashboard UI

**Controls:**
- `Ctrl+]` - Exit monitor (QEMU keeps running)
- `Ctrl+C` twice - Fully exit

### Build Only

```bash
cd ~/projects/digi-dash
. ~/esp-idf-5.5.2/export.sh
idf.py build
```

### Clean Build

```bash
idf.py fullclean
idf.py build
```

### Flash to Real Hardware

```bash
# Find your ESP32-S3 device
ls /dev/ttyUSB* /dev/ttyACM*

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## 📂 Project Structure

```
digi-dash/
├── main/
│   ├── main.c              # Application entry with LVGL setup
│   ├── idf_component.yml   # Dependencies (LVGL, esp_lcd_qemu_rgb)
│   └── CMakeLists.txt
├── simulator/              # Desktop SDL2 simulator
├── components/             # Custom ESP-IDF components
├── docs/                   # Documentation
└── tests/                  # Unit tests
```

## 🔧 QEMU Build Details

The custom QEMU build includes:
- **Source:** Espressif fork (esp-develop branch)
- **Target:** xtensa-softmmu (ESP32-S3 machine type)
- **Display:** SDL2 backend for RGB framebuffer
- **Network:** SLIRP user networking
- **RGB Device:** Virtual framebuffer at 0x20000000 (320×240, RGB565)

## 🎨 LVGL Configuration

Current setup in `main/main.c`:
- **Display:** 320×240 pixels
- **Color:** RGB565 (16-bit)
- **Buffer:** 10 lines (3,200 pixels × 2 bytes = 6,400 bytes)
- **Render Mode:** Partial (line-by-line)
- **Task:** 5ms handler loop, 4KB stack

## 📝 Component Dependencies

Managed via ESP-IDF Component Manager:

```yaml
dependencies:
  espressif/esp_lcd_qemu_rgb: "^1.0.0"
  lvgl/lvgl: "^9.2.2"
```

Components downloaded to `managed_components/` on first build.

## 🐛 Troubleshooting

### QEMU window doesn't appear
```bash
# Check QEMU is in PATH
which qemu-system-xtensa

# Should show: /home/catachan/qemu-esp-develop/build/qemu-system-xtensa
```

### Build errors about missing components
```bash
# Clean and rebuild to fetch components
idf.py fullclean
idf.py build
```

### SDL display error
```bash
# Ensure DISPLAY is set (WSL2)
echo $DISPLAY

# Check SDL2 libraries
ldconfig -p | grep SDL2
```

## 🔗 Resources

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/)
- [LVGL Documentation](https://docs.lvgl.io/9.4/)
- [Espressif QEMU](https://github.com/espressif/qemu)
- [esp_lcd_qemu_rgb Component](https://components.espressif.com/components/espressif/esp_lcd_qemu_rgb)

## 🎯 Next Steps

1. **Customize UI** - Edit `main/main.c` to add widgets, gauges, graphs
2. **Add data sources** - Integrate sensors, OBD2, or mock data
3. **Test in simulator** - Use `simulator/` for rapid UI iteration
4. **Deploy to hardware** - Flash to real ESP32-S3 with TFT display

Happy coding! 🚀
