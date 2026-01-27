#!/bin/bash
# Flash ESP32-S3 firmware

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIRMWARE_DIR="${PROJECT_DIR}/firmware"

if [ ! -d "$IDF_PATH" ]; then
    echo "Error: IDF_PATH not set. Please source the ESP-IDF environment setup."
    exit 1
fi

cd "$FIRMWARE_DIR"

PORT="${1:-/dev/ttyUSB0}"
echo "Flashing firmware to $PORT..."

idf.py -p "$PORT" flash
idf.py -p "$PORT" monitor
