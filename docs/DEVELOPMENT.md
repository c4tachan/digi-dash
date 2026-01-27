# Project Development Guide

## Quick Start

### 1. Install Dependencies

```bash
cd digi-dash
bash scripts/install_deps.sh
```

### 2. Build the Simulator

```bash
bash scripts/build_sim.sh
```

### 3. Run the Simulator

```bash
./simulator/build/digi-dash-simulator
```

You should see a black window with simulated PID data updating the display.

## Development Workflow

### Adding a New Engine Component

1. Create header in `engine/include/digidash/`
2. Create implementation in `engine/src/`
3. Update `engine/CMakeLists.txt` to compile new files
4. Add tests in `engine/tests/`

### Adding a Platform Implementation

1. Create header in `simulator/include/` or `firmware/include/`
2. Implement interface defined in `engine/include/digidash/platform_*.h`
3. Link in simulator or firmware CMakeLists.txt

### Testing the Simulator

Build and run:
```bash
bash scripts/build_sim.sh
./simulator/build/digi-dash-simulator
```

The simulator displays:
- Black background (RGBA 0,0,0,255)
- Simulated OBD2 values updating the gauge
- 30 FPS rendering loop

Press `Q` or close the window to exit.

## Code Style

- **C++ Standard**: C++17
- **Naming**: snake_case for functions/variables, PascalCase for classes
- **Headers**: Guard with `#pragma once`
- **Namespaces**: Everything under `digidash::`
- **Comments**: Doxygen format for public APIs

Format code with:
```bash
bash scripts/format_code.sh
```

## Common Tasks

### Build Engine Only

```bash
mkdir engine/build && cd engine/build
cmake ..
make
```

### Build with Debug Symbols

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
gdb ./digi-dash-simulator
```

### Update Dependencies

```bash
sudo apt-get update
bash scripts/install_deps.sh
```

## Troubleshooting

### CMake: "SDL2 not found"

```bash
sudo apt-get install libsdl2-dev libsdl2-image-dev
```

### Runtime: "Segmentation fault in VectorRenderer"

- Check framebuffer allocation in SDLDisplay
- Verify path data is valid
- Use valgrind: `valgrind ./digi-dash-simulator`

### Slow Build Times

Use Ninja instead of Make:
```bash
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

## Git Workflow

1. Create a feature branch: `git checkout -b feature/my-feature`
2. Make changes and test
3. Commit with clear messages: `git commit -m "Add VectorRenderer cubic curve support"`
4. Push: `git push origin feature/my-feature`
5. Submit pull request with description

## Next Development Tasks

1. Implement actual cubic Bézier rasterization in VectorRenderer
2. Create SVG preprocessing pipeline
3. Implement binary gauge file loader
4. Add LVGL display support for ESP32
5. Implement Bluetooth OBD2 data reception
6. Create example gauges (RPM, speed, temperature)
