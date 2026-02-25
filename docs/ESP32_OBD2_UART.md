# ESP32 OBD2 UART Implementation

This document describes the current ESP32 firmware implementation for reading live OBD2 data using a **wired ELM327 UART connection**.

## Overview

The firmware polls real vehicle PIDs over UART and feeds values directly into the renderer. No Bluetooth transport is used.

## Hardware Wiring (Qualia S3)

- `A1` (`GPIO16`) → ELM327 RX
- `A0` (`GPIO17`) → ELM327 TX
- GND ↔ GND

Notes:
- UART is configured as `38400`, `8N1`, no flow control.
- The current implementation uses `UART1`.

## Firmware Components

- `firmware/main/subsystems/obd2/obd2_uart_manager.h`
- `firmware/main/subsystems/obd2/obd2_uart_manager.cpp`

Application integration:
- `firmware/main/application/application.h`
- `firmware/main/application/application.cpp`

Build registration:
- `firmware/main/CMakeLists.txt`

## ELM327 Initialization Sequence

1. `ATZ`
2. `ATE0`
3. `ATL0`
4. `ATH0`
5. `ATSP0`

## PID Query Cycle

The manager rotates through:

| Command | PID | Description | Conversion |
|---|---|---|---|
| `010C` | `0x0C` | Engine RPM | `(A*256 + B) / 4` |
| `010D` | `0x0D` | Vehicle speed | `A` km/h |
| `0105` | `0x05` | Coolant temp | `A - 40` °C |

Renderer PID mapping:
- RPM → `pid_id = 0`
- Speed → `pid_id = 1`
- Coolant temp → `pid_id = 3`

## Build / Flash / Monitor

```bash
export_script=""; for p in "$HOME/esp/esp-idf/export.sh" "$HOME/.espressif/frameworks/esp-idf-v5.2.1/export.sh" "$HOME/.espressif/frameworks/esp-idf-v5.2/export.sh" "$HOME/.espressif/esp-idf/export.sh"; do if [[ -f "$p" ]]; then export_script="$p"; break; fi; done; if [[ -z "$export_script" ]]; then echo "No ESP-IDF export.sh found"; exit 1; fi; . "$export_script" && idf.py -C /home/c4tachan/projects/digi-dash/firmware -p /dev/ttyACM0 flash monitor
```

If serial monitor fails due to a busy device, close other serial tools and retry.

## Troubleshooting

### No PID updates

- Verify wiring TX/RX is crossed correctly.
- Confirm common ground is connected.
- Confirm ignition is ON.
- Try a different ELM327 adapter (some clones are unreliable).

### UART not receiving data

- Confirm adapter baud is `38400`.
- Probe RX/TX with a USB-UART adapter if needed.

## References

- [ELM327 Datasheet](https://www.elmelectronics.com/wp-content/uploads/2016/07/ELM327DS.pdf)
- [OBD-II PIDs](https://en.wikipedia.org/wiki/OBD-II_PIDs)
