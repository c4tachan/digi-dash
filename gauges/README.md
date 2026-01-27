# README

## Digi-Dash: Customizable Automotive Digital Dashboard

A modular, high-performance digital dashboard system for the ESP32-S3 with a Linux desktop simulator. Features vector graphics rendering, real-time animations, OBD2 integration, and wireless gauge updates.

## Features

✅ **Platform-Agnostic Engine**: Shared C++ code compiled for both ESP32 and Linux simulator
✅ **Vector Graphics**: Renders cubic Bézier paths with anti-aliasing
✅ **Real-Time Animations**: Smooth gauge needle and element transitions
✅ **PID Binding System**: Direct mapping of OBD2 data to gauge elements
✅ **Binary Asset Format**: Pre-processed, size-optimized gauge files
✅ **Linux Simulator**: Rapid development without hardware
✅ **Bluetooth Support**: OBD2 data and wireless asset updates
✅ **Customizable Gauges**: User-created gauge definitions via SVG

## Quick Start

### Prerequisites

- CMake 3.16+
- SDL2 development libraries
- C++17 compiler (GCC 7+, Clang 5+)
- Python 3.6+ (for preprocessing tools)

### Install Dependencies

```bash
bash scripts/install_deps.sh
```

### Build & Run Simulator

```bash
bash scripts/build_sim.sh
./simulator/build/digi-dash-simulator
```

You'll see a window with simulated gauge data updating at 30 FPS.

## Project Structure

```
digi-dash/
├── engine/              # Shared rendering + animation engine (C++)
├── simulator/           # Linux desktop simulator (SDL2 + LVGL)
├── firmware/            # ESP32-S3 firmware (FreeRTOS + LVGL)
├── tools/               # Preprocessing tools (Python + C++)
├── gauges/              # Example gauges and templates
├── docs/                # Architecture, specifications, guides
└── scripts/             # Build and deployment helpers
```

## Key Components

### Engine (`engine/`)

Provides the core functionality:
- `VectorRenderer` - Rasterizes vector paths
- `BinaryGaugeLoader` - Loads gauge assets
- `AnimationEngine` - Keyframe animations with easing
- `GaugeScene` - Orchestrates rendering pipeline
- `PIDBindingSystem` - Maps OBD2 values to gauge elements
- `FontManager` - Bitmap font rendering

### Simulator (`simulator/`)

Development environment for rapid iteration:
- SDL2-based windowed interface
- Fake OBD2 PID provider with multiple modes
- Direct framebuffer rendering

### Firmware (`firmware/`)

Production code for ESP32-S3:
- FreeRTOS multitasking
- LVGL display driver integration
- Bluetooth OBD2 receiver
- Flash-based asset storage

## Usage Example

```cpp
#include "digidash/gauge_scene.h"

int main() {
    // Create gauge scene
    digidash::GaugeScene gauge;
    
    // Load a gauge asset
    BinaryGaugeLoader loader;
    GaugeAsset asset;
    loader.load_from_file("dashboard.gauge", asset);
    gauge.load_gauge(asset);
    
    // Update with OBD2 data
    gauge.set_pid_value(0x010C, 3500.0f);  // RPM
    gauge.set_pid_value(0x010D, 65.0f);    // Speed
    
    // Render to framebuffer
    uint8_t framebuffer[480*320*4];
    gauge.update(16);  // ~60 FPS
    gauge.render(framebuffer, 480, 320, 480*4);
}
```

## Documentation

- [Architecture Overview](docs/ARCHITECTURE_DETAILED.md) - System design and components
- [Binary Gauge Format](docs/gauge_format/BINARY_FORMAT.md) - Asset file specification
- [Rendering Pipeline](docs/rendering_pipeline/PIPELINE.md) - How pixels are rendered
- [Development Guide](docs/DEVELOPMENT.md) - Building, testing, and contributing

## Building for ESP32

With ESP-IDF installed:

```bash
# Configure and build
cd firmware
idf.py build

# Flash to device
idf.py flash

# Monitor serial output
idf.py monitor
```

## Performance Targets

| Metric | Target |
|--------|--------|
| Frame Rate | 30-60 FPS |
| Gauge Load Time | <1 second |
| Memory Usage | <2 MB (ESP32 PSRAM) |
| Power Consumption | <500 mW (idle) |

## Design Principles

1. **Platform Abstraction**: Core engine is platform-agnostic; platform-specific code isolated
2. **Pre-Processing**: Heavy computation done offline; runtime optimized for embedded
3. **Modular Architecture**: Components are independent and testable
4. **Linux-First Development**: Simulator enables 10x faster iteration
5. **Real-Time Safety**: No blocking operations in render path; heap pre-allocated

## License

Copyright 2026. See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Support

For issues, questions, or suggestions, please open an issue on GitHub.

---

**Status**: Active Development | **Version**: 0.1.0 | **Last Updated**: January 2026
