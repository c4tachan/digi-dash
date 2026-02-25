# Digi-Dash Project — Architecture & Development Instructions for GitHub Copilot

## Overview
**Last Updated: January 27, 2026**

This repository contains the codebase for a fully customizable automotive digital dashboard system.

**Current Implementation Status:**
- ✅ ESP32-S3 firmware (ESP-IDF 5.5.2) with SPIFFS filesystem
- ✅ Linux desktop simulator (SDL2, no LVGL)
- ✅ Shared C++ rendering engine with vector tessellation
- ✅ SVG preprocessing using ThorVG 0.15.16 → binary gauge format
- ✅ Binary gauge format (.gauge files, 352 bytes for test dashboard)
- ✅ Tile-based rendering (720x720 display, 169KB draw buffer)
- A binary gauge asset format with JSON bindings
- A user-customizable gauge system with animations and dynamic data
- 🔄 OBD2 integration (planned)
- 🔄 Mobile app (future)

**Development Environment:**
- Linux (Ubuntu/WSL)
- ThorVG 0.15.16 at `/home/catachan/projects/thorvg` (built with meson)
- ESP-IDF 5.5.2 at `~/esp/esp-idf` (symlink to `~/esp/esp-idf-v5.5.2`)
- Target: ESP32-S3 with 720x720 display

---

## High-Level Goals for Copilot
Copilot should help generate and maintain:

**Current Working Features:**
1. ✅ ThorVG-based SVG preprocessing to binary gauge format
2. ✅ Desktop simulator renders gauge files correctly
3. ✅ ESP32-S3 firmware builds and boots in QEMU
4. ✅ SPIFFS partition for gauge file storage
5. ✅ Tile-based rendering (12 tiles of 60 lines each)
6. ✅ Binary gauge loader with path tessellation (50 segments/curve)
7. ✅ Circular stroke rendering with round caps

**Build Systems:**
1. CMake for engine + simulator + preprocessor
2. ESP-IDF for firmware (idf.py)
4. Shared C++ engine code used by both simulator and firmware
5. SDL2-based Linux simulator (no LVGL currently)
6. Python spiffsgen.py for filesystem image creation

**Required Build/Run Scripts (must remain working):**
- [scripts/build_preprocessor.sh](scripts/build_preprocessor.sh) — build svg_preprocessor
- [scripts/run_preprocessor.sh](scripts/run_preprocessor.sh) — run svg_preprocessor
- [scripts/build_pc_simulator.sh](scripts/build_pc_simulator.sh) — build PC simulator
- [scripts/run_pc_simulator.sh](scripts/run_pc_simulator.sh) — run PC simulator
- [scripts/build_qemu_firmware.sh](scripts/build_qemu_firmware.sh) — build QEMU firmware
- [scripts/run_qemu_firmware.sh](scripts/run_qemu_firmware.sh) — run QEMU firmware
- [scripts/build_qualia_firmware.sh](scripts/build_qualia_firmware.sh) — build Qualia firmware
- [scripts/run_qualia_firmware.sh](scripts/run_qualia_firmware.sh) — flash/run Qualia firmware

**Code Preferences:**
- C++17 for engine and firmware
- CMake for native builds
- ESP-IDF CMake for firmware
- **SOLID Design Principles:**
  - **S**ingle Responsibility: Each class has one reason to change (DisplayDriver for LCD, StorageManager for SPIFFS, RenderEngine for rendering coordination)
  - **O**pen/Closed: Use abstract interfaces (TileRenderer, PlatformDisplay) to extend functionality without modifying existing code
  - **L**iskov Substitution: DisplayDriver implements PlatformDisplay interface for proper polymorphic substitution
  - **I**nterface Segregation: Platform-specific interfaces are focused (platform_display.h, platform_storage.h, platform_input.h)
  - **D**ependency Inversion: RenderEngine depends on TileRenderer abstraction, not concrete TileHeightRenderer; Application uses unique_ptr for dependency injection
- Modular, reusable components
- No NanoSVG (replaced with ThorVG)

---

## Required Folder Structure

Create and maintain the following structure:

