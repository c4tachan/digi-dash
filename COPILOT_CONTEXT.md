# Digi-Dash Project — Development Context for GitHub Copilot

This project is a custom multi-display automotive dashboard for a Toyota MR2 Spyder. 
The dashboard will run on an ESP32 and render gauges using LVGL with SVG support via ThorVG. 
Development is happening on Linux, with simulation options:
1. SDL2-based PC simulator for quick UI testing
2. ESP32-S3 QEMU (virtual RGB framebuffer) for firmware bring-up
3. Physical ESP32 hardware deployment

## Goals
- Build a modular, maintainable C++ codebase.
- Use LVGL v9+ with ThorVG for SVG-based dashboard rendering.
- Simulate the dashboard UI on Linux using SDL2 simulator.
- Test ESP32 firmware in QEMU before hardware deployment.
- Deploy to actual ESP32 hardware with minimal changes.
- Integrate OBD-II and CAN bus data (mock data for now).

## Repository Structure
- `simulator/`       → LVGL PC simulator project (CMake + SDL2)
- `esp32-qemu/`      → ESP32 firmware with QEMU run helper
   - `digi-dash-esp32/` → ESP-IDF project with LVGL + ThorVG
   - `run-qemu-s3.sh`    → QEMU launch helper (SDL display + UART)
- `assets/`          → SVG dashboard assets, icons, fonts
- `docs/`            → Architecture notes, wiring diagrams

## What I want Copilot to help with
1. Scaffold the LVGL simulator project inside `simulator/` using CMake and SDL2.
2. Create a clean C++ structure for the UI:
   - A main screen
   - RPM gauge
   - Speed display
   - Coolant temp gauge (later)
3. Provide mock data generators for RPM, speed, and temperature.
4. Keep UI code modular (e.g., separate classes or modules for each gauge).
5. Prepare for future ESP32 deployment by keeping hardware-specific code isolated.
6. Use modern C++ conventions and clean architecture.

## Constraints
- Linux development environment (Ubuntu/Debian).
- GNOME Terminal.
- CMake for the simulator.
- ESP-IDF v5.3.1 for ESP32 firmware.
- C++17 or newer for simulator, C11 for ESP32.
- LVGL v9+ with ThorVG for SVG rendering.
- QEMU (esp-qemu from ESP-IDF tools) for ESP32-S3 simulation.
- No Python or JavaScript for core logic.
- Avoid platform-specific hacks; keep code portable.

## Autonomous Setup Instructions

**IMPORTANT: When cloning this repo fresh, Copilot should execute these steps autonomously to set up the complete development environment.**

### 1. Install System Dependencies

```bash
# Update package lists
sudo apt-get update

# Install build tools and dependencies
sudo apt-get install -y \
    git wget flex bison gperf python3 python3-pip python3-venv \
    cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 \
    build-essential libsdl2-dev dotnet-runtime-8.0 python3-pip \
    gtk-sharp2-gapi libglade2.0-cil-dev libglib2.0-cil-dev libgtk2.0-cil-dev
```

### 2. Install ESP-IDF v5.3.1

```bash
# Clone ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone --recursive --depth 1 --branch v5.3.1 https://github.com/espressif/esp-idf.git

# Install ESP-IDF tools
cd esp-idf
./install.sh esp32

# Verify installation (will need to source export.sh first)
. ./export.sh
idf.py --version
```

### 3. Install esp-qemu (ESP32-S3 QEMU from ESP-IDF tools)

```bash
# From inside ESP-IDF
cd ~/esp/esp-idf
. ./export.sh
python -m idf_tools.py install esp-qemu
python -m idf_tools.py install-python-env  # ensures deps for esp-qemu wrappers
```

### 4. Build SDL2 Simulator

```bash
cd /path/to/digi-dash/simulator

# Create build directory
mkdir -p build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Test run (will auto-exit after timeout)
timeout 5 ./digi-dash-simulator
```

### 5. Build ESP32 Firmware

```bash
cd /path/to/digi-dash/esp32-qemu/digi-dash-esp32

# Source ESP-IDF environment
. ~/esp/esp-idf/export.sh

# Build firmware
idf.py build

# The output will be: build/digi-dash-esp32.elf
```

### 6. Run ESP32-S3 firmware in QEMU (GUI)

```bash
cd /path/to/digi-dash/esp32-qemu/digi-dash-esp32
. ~/esp/esp-idf/export.sh

# Build (if not already built)
idf.py build

# Launch bundled QEMU with SDL display
idf.py qemu -- -display sdl -serial stdio
```

### 7. Flash to Physical ESP32 Hardware

```bash
cd /path/to/digi-dash/esp32-qemu/digi-dash-esp32

# Source ESP-IDF environment
. ~/esp/esp-idf/export.sh

# Flash to ESP32 (replace /dev/ttyUSB0 with your port)
idf.py -p /dev/ttyUSB0 flash monitor
```

