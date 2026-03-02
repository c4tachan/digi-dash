# ESP32 OBD2 Bluetooth Implementation

This document describes the ESP32 firmware implementation for connecting to the OBDLink LX adapter via Bluetooth Classic SPP.

## Overview

The firmware automatically discovers, pairs with, and connects to the OBDLink LX Bluetooth OBD2 adapter to read real-time vehicle data. No simulated PIDs are used - all data comes from the vehicle's ECU.

## Current Firmware Path: Wired ELM327 over UART

The current active firmware path uses a **wired ELM327 UART adapter** (not Bluetooth) via Qualia analog header pins.

### Pin Mapping (Adafruit Qualia S3)

- `A1` = `GPIO16` → ESP32 TX (connect to ELM RX)
- `A0` = `GPIO17` → ESP32 RX (connect to ELM TX)
- Common GND between ESP32 and ELM327 is required

### Firmware Module

- `subsystems/obd2/obd2_uart_manager.cpp`
- Defaults: UART1, 38400 baud, `TX=GPIO16`, `RX=GPIO17`
- ELM init sequence: `ATZ`, `ATE0`, `ATL0`, `ATH0`, `ATSP0`
- PID cycle: `010C` (RPM), `010D` (Speed), `0105` (Coolant)

### Build / Flash / Monitor (with export autodetect)

```bash
export_script=""; for p in "$HOME/esp/esp-idf/export.sh" "$HOME/.espressif/frameworks/esp-idf-v5.2.1/export.sh" "$HOME/.espressif/frameworks/esp-idf-v5.2/export.sh" "$HOME/.espressif/esp-idf/export.sh"; do if [[ -f "$p" ]]; then export_script="$p"; break; fi; done; . "$export_script" && idf.py -C /home/c4tachan/projects/digi-dash/firmware -p /dev/ttyACM0 flash monitor
```

If monitor fails because the serial port is busy, close other serial tools and retry the same command.

## Architecture

### OBD2Manager (`subsystems/obd2/`)

Core responsibilities:
- Bluetooth Classic initialization and device discovery
- SPP (Serial Port Profile) connection management
- ELM327 protocol implementation
- Asynchronous response parsing
- PID query scheduling and parsing

### Key Features

1. **Automatic Discovery** - Scans for "OBDLink LX" device name
2. **Asynchronous Communication** - Non-blocking SPP data handling
3. **Connection Resilience** - Automatic reconnection on disconnect
4. **Status Feedback** - Real-time connection state callbacks

## Connection State Machine

```
DISCONNECTED → CONNECTING → INITIALIZING → CONNECTED
     ↑                                          ↓
     └──────────────────────────────────────────┘
                  (on disconnect)
```

### States

- **DISCONNECTED**: Initial state, no Bluetooth activity
- **CONNECTING**: Bluetooth discovery active, searching for OBDLink LX
- **INITIALIZING**: SPP connected, sending ELM327 init commands
- **CONNECTED**: ELM327 ready, reading PIDs
- **ERROR**: Fatal error occurred, will retry

## ELM327 Protocol Implementation

### Initialization Sequence

1. **ATZ** - Reset adapter, wait 500ms
2. **ATE0** - Disable echo, wait 500ms  
3. **ATSP0** - Auto protocol detection, wait 500ms

### PID Query Cycle

Rotates through 3 PIDs at 10 Hz (100ms interval):

| Command | PID | Description | Response Format | Calculation |
|---------|-----|-------------|-----------------|-------------|
| 010C | 0x0C | Engine RPM | 41 0C AA BB | (A×256 + B) / 4 |
| 010D | 0x0D | Vehicle Speed | 41 0D AA | A (km/h) |
| 0105 | 0x05 | Coolant Temp | 41 05 AA | A - 40 (°C) |

### Response Parsing

Responses arrive asynchronously via SPP data callback. The response buffer accumulates data until a prompt character (`>`) is received, then the complete response is parsed:

```
Input:  "41 0C 1A F8>"
Parse:  mode=41, pid=0C, A=1A, B=F8
Calculate: RPM = (0x1A × 256 + 0xF8) / 4 = 1726 RPM
```

## Application Integration

### Initialization (application.cpp)

```cpp
// Create OBD2 manager
obd2_ = std::make_unique<OBD2Manager>();

// Register callbacks
obd2_->set_pid_callback([this](uint32_t pid_id, float value) {
    renderer_->set_pid_value(pid_id, value);
});

obd2_->set_state_callback([this](ConnectionState state, const char* msg) {
    obd2_state_ = state;
    ESP_LOGI(TAG, "OBD2: %s", msg);
});

// Initialize Bluetooth and start discovery
obd2_->initialize();
```

### Main Loop

```cpp
while (true) {
    // Update OBD2 (send queued commands, process responses)
    obd2_->update();
    
    if (obd2_state_ != OBD2Manager::ConnectionState::CONNECTED) {
        // Display connection status in center of screen
        display_connection_status(obd2_->get_status_message());
    } else {
        // Render gauge with real PID data
        renderer_->render_frame();
    }
    
    vTaskDelay(33ms); // 30 FPS
}
```

