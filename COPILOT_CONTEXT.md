# Digi-Dash Project — Architecture & Development Instructions for GitHub Copilot

## Overview
This repository contains the codebase for a fully customizable automotive digital dashboard system. The system includes:

- ESP32-S3 firmware (ESP-IDF or PlatformIO)
- A Linux desktop simulator (SDL + LVGL)
- A shared C++ rendering and animation engine
- A preprocessing toolchain for SVG → vector bytecode → gauge assets
- A future mobile app pipeline (Android/iOS)
- A binary gauge asset format with JSON bindings
- A user-customizable gauge system with animations and dynamic data

All development is done on Linux (Ubuntu or WSL). All build systems must support Linux-first workflows.

---

## High-Level Goals for Copilot
Copilot should help generate and maintain:

1. A clean, scalable folder structure
2. CMake-based build systems for engine + simulator
3. ESP32 firmware build system (ESP-IDF or PlatformIO)
4. Shared C++ engine code used by both simulator and firmware
5. A Linux simulator using SDL + LVGL
6. Preprocessing tools (Python or C++)
7. Documentation and templates
8. Helper scripts for WSL/Linux development

Copilot should always prefer:
- C++
- CMake
- Cross-platform abstractions
- Clean architecture
- Modular components
- Reusable code between firmware and simulator

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
- PlatformBluetooth

---

## Simulator Requirements

The simulator must:
- Build with CMake on Linux/WSL
- Use SDL2 + LVGL
- Load gauge binary files
- Simulate animations
- Provide fake PID data
- Hot-reload gauge assets if possible

---

## Firmware Requirements

The firmware must:
- Build with ESP-IDF or PlatformIO
- Use LVGL for rendering
- Receive gauge files via Bluetooth
- Store gauge files in flash
- Load and render gauge assets using the shared engine
- Run animations at 30–60 FPS

---

## Preprocessing Tools Requirements

Tools should:
- Parse SVG files
- Resolve transforms
- Convert shapes to cubic Bézier paths
- Convert static text to vector outlines
- Convert dynamic text to LVGL bitmap fonts
- Package everything into a binary gauge file
- Output JSON bindings + vector data

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