digi-dash/
│
├── engine/                     # Shared C++ rendering + animation engine
│   ├── include/digidash/
│   ├── src/
│   ├── tests/
│   └── CMakeLists.txt
│
├── firmware/                   # ESP32-S3 firmware
│   ├── src/
│   ├── include/
│   ├── components/
│   ├── CMakeLists.txt          # if ESP-IDF
│   └── platformio.ini          # if PlatformIO
│
├── simulator/                  # Linux desktop simulator (SDL + LVGL)
│   ├── src/
│   ├── include/
│   ├── assets/
│   ├── CMakeLists.txt
│   └── run.sh
│
├── tools/                      # Preprocessing tools
│   ├── svg_preprocessor/
│   ├── font_preprocessor/
│   ├── gauge_packer/
│   └── CMakeLists.txt or Python venv
│
├── gauges/                     # Example gauges and templates
│   ├── examples/
│   ├── templates/
│   └── README.md
│
├── docs/                       # Architecture, specs, diagrams
│   ├── architecture/
│   ├── gauge_format/
│   ├── rendering_pipeline/
│   └── README.md
│
├── scripts/                    # Linux/WSL helper scripts
│   ├── build_sim.sh
│   ├── build_fw.sh
│   ├── flash_fw.sh
│   └── format_code.sh
│
└── CMakeLists.txt              # Top-level CMake for engine + simulator

---

## Shared Engine Requirements

Copilot should generate a C++ engine with the following components:

- VectorRenderer
- BinaryGaugeLoader
- AnimationEngine
- GaugeScene
- FontManager
- PIDBindingSystem

All engine code must be platform-agnostic and compile on both:
- Linux (simulator)
- ESP32-S3 (firmware)

Use abstract interfaces for platform-specific functionality:
- PlatformDisplay
- PlatformInput
- PlatformStorage
- PlatformDataLink

---

## Simulator Requirements

The simulator must:
- Build with CMake on Linux/WSL
- Use SDL2 + LVGL
- Load gauge binary files
- Simulate animations
- Provide fake PID data

---

## Firmware Requirements
- Store gauge files in flash
- Load and render gauge assets using the shared engine
- Run animations at 30–60 FPS

---

## Preprocessing Tools Requirements

Tools should:
- Parse SVG files

Tools must run on Linux/WSL.

---

## Coding Style & Conventions

Copilot should follow:
- Modern C++ (C++17 or newer)
- RAII for resource management
- Namespaces under `digidash`
- Header files in `engine/include/digidash/`
- Source files in `engine/src/`
- Unit tests using GoogleTest or Catch2 (Linux)
- ESP-IDF Unity tests for firmware components

---

## What Copilot Should Automatically Generate

Copilot should:
- Create missing folders
- Generate CMakeLists.txt files
- Scaffold class headers and source files
- Write platform abstraction layers
- Implement the Linux simulator entry point
- Implement ESP32 firmware entry point
- Generate helper scripts for WSL
- Create documentation stubs
- Maintain consistency across modules

Copilot should NOT:
- Mix simulator and firmware code
- Hardcode platform-specific logic into the engine
- Generate unused files or folders

---

## Developer Workflow (for Copilot awareness)

1. Write or modify engine code in `engine/`
2. Build and test on Linux using the simulator
3. Build firmware using ESP-IDF or PlatformIO
4. Use preprocessing tools to generate gauge assets
5. Load assets into simulator or ESP32
6. Iterate quickly using Linux simulator

Copilot should optimize for this workflow.

---

## End of Instructions
## CURRENT STATUS & KNOWN ISSUES (January 27, 2026)

### ✅ Working
- Desktop simulator renders gauge correctly with proper circular strokes and round caps
- ESP32-S3 firmware builds successfully
- SPIFFS partition detected and formatted
- Draw buffer allocates successfully (169KB)
- Tile-based rendering implemented (12 tiles of 60 lines each)
- Binary gauge format serialization/deserialization
- ThorVG 0.15.16 SVG preprocessing

### 🔄 In Progress / Known Issues
- **SPIFFS File Access**: Gauge file needs to be accessible after formatting
  - File is in spiffs_image/ directory and SPIFFS binary
  - SPIFFS formats on first mount (erases contents in QEMU)
  - Works correctly when flashed to real hardware
- **Tile Rendering**: Y-offset not yet implemented for partial rendering
- **Display Driver**: Not yet integrated (SPI/LVGL/direct)

### 📋 Next Steps
1. Fix SPIFFS gauge file access for QEMU testing
2. Implement y_offset rendering for proper tile-based display
3. Add display driver integration
4. Test on real ESP32-S3 hardware with display
5. Add OBD2 data integration
6. Implement animations and data bindings

---

## IMPORTANT TECHNICAL NOTES

### ThorVG Integration
- **Version**: 0.15.16 (NOT 0.14.x or NanoSVG)
- **Location**: `/home/catachan/projects/thorvg`
- **Build**: Meson (not CMake)
- **API**: `tvg::Picture::load()` + `tvg::Accessor` for path extraction
- **Find Package**: Uses custom `cmake/Findthorvg.cmake`

