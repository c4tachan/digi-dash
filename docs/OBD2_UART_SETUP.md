# OBD-II UART Setup

This guide explains how to run digi-dash with a **wired ELM327 adapter over UART/serial**.

## Firmware (ESP32 Qualia)

### Wiring

- `A1 (GPIO16)` → ELM327 RX
- `A0 (GPIO17)` → ELM327 TX
- GND ↔ GND

### Build/Flash/Monitor

```bash
export_script=""; for p in "$HOME/esp/esp-idf/export.sh" "$HOME/.espressif/frameworks/esp-idf-v5.2.1/export.sh" "$HOME/.espressif/frameworks/esp-idf-v5.2/export.sh" "$HOME/.espressif/esp-idf/export.sh"; do if [[ -f "$p" ]]; then export_script="$p"; break; fi; done; if [[ -z "$export_script" ]]; then echo "No ESP-IDF export.sh found"; exit 1; fi; . "$export_script" && idf.py -C /home/c4tachan/projects/digi-dash/firmware -p /dev/ttyACM0 flash monitor
```

## Simulator (Linux)

The simulator uses serial devices directly.

### Typical command

```bash
cd /home/c4tachan/projects/digi-dash/simulator/build
./digi-dash-simulator --obd2 /dev/ttyUSB0
```

If your adapter enumerates differently, use the correct path (for example `/dev/ttyACM0` or `/dev/ttyUSB1`).

## Troubleshooting

- `Permission denied`: add your user to `dialout` and re-login.
- `No response`: verify ignition is ON and wiring/serial path is correct.
- `Busy device`: close other tools using the same serial port.
