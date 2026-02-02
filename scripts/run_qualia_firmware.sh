#!/bin/bash
# Flash and monitor Qualia S3 firmware

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIRMWARE_DIR="${PROJECT_DIR}/firmware"
PORT="${1:-/dev/ttyACM0}"

if [ -z "$IDF_PATH" ]; then
    if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        . "$HOME/esp/esp-idf/export.sh" > /dev/null 2>&1
    else
        echo "ERROR: IDF_PATH not set and $HOME/esp/esp-idf/export.sh not found"
        exit 1
    fi
fi

if [ ! -f "$FIRMWARE_DIR/build/digi-dash.bin" ]; then
    "$PROJECT_DIR/scripts/build_qualia_firmware.sh"
fi

cd "$FIRMWARE_DIR"

idf.py -p "$PORT" flash monitor