## Environment Setup for Each Terminal Session

Every time you open a new terminal to work on ESP32 firmware:

```bash
# Source ESP-IDF environment
. ~/esp/esp-idf/export.sh

# Now you can use idf.py commands
cd /path/to/digi-dash/esp32-qemu/digi-dash-esp32
idf.py build
```

## Quick Test Commands

```bash
# Test SDL2 simulator
cd simulator/build && timeout 5 ./digi-dash-simulator

# Test ESP32 build
cd esp32-qemu/digi-dash-esp32 && . ~/esp/esp-idf/export.sh && idf.py build

# Run in QEMU with GUI
cd esp32-qemu/digi-dash-esp32 && . ~/esp/esp-idf/export.sh && idf.py qemu -- -display sdl -serial stdio
```

## What Copilot Should Help With

1. **Autonomous Environment Setup**
   - When repo is freshly cloned, automatically execute all setup steps
   - Install dependencies (esp-qemu, ESP-IDF, SDL2, etc.)
   - Build both simulator and ESP32 firmware
   - Verify everything is working with test commands

2. **LVGL UI Development**
   - Create SVG-based dashboard layouts
   - Implement gauges using ThorVG vector graphics
   - Keep UI code modular and reusable
   - Test in SDL2 simulator first, then ESP32

3. **ESP32 Firmware**
   - Configure LVGL with ThorVG support
   - Set up display drivers (ILI9341, ST7789, etc.)
   - Manage SPIFFS for SVG asset storage
   - Handle OBD-II/CAN data integration

4. **Code Organization**
   - Maintain clean C++/C structure
   - Separate platform-specific code
   - Use modern conventions (C++17 for sim, C11 for ESP32)
   - Keep code portable between simulator and hardware

5. **Testing & Debugging**
   - Use `timeout` command for automated simulator testing
   - Test in QEMU before flashing hardware
   - Provide clear error messages and logging
   - Validate SVG rendering on all platforms

## Current Development Status

✅ SDL2 simulator with LVGL v9 + ThorVG support  
✅ ESP-IDF v5.3.1 installed and configured  
✅ esp-qemu installed for ESP32-S3 simulation  
✅ ESP32 project created with LVGL + ThorVG  
✅ Build system configured (CMake + ESP-IDF)  
⏳ Display driver configuration (ILI9341/ST7789)  
⏳ SVG dashboard design and asset pipeline  
⏳ OBD-II/CAN data integration  

## Key Technical Details

### LVGL Configuration (Both Platforms)
- LVGL v9.2+
- Color depth: 16-bit (RGB565) for ESP32, 32-bit for simulator
- ThorVG enabled for SVG rendering
- Required flags:
  - `LV_USE_FLOAT 1`
  - `LV_USE_MATRIX 1`
  - `LV_USE_VECTOR_GRAPHIC 1`
  - `LV_USE_SVG 1`
  - `LV_USE_THORVG_INTERNAL 1`

### ESP32 Hardware Configuration
- Target: ESP32 (can be adapted for ESP32-S3, ESP32-C3)
- Display: ILI9341 or ST7789 via SPI
- Resolution: 320x240 or 480x320
- SPIFFS partition for SVG assets
- PSRAM recommended for larger dashboards

### File System Setup
- SDL2 Simulator: Needs POSIX file system driver or embedded SVG data
- ESP32: SPIFFS with 'S:' driver letter for LVGL file access
  - Example: `lv_image_set_src(img, "S:/spiffs/dashboard.svg")`

## Common Issues & Solutions

### "unknown driver letter" Error
- **Cause**: LVGL file system not initialized
- **Solution**: Register POSIX driver for simulator, use SPIFFS with 'S:' for ESP32

### Build Errors with ThorVG
- **Cause**: Missing LV_USE_FLOAT or LV_USE_MATRIX
- **Solution**: Enable all required flags in lv_conf.h

### QEMU Display Not Showing
- **Cause**: Missing `-display sdl` or esp-qemu not installed
- **Solution**: Ensure `idf.py qemu -- -display sdl -serial stdio` and run `python -m idf_tools.py install esp-qemu`

### ESP-IDF Commands Not Found
- **Cause**: Environment not sourced
- **Solution**: Run `. ~/esp/esp-idf/export.sh` in every new terminal

## Starting Tasks for Copilot

When setting up a fresh clone:
1. Check if dependencies are installed, install if missing
2. Verify esp-qemu installation (via `idf_tools.py`) and ESP-IDF
3. Build simulator and test with `timeout 5 ./digi-dash-simulator`
4. Build ESP32 firmware
5. Test in QEMU with `idf.py qemu -- -display sdl -serial stdio`
6. Report status and next steps

Use this context to guide all suggestions and code generation autonomously.