## Configuration

### sdkconfig.defaults

```ini
# Bluetooth Classic for OBD2
CONFIG_BT_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=y
CONFIG_BT_CLASSIC_ENABLED=y
CONFIG_BT_SPP_ENABLED=y
CONFIG_BT_BLE_ENABLED=n  # Disable BLE to save memory
```

### CMakeLists.txt

```cmake
SRCS "subsystems/obd2/obd2_manager.cpp"
INCLUDE_DIRS "subsystems/obd2"
REQUIRES ... bt  # Add bt component
```

## Memory Considerations

- **BLE Disabled**: Releases ~50KB of RAM for Classic BT
- **Response Buffer**: 256 bytes for ELM327 responses
- **Static Allocation**: No dynamic allocation in OBD2Manager

## Hardware Requirements

- **ESP32-S3** with Bluetooth Classic support
- **OBDLink LX** Bluetooth OBD2 adapter
- **Vehicle** with OBD2 port (1996+ US vehicles)

## Bluetooth Classic vs BLE

The OBDLink LX uses Bluetooth Classic SPP (Serial Port Profile), NOT BLE. This is why we disable BLE and enable Classic:

```
Classic SPP = Serial over Bluetooth (like virtual COM port)
BLE = Low-energy peripheral/central model
```

ELM327 adapters universally use Classic SPP for compatibility.

## Testing

### Build & Flash

```bash
cd firmware
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Expected Serial Output

```
[OBD2Manager] Initializing Bluetooth Classic for OBD2...
[OBD2Manager] Bluetooth initialized, searching for OBDLink LX...
[OBD2Manager] Found device: OBDLink LX
[OBD2Manager] SPP connection opened!
[OBD2Manager] Initializing ELM327 step 0...
[OBD2Manager] ELM327 response: ELM327 v1.5>
[OBD2Manager] Initializing ELM327 step 1...
[OBD2Manager] ELM327 response: OK>
[OBD2Manager] Initializing ELM327 step 2...
[OBD2Manager] ELM327 response: OK>
[OBD2Manager] ELM327 initialized successfully!
[OBD2Manager] State: 3 - Connected!
[Application] OBD2: Connected!
```

### Verify PID Updates

Enable verbose logging in obd2_manager.cpp to see PID query/response cycle:

```
[OBD2Manager] Sending: 010C
[OBD2Manager] Response: 41 0C 1A F8>
[OBD2Manager] RPM: 1726.0
[OBD2Manager] Sending: 010D  
[OBD2Manager] Response: 41 0D 3C>
[OBD2Manager] Speed: 60.0
```

## Troubleshooting

### "Device not found" Loop

**Problem**: Discovery completes without finding OBDLink LX

**Solutions**:
- Ensure vehicle ignition is ON (powers OBD2 port)
- Check OBDLink is not paired to phone/another device
- Verify device name matches exactly "OBDLink LX"
- Try power-cycling OBDLink (unplug/replug from OBD2 port)

### SPP Connection Fails

**Problem**: Device found but SPP connection fails

**Solutions**:
- OBDLink may be paired to another device (unpair from all devices)
- Check ESP32 Bluetooth antenna is connected (Qualia has onboard antenna)
- Verify `CONFIG_BT_SPP_ENABLED=y` in sdkconfig

### No PID Responses

**Problem**: Connected but no PID data

**Solutions**:
- Confirm vehicle is running or in accessory mode (some ECUs sleep when off)
- Try different PIDs - not all vehicles support all PIDs
- Check ELM327 initialization succeeded (look for "OK" responses)
- Some vehicles require specific protocol (try ATSP6 for CAN)

### Connection Drops

**Problem**: Intermittent disconnections

**Solutions**:
- Check OBD2 port power stability (loose connector)
- Reduce query rate (increase `PID_UPDATE_INTERVAL_MS`)
- Verify ground/power to display (power fluctuations can reset BT)

## Future Enhancements

### Planned Features

- [ ] Connection status text rendering (currently just logs)
- [ ] Configurable PID list from gauge file
- [ ] DTC (Diagnostic Trouble Code) reading
- [ ] Freeze frame data
- [ ] Live sensor graph mode
- [ ] Multiple adapter support (choose from list)
- [ ] Bluetooth PIN/passkey support

### Performance Optimizations

- [ ] Batch PID queries (some adapters support multiple PIDs in one query)
- [ ] Adaptive update rate (slow down when values stable)
- [ ] Protocol detection cache (skip ATSP0 on reconnect)

## References

- [ESP-IDF Bluetooth Classic API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetoot/esp_bt_main.html)
- [ELM327 Command Reference](https://www.elmelectronics.com/wp-content/uploads/2016/07/ELM327DS.pdf)
- [OBD-II PID Codes](https://en.wikipedia.org/wiki/OBD-II_PIDs)
- [OBDLink LX Datasheet](https://www.obdlink.com/products/obdlink-lx/)
