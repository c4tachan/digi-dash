# Digi-Dash Project — Development Context for GitHub Copilot

This project is a custom multi-display automotive dashboard for a Toyota MR2 Spyder. 
The dashboard will run on an ESP32 and render gauges using LVGL. 
Development is happening on Linux, with a PC-based LVGL simulator for UI work and 
ESP32 firmware for deployment.

## Goals
- Build a modular, maintainable C++ codebase.
- Use LVGL for all UI rendering.
- Simulate the dashboard UI on Linux using the LVGL PC simulator (SDL2 backend).
- Deploy the same UI code to the ESP32 with minimal changes.
- Integrate OBD-II and CAN bus data later (mock data for now).

## Repository Structure
- simulator/  → LVGL PC simulator project (CMake + SDL2)
- firmware/   → ESP32 firmware (C++ with LVGL)
- tools/      → CAN/OBD-II utilities (optional)
- assets/     → icons, fonts, layout resources
- docs/       → architecture notes, wiring diagrams

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
- Linux development environment (Ubuntu).
- GNOME Terminal.
- CMake for the simulator.
- C++17 or newer.
- No Python or JavaScript for core logic.
- Avoid platform-specific hacks; keep code portable.

## Starting Tasks for Copilot
- Generate a CMakeLists.txt for the LVGL simulator.
- Set up SDL2 initialization and LVGL display driver.
- Create a basic LVGL screen with placeholder widgets.
- Add a simple update loop that animates mock RPM and speed values.
- Organize code into `src/` and `include/` inside the simulator folder.

Use this context to guide all suggestions and code generation.
