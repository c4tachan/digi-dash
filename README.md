# Digi-Dash: ESP32-S3 Digital Dashboard

A digital dashboard application for ESP32-S3 with LVGL graphics, designed to run on real hardware or in QEMU emulation.

| Supported Targets | ESP32-S3 |
| ----------------- | -------- |

## Features

- **LVGL 9** graphics library with 320×240 RGB565 display
- **QEMU emulation** with SDL graphics output for development
- **ESP32-S3** native support via ESP-IDF 5.5.2
- **Component-based** architecture for easy extension
- **Desktop simulator** for rapid UI prototyping

## Quick Start

### Prerequisites

- **ESP-IDF v5.5.2** - [Installation Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/get-started/index.html)
- **QEMU (Espressif fork)** - Built from source with ESP32-S3 RGB support
- **Linux/WSL2** environment (for QEMU graphics)

### Build and Run in QEMU

```bash
# Setup ESP-IDF environment
. ~/esp-idf-5.5.2/export.sh

# Build firmware
idf.py build

# Run in QEMU with graphics window
idf.py qemu --graphics monitor
```

An SDL window will open showing the dashboard display. Press `Ctrl+]` to exit the monitor.

### Flash to Real Hardware

```bash
# Setup ESP-IDF environment
. ~/esp-idf-5.5.2/export.sh

# Build and flash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Project Structure

```
├── main/                      # Main ESP32 application
│   ├── main.c                # Application entry point with LVGL setup
│   ├── idf_component.yml     # Component dependencies (LVGL, esp_lcd_qemu_rgb)
│   └── CMakeLists.txt
├── simulator/                 # Desktop SDL2 simulator
│   ├── src/                  # Simulator source code
│   ├── include/              # Headers
│   └── CMakeLists.txt
├── components/                # Custom ESP-IDF components
├── docs/                      # Documentation
└── tests/                     # Unit tests
```

## Development Workflow

1. **Prototype UI** - Use desktop simulator for fast iteration
2. **Test in QEMU** - Verify ESP32-S3 integration with `idf.py qemu --graphics`
3. **Run unit tests** - Validate core functionality: `cd test_app && idf.py qemu monitor`
4. **Deploy to hardware** - Flash to real ESP32-S3 device

## Testing

### Unit Tests

ESP-IDF Unity tests for core functionality:

```bash
cd test_app
. ~/esp-idf-5.5.2/export.sh
idf.py build
idf.py qemu monitor
```

See [ESP32 Testing Guide](docs/ESP32_TESTING.md) for details.

## Configuration

The LVGL display is configured in `main/main.c`:
- Resolution: 320×240 pixels
- Color depth: RGB565 (16-bit)
- Buffer: 10 lines (partial rendering)

## Dependencies

Managed via ESP-IDF Component Manager (`main/idf_component.yml`):
- `espressif/esp_lcd_qemu_rgb` - QEMU virtual framebuffer driver
- `lvgl/lvgl` - LVGL graphics library v9.x

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Testing Guide](docs/TESTING.md)
- [Safety Considerations](docs/SAFETY.md)
- [Setup Complete Guide](SETUP_COMPLETE.md)
