#!/bin/bash
# Build Qualia S3 firmware

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIRMWARE_DIR="${PROJECT_DIR}/firmware"

if [ -z "$IDF_PATH" ]; then
    if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        . "$HOME/esp/esp-idf/export.sh" > /dev/null 2>&1
    else
        echo "ERROR: IDF_PATH not set and $HOME/esp/esp-idf/export.sh not found"
        exit 1
    fi
fi

cd "$FIRMWARE_DIR"

idf.py build

echo "✅ Qualia firmware built"
