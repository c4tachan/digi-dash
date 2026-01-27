# Digi-Dash Architecture

## Overview

Digi-Dash is a fully customizable automotive digital dashboard system built for the ESP32-S3 with a Linux desktop simulator. The architecture emphasizes modularity, platform abstraction, and code reuse between firmware and simulator.

## Core Components

### 1. Shared Engine (`engine/`)

The heart of the system, providing:

- **VectorRenderer**: Renders vector paths (cubic Bézier curves) at high quality
- **BinaryGaugeLoader**: Loads pre-processed gauge assets from binary files
- **AnimationEngine**: Manages keyframe animations, easing, and transitions
- **GaugeScene**: Orchestrates rendering, animations, and data binding
- **FontManager**: Manages LVGL bitmap fonts for text rendering
- **PIDBindingSystem**: Maps OBD2 PID values to gauge elements

All engine code is platform-agnostic and compiles on both Linux and ESP32.

### 2. Platform Abstractions

The engine depends on abstract interfaces implemented by platform-specific code:

- **PlatformDisplay**: Framebuffer access (SDL2 on Linux, LVGL on ESP32)
- **PlatformInput**: Button and touch input handling
- **PlatformStorage**: File/flash storage operations
- **PlatformBluetooth**: Bluetooth communication for OBD2 and asset transfer

### 3. Linux Simulator (`simulator/`)

A development environment using:

- **SDL2**: Window management and rendering
- **Fake PID Provider**: Simulates OBD2 data with various patterns
- **SDLDisplay**: Platform implementation using SDL2
- **SDLInput**: Keyboard/mouse input handling

### 4. ESP32 Firmware (`firmware/`)

Production code for the ESP32-S3:

- **LVGL Display**: Full-featured display rendering
- **FreeRTOS Tasks**: Real-time execution
- **Bluetooth Stack**: OBD2 data reception and asset updates
- **Flash Storage**: Gauge asset persistence

### 5. Preprocessing Tools (`tools/`)

Command-line utilities:

- **svg_preprocessor**: Converts SVG → vector bytecode
- **font_preprocessor**: Converts fonts → LVGL bitmap fonts
- **gauge_packer**: Assembles assets into binary gauge files

## Build System

### CMake (Linux + Simulator)

```bash
cd digi-dash/simulator/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4
./digi-dash-simulator
```

### ESP-IDF (Firmware)

```bash
cd digi-dash/firmware
idf.py build
idf.py flash
idf.py monitor
```

## Data Flow

### Gauge Rendering

```
1. SVG file
2. svg_preprocessor (CPU-intensive, runs once)
3. Binary gauge file (size-optimized)
4. BinaryGaugeLoader (ESP32/simulator)
5. GaugeScene
6. VectorRenderer → Framebuffer
```

### Live Updates

```
1. OBD2 Bluetooth → PIDBindingSystem
2. GaugeScene::set_pid_value()
3. AnimationEngine updates needle position
4. VectorRenderer outputs to display
```

## Code Organization

```
engine/include/digidash/    # Public headers
engine/src/                 # Implementation
simulator/                  # Linux development environment
firmware/                   # ESP32-S3 production code
tools/                      # Preprocessing and utilities
docs/                       # Architecture and specifications
scripts/                    # Build and deployment helpers
```

## Key Design Decisions

1. **Binary Gauge Format**: Pre-processed assets reduce ESP32 memory and CPU usage
2. **Platform Abstraction**: Shared engine code with pluggable implementations
3. **C++17**: Modern language features for cleaner, more efficient code
4. **No Dynamic Memory in Hot Paths**: Real-time constraints on ESP32
5. **Linux-First Development**: Simulator enables rapid iteration

## Next Steps

- Implement VectorRenderer with cubic Bézier rasterization
- Create binary gauge file format specification
- Develop SVG preprocessing pipeline
- Implement LVGL platform layer for ESP32
- Add Bluetooth OBD2 protocol handling
