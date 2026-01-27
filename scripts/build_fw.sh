#!/bin/bash
# Build ESP32-S3 firmware using ESP-IDF

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIRMWARE_DIR="${PROJECT_DIR}/firmware"

if [ ! -d "$IDF_PATH" ]; then
    echo "Error: IDF_PATH not set. Please source the ESP-IDF environment setup."
    exit 1
fi

cd "$FIRMWARE_DIR"
echo "Building firmware with ESP-IDF..."
idf.py build

echo "Build complete!"
echo "Flash with: idf.py flash"
echo "Monitor with: idf.py monitor"
