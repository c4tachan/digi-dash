# OBD-II Implementation Summary

## Overview

The digi-dash simulator now supports reading real vehicle data from OBD-II Bluetooth adapters (ELM327 compatible). This allows you to display live engine metrics on the dashboard.

## Architecture

### Data Source Abstraction

The simulator uses a `DataSource` interface that can be implemented by different data providers:

- **MockData**: Generates simulated oscillating sensor values
- **OBD2DataSource**: Reads real vehicle data via ELM327 adapter

### Class Structure

```
DataSource (interface)
├── MockData
└── OBD2DataSource
    ├── Serial port handling (termios)
    ├── ELM327 protocol implementation
    └── OBD-II PID query/response parsing
```

## Implementation Details

### Serial Communication

- **Platform**: Linux (POSIX termios)
- **Baud Rate**: 38400 (configurable, ELM327 default)
- **Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **Flow Control**: None
- **Timeout**: 100ms per command with 2-second total timeout

### ELM327 Initialization Sequence

1. **Reset Adapter**: `ATZ\r` - Resets ELM327 to defaults
2. **Disable Echo**: `ATE0\r` - Turns off command echo
3. **Set Protocol**: `ATSP0\r` - Auto-detect vehicle protocol

### PID Query Protocol

Standard OBD-II Mode 01 PIDs:

| Parameter | Command | Response Format | Calculation |
|-----------|---------|-----------------|-------------|
| RPM | `010C\r` | `41 0C XX YY` | ((XX * 256) + YY) / 4 |
| Speed | `010D\r` | `41 0D XX` | XX km/h |
| Coolant Temp | `0105\r` | `41 05 XX` | XX - 40 °C |

### Response Parsing

The implementation:
1. Sends PID command with carriage return (`\r`)
2. Waits for response ending with `>` or containing "OK"/"ERROR"
3. Strips carriage returns, newlines, and prompt characters
4. Parses hex bytes using `sscanf()`
5. Applies PID-specific conversion formulas

### Error Handling

- **Connection Failures**: Auto-retry connection on next update
- **No Response**: Returns -1, keeps previous value
- **Parse Errors**: Logs error, returns -1
- **Disconnection**: Automatically attempts reconnection

## Configuration

PIDs are configured via `PidConfig` map:

```cpp
std::map<std::string, PidConfig> pid_config;
pid_config["rpm"] = {"010C", "RPM", "rpm"};
pid_config["speed"] = {"010D", "Speed", "km/h"};
pid_config["coolant_temp"] = {"0105", "Coolant Temperature", "°C"};
```

## Usage

### Command-Line Interface

```bash
# Mock data (simulated)
./digi-dash-simulator --mock

# Real OBD-II data
./digi-dash-simulator --obd2 /dev/rfcomm0

# Help
./digi-dash-simulator --help
```

### Bluetooth Setup Workflow

1. **Pair Adapter**: Use `bluetoothctl` to pair ELM327
2. **Create Serial Port**: `sudo rfcomm bind /dev/rfcomm0 <MAC> 1`
3. **Set Permissions**: `sudo usermod -a -G dialout $USER`
4. **Run Simulator**: `./digi-dash-simulator --obd2 /dev/rfcomm0`

## Performance

- **Query Rate**: ~50ms between PID queries (20 Hz)
- **Connection Time**: 1-2 seconds for initial ELM327 initialization
- **Memory**: Minimal overhead (~1KB for buffers)

## Limitations

### Current Implementation

- **Linux Only**: Uses POSIX termios (Windows/macOS need porting)
- **Serial Only**: No direct Bluetooth socket support
- **Standard PIDs**: Only implements basic Mode 01 PIDs
- **Single ECU**: Doesn't support multi-ECU vehicles
- **No CAN**: Direct CAN bus access not implemented

### ELM327 Adapter Compatibility

- **Recommended**: ELM327 v1.5 or newer
- **Avoid**: Cheap clones with fake version numbers
- **Tested**: Generic Bluetooth ELM327 adapters
- **Baud Rate**: Most use 38400, some older units use 9600

## Future Enhancements

Potential improvements:

1. **Extended PIDs**: Add support for more Mode 01 PIDs (fuel level, throttle, etc.)
2. **Mode 02**: Freeze frame data
3. **Mode 03/07**: Diagnostic trouble codes
4. **Mode 09**: Vehicle information (VIN, calibration ID)
5. **Automatic Baud Detection**: Try 9600 if 38400 fails
6. **CAN Filtering**: Optimize queries for specific ECUs
7. **Windows/macOS**: Port serial code to other platforms
8. **Direct Bluetooth**: Use Bluetooth sockets without RFCOMM
9. **Configuration File**: Load PID map from JSON

## Debugging

### Enable Verbose Logging

The implementation already logs:
- Connection attempts and status
- Failed queries
- Response parsing errors

### Testing Without Vehicle

1. Use mock mode: `--mock`
2. Test with OBD-II simulator software
3. Use serial port emulator (socat) for development

### Common Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| "Failed to open" | Device doesn't exist | Check `rfcomm bind` command |
| "Permission denied" | No dialout group | `sudo usermod -a -G dialout $USER` |
| "No response" | Vehicle off or adapter not ready | Turn ignition ON |
| Parse errors | Non-standard adapter | Try different ELM327 clone |

## Code Files

- [obd2_data_source.h](../simulator/include/obd2_data_source.h) - Interface
- [obd2_data_source.cpp](../simulator/src/obd2_data_source.cpp) - Implementation  
- [main.cpp](../simulator/src/main.cpp) - CLI argument parsing
- [config.h](../simulator/include/config.h) - PidConfig structure

## References

- [ELM327 Datasheet](https://www.elmelectronics.com/wp-content/uploads/2016/07/ELM327DS.pdf)
- [OBD-II PIDs (Wikipedia)](https://en.wikipedia.org/wiki/OBD-II_PIDs)
- [SAE J1979 Standard](https://www.sae.org/standards/content/j1979_201202/)
