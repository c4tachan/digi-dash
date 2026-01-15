# Digi-Dash Simulator

PC-based simulator for the Digi-Dash automotive dashboard UI using LVGL and SDL2.

## Features

- **RPM Gauge**: Arc-style gauge (0-8000 RPM)
- **Speed Display**: Digital speed display (km/h)
- **Coolant Temperature Gauge**: Vertical bar gauge with color coding
- **Mock Data Generator**: Simulates realistic sensor data changes
- **OBD-II Support**: Connect to real vehicle data via Bluetooth ELM327 adapters

## Prerequisites

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake git libsdl2-dev
```

### Fedora
```bash
sudo dnf install gcc gcc-c++ cmake git SDL2-devel
```

### Arch Linux
```bash
sudo pacman -S base-devel cmake git sdl2
```

## Building

1. **Clone and initialize submodules** (if not already done):
   ```bash
   cd /path/to/digi-dash
   git submodule update --init --recursive
   ```

2. **Build the simulator**:
   ```bash
   cd simulator
   mkdir build
   cd build
   cmake ..
   make -j$(nproc)
   ```

3. **Run the simulator**:
   ```bash
   # With mock data (default)
   ./digi-dash-simulator --mock
   
   # With real OBD-II data via Bluetooth
   ./digi-dash-simulator --obd2 /dev/rfcomm0
   
   # View all options
   ./digi-dash-simulator --help
   ```

## OBD-II Bluetooth Setup

To connect to a real vehicle via Bluetooth ELM327 adapter, see the detailed setup guide: [OBD-II Bluetooth Setup](../docs/OBD2_BLUETOOTH_SETUP.md)

Quick start:
```bash
# 1. Pair your ELM327 adapter
bluetoothctl
# ... follow pairing steps ...

# 2. Create RFCOMM connection
sudo rfcomm bind /dev/rfcomm0 XX:XX:XX:XX:XX:XX 1

# 3. Run simulator with OBD-II
./digi-dash-simulator --obd2 /dev/rfcomm0
```

## Project Structure

```
simulator/
├── CMakeLists.txt          # Build configuration
├── lv_conf.h              # LVGL configuration
├── include/               # Header files
│   ├── dashboard_screen.h # Main dashboard
│   ├── rpm_gauge.h        # RPM gauge widget
│   ├── speed_display.h    # Speed display widget
│   ├── temp_gauge.h       # Temperature gauge widget
│   └── mock_data.h        # Mock data generator
├── src/                   # Source files
│   ├── main.cpp           # Entry point
│   ├── dashboard_screen.cpp
│   ├── rpm_gauge.cpp
│   ├── speed_display.cpp
│   ├── temp_gauge.cpp
│   └── mock_data.cpp
└── lib/
    └── lvgl/              # LVGL library (git submodule)
```

## Controls

- **Close Window**: Click the X button or press Alt+F4 to exit

## Troubleshooting

### SDL2 not found
If CMake can't find SDL2, ensure the development packages are installed:
```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev

# Check installation
pkg-config --modversion sdl2
```

### Build errors with LVGL
Make sure the LVGL submodule is properly initialized:
```bash
git submodule update --init --recursive
```

### Display issues
The simulator is configured for an 800x480 display. To change this, edit [src/main.cpp](src/main.cpp) and modify `WINDOW_WIDTH` and `WINDOW_HEIGHT`.

## Next Steps

- Integrate real OBD-II/CAN bus data
- Port UI code to ESP32 firmware
- Add more gauges (oil pressure, fuel level, etc.)
- Implement night mode / themes
- Add touch input handling for ESP32

## License

This project is part of the Digi-Dash automotive dashboard system.
