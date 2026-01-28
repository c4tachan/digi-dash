#!/bin/bash
# Helper script to rebuild gauge and flash to qemu emulator

set -e

PROJECT_DIR="/home/catachan/projects/digi-dash"
SVG_FILE="${1:-assets/dashboard_tiny.svg}"
GAUGE_FILE="${SVG_FILE%.*}.gauge"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║         Digi-Dash: SVG → Gauge → SPIFFS → QEMU            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Convert SVG to gauge
echo "📄 Converting SVG to gauge format..."
cd "$PROJECT_DIR"
"$PROJECT_DIR/tools/svg_preprocessor/build/svg_preprocessor" "$SVG_FILE" "$GAUGE_FILE"
echo "   ✅ Generated: $GAUGE_FILE"
echo ""

# Step 2: Copy to SPIFFS directory
echo "📦 Copying to SPIFFS image..."
cp "$GAUGE_FILE" "firmware/spiffs_image/${GAUGE_FILE##*/}"
echo "   ✅ Copied to SPIFFS"
echo ""

# Step 3: Regenerate spiffs.bin
echo "🔨 Rebuilding SPIFFS partition..."
python3 /home/catachan/esp/esp-idf/components/spiffs/spiffsgen.py \
  0x120000 firmware/spiffs_image \
  firmware/build/spiffs.bin > /dev/null 2>&1
echo "   ✅ SPIFFS rebuilt"
echo ""

# Step 4: Merge partitions into qemu_flash.bin
echo "⚙️  Rebuilding qemu_flash.bin..."
python3 << 'EOF'
# Create a 2MB qemu_flash.bin
qemu_flash = bytearray(0x200000)  # 2MB

# Read the bootloader
with open('firmware/build/bootloader/bootloader.bin', 'rb') as f:
    data = f.read()
    qemu_flash[0:len(data)] = data

# Read partition table
with open('firmware/build/partition_table/partition-table.bin', 'rb') as f:
    data = f.read()
    qemu_flash[0x8000:0x8000+len(data)] = data

# Read app
with open('firmware/build/digi-dash.bin', 'rb') as f:
    data = f.read()
    qemu_flash[0x10000:0x10000+len(data)] = data

# Read SPIFFS
with open('firmware/build/spiffs.bin', 'rb') as f:
    data = f.read()
    qemu_flash[0xe0000:0xe0000+len(data)] = data

# Write qemu_flash.bin
with open('firmware/build/qemu_flash.bin', 'wb') as f:
    f.write(qemu_flash)
EOF
echo "   ✅ qemu_flash.bin rebuilt"
echo ""

# Step 5: Start qemu
echo "🚀 Launching QEMU with updated gauge..."
echo "   Close the window to exit"
echo ""
pkill -9 qemu-system-xtensa 2>/dev/null || true
sleep 1
cd firmware && \
/home/catachan/qemu-esp-develop/build/qemu-system-xtensa \
  -machine esp32s3 \
  -drive file=build/qemu_flash.bin,if=mtd,format=raw \
  -serial mon:stdio \
  -display sdl

