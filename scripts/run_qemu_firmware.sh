#!/bin/bash
# Run QEMU firmware (ESP32-S3)

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIRMWARE_DIR="${PROJECT_DIR}/firmware"
SPIFFS_SIZE=0x120000
QEMU_BIN="${QEMU_BIN:-qemu-system-xtensa}"

if [ -z "$IDF_PATH" ]; then
    if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        . "$HOME/esp/esp-idf/export.sh" > /dev/null 2>&1
    else
        echo "ERROR: IDF_PATH not set and $HOME/esp/esp-idf/export.sh not found"
        exit 1
    fi
fi

if [ ! -f "$FIRMWARE_DIR/build/digi-dash.bin" ]; then
    "$PROJECT_DIR/scripts/build_qemu_firmware.sh"
fi

if [ ! -x "$QEMU_BIN" ]; then
    if command -v "$QEMU_BIN" >/dev/null 2>&1; then
        QEMU_BIN="$(command -v "$QEMU_BIN")"
    elif [ -x "$HOME/qemu-esp-develop/build/qemu-system-xtensa" ]; then
        QEMU_BIN="$HOME/qemu-esp-develop/build/qemu-system-xtensa"
    else
        echo "ERROR: QEMU binary not found. Set QEMU_BIN or add qemu-system-xtensa to PATH."
        exit 1
    fi
fi

cd "$FIRMWARE_DIR"

python3 "$IDF_PATH/components/spiffs/spiffsgen.py" \
  $SPIFFS_SIZE spiffs_image \
  build/spiffs.bin > /dev/null 2>&1

python3 << 'EOF'
# Build 2MB qemu_flash.bin from bootloader, partition table, app, and spiffs
qemu_flash = bytearray(0x200000)  # 2MB

with open('build/bootloader/bootloader.bin', 'rb') as f:
    data = f.read()
    qemu_flash[0x0:len(data)] = data

with open('build/partition_table/partition-table.bin', 'rb') as f:
    data = f.read()
    qemu_flash[0x8000:0x8000+len(data)] = data

with open('build/digi-dash.bin', 'rb') as f:
    data = f.read()
    qemu_flash[0x10000:0x10000+len(data)] = data

with open('build/spiffs.bin', 'rb') as f:
    data = f.read()
    qemu_flash[0xe0000:0xe0000+len(data)] = data

with open('build/qemu_flash.bin', 'wb') as f:
    f.write(qemu_flash)
EOF

# Choose display mode: set WINDOW=1 to open an SDL window, otherwise run headless
if [ "${WINDOW:-0}" = "1" ]; then
        QEMU_DISPLAY_OPTS=( -display sdl -monitor none -serial stdio )
else
        QEMU_DISPLAY_OPTS=( -nographic -monitor none -serial stdio )
fi

"$QEMU_BIN" \
    -machine esp32s3 \
    -drive file=build/qemu_flash.bin,if=mtd,format=raw \
    "${QEMU_DISPLAY_OPTS[@]}"