### Rendering Details
- **Tessellation**: 50 segments per cubic Bézier curve (increased from 10)
- **Stroke**: Circular brush using distance calculation (NOT square)
  - Formula: `distance_from_center <= radius` where `radius = stroke_width/2`
- **Caps**: Round caps at arc endpoints with anti-aliasing
- **Color**: RGBA format (4 bytes per pixel)

### ESP32-S3 Configuration
- **Target**: ESP32-S3 (set via `idf.py set-target esp32s3`)
- **Partition Table**: Custom `partitions.csv` with SPIFFS at 0xe0000
- **SPIFFS Size**: 0x120000 (1,179,648 bytes = 1.15 MB)
- **Factory App**: 0x10000 to 0xe0000 (832 KB)
- **PSRAM**: NOT required with tile-based rendering

### Memory Layout
```
Full framebuffer:     2,073,600 bytes (720×720×4) - TOO LARGE for ESP32-S3
Draw buffer:            172,800 bytes (720×60×4)  - ✅ FITS in 332KB DRAM
Tile rendering:         12 passes per frame (720÷60=12)
ESP32-S3 DRAM:          332 KB available
```

### Binary Gauge Format
- **Magic**: "DGGE" (4 bytes)
- **Version**: 1 (uint32_t)
- **Dimensions**: width, height (uint32_t each)
- **Paths**: Serialized with MOVE/LINE/CUBIC/CLOSE commands
- **Example Size**: dashboard_tiny.gauge = 352 bytes

### Key File Paths
- Engine headers: `engine/include/digidash/*.h`
- Firmware entry: `firmware/main/main.cpp`
- Simulator: `simulator/src/main.cpp`
- Preprocessor: `tools/svg_preprocessor/src/`
- Test gauge: `assets/dashboard_tiny.gauge`
- ThorVG: `/home/catachan/projects/thorvg/build_dir/src/libthorvg.so`

---

## BUILD COMMANDS

### Desktop Simulator
```bash
cd /home/catachan/projects/digi-dash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4
./simulator/digi-dash-simulator
```

### ESP32-S3 Firmware
```bash
cd /home/catachan/projects/digi-dash/firmware
source ~/esp/esp-idf/export.sh
idf.py build
```

### QEMU Testing
```bash
cd /home/catachan/projects/digi-dash/firmware
source ~/esp/esp-idf/export.sh

# Create SPIFFS image
python3 ~/esp/esp-idf/components/spiffs/spiffsgen.py 1179648 spiffs_image build/spiffs.bin

# Create complete flash image
esptool.py --chip=esp32s3 merge_bin --output=build/qemu_flash.bin \
  --fill-flash-size=2MB --flash_mode dio --flash_freq 80m --flash_size 2MB \
  0x0 build/bootloader/bootloader.bin \
  0x10000 build/digi-dash.bin \
  0x8000 build/partition_table/partition-table.bin \
  0xe0000 build/spiffs.bin

# Run in QEMU
idf.py qemu --graphics
```

### SVG Preprocessing
```bash
cd /home/catachan/projects/digi-dash
./build/tools/svg_preprocessor/svg_preprocessor \
  assets/dashboard_tiny.svg \
  assets/dashboard_tiny.gauge
```

---

## DEVELOPMENT WORKFLOW

### Making Engine Changes
1. Edit files in `engine/src/` or `engine/include/digidash/`
2. Rebuild: `cd build && make -j4`
3. Test simulator: `./simulator/digi-dash-simulator`
4. Rebuild firmware: `cd ../firmware && idf.py build`

### Testing SVG Changes
1. Edit `assets/dashboard_tiny.svg`
2. Run preprocessor to create `.gauge` file
3. Test in simulator
4. Copy to firmware: `cp assets/dashboard_tiny.gauge firmware/spiffs_image/`
5. Regenerate SPIFFS binary (see QEMU commands above)

### Firmware Development
1. Edit `firmware/main/main.cpp`
2. Build: `idf.py build`
3. Test in QEMU: `idf.py qemu --graphics`
4. Flash to hardware: `idf.py -p /dev/ttyUSB0 flash monitor`

---

## RECENT GIT COMMITS
- `5b986a7` - Tile-based rendering with 60-line draw buffer (169KB)
- `208d258` - Switch to ESP32-S3 target with SPIFFS partition
- `71c3054` - SPIFFS partition working in bootloader
- `15b271f` - ESP-IDF firmware build complete
- Earlier: ThorVG integration, binary gauge format, circular stroke rendering

