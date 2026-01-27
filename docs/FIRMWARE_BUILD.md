# Digi-Dash Firmware Build Instructions

## Building the Engine Library

The firmware depends on the `digidash-engine` library. Build it first using the main project CMake:

```bash
cd /home/catachan/projects/digi-dash
mkdir -p build
cd build
cmake ..
make -j4

# The engine will be built as: build/engine/libdigidash-engine.a
```

## Building with ESP-IDF for Firmware/QEMU

The firmware is built using ESP-IDF's `idf.py` tool. You need ESP-IDF 5.5 or later.

### 1. Setup ESP-IDF (one time)

```bash
# Download and install ESP-IDF
mkdir -p ~/esp-idf
cd ~/esp-idf
git clone -b release/v5.5 https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
```

### 2. Source ESP-IDF in your shell

```bash
source ~/esp-idf/esp-idf/export.sh
```

Or add to your `~/.bashrc`:
```bash
alias idf_setup='source ~/esp-idf/esp-idf/export.sh'
```

### 3. Build the firmware

From the project root:

```bash
# Build for hardware flash (creates build/firmware/)
idf.py build

# Build for QEMU
idf.py set-target esp32s3
idf.py build
```

### 4. Run in QEMU

```bash
idf.py qemu monitor

# Press CTRL-] to exit QEMU
```

### 5. Flash to hardware

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Project Structure

- `engine/` - Core rendering library (C++17, platform-independent)
- `firmware/` - ESP32 firmware using ESP-IDF
  - `src/main.cpp` - Firmware entry point
  - `CMakeLists.txt` - ESP-IDF component definition
- `simulator/` - Linux desktop simulator for testing
- `tools/svg_preprocessor/` - Converts SVG to binary gauge format

## Gauge File Preparation

Before building firmware, generate the gauge file:

```bash
cd /home/catachan/projects/digi-dash

# First, build the preprocessor
cd build && make -j4 && cd ..

# Generate gauge file
./build/tools/svg_preprocessor/svg_preprocessor \
    assets/dashboard_tiny.svg \
    assets/dashboard_tiny.gauge
```

The firmware will load `assets/dashboard_tiny.gauge` from SPIFFS.

## Troubleshooting

### CMake errors about engine not found
Make sure the engine is built before building firmware:
```bash
cd build && make digidash-engine
```

### SPIFFS errors in QEMU
SPIFFS needs to be pre-populated with the gauge file. This is handled by the build process if the gauge file exists at `assets/dashboard_tiny.gauge`.

### IDF_PATH not set
Run: `source ~/esp-idf/esp-idf/export.sh`

See [QEMU_SIMULATOR.md](QEMU_SIMULATOR.md) for more detailed QEMU setup.
