#!/bin/bash
# Complete workflow: rebuild gauge, regenerate SPIFFS, flash firmware and SPIFFS to device

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIRMWARE_DIR="${PROJECT_DIR}/firmware"
PORT="${1:-/dev/ttyACM0}"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "════════════════════════════════════════════════════"
echo "  Digi-Dash: Gauge → SPIFFS → Device Flash"
echo "════════════════════════════════════════════════════"
echo ""

# Step 1: Setup ESP-IDF environment
if [ -z "$IDF_PATH" ]; then
    if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        echo -e "${BLUE}🔧 Setting up ESP-IDF environment...${NC}"
        . "$HOME/esp/esp-idf/export.sh" > /dev/null 2>&1
    else
        echo "ERROR: IDF_PATH not set and $HOME/esp/esp-idf/export.sh not found"
        exit 1
    fi
fi

# Step 2: Rebuild gauge from SVG
echo -e "${BLUE}🎨 Rebuilding gauge from SVG...${NC}"
cd "$PROJECT_DIR"
./tools/svg_preprocessor/build/svg_preprocessor \
    assets/dashboard_tiny.svg \
    assets/dashboard_tiny.gauge

echo -e "${GREEN}   ✅ Gauge built${NC}"

# Step 3: Copy to SPIFFS image directory
echo -e "${BLUE}📦 Copying gauge to SPIFFS directory...${NC}"
cp -f assets/dashboard_tiny.gauge firmware/spiffs_image/dashboard_tiny.gauge
echo -e "${GREEN}   ✅ Copied to firmware/spiffs_image/${NC}"

# Step 4: Generate SPIFFS partition image
echo -e "${BLUE}🔨 Generating SPIFFS partition image...${NC}"
cd "$FIRMWARE_DIR"

# Read partition size from partitions.csv (SPIFFS is 0x90000 = 589824 bytes)
SPIFFS_SIZE=0x90000

python $IDF_PATH/components/spiffs/spiffsgen.py \
    $SPIFFS_SIZE \
    spiffs_image \
    spiffs.bin \
    --page-size 256 \
    --obj-name-len 32 \
    --meta-len 4 \
    --use-magic \
    --use-magic-len

echo -e "${GREEN}   ✅ SPIFFS image generated: $(ls -lh spiffs.bin | awk '{print $5}')${NC}"

# Step 5: Flash firmware (if not already done or if changed)
echo -e "${BLUE}🚀 Flashing firmware to $PORT...${NC}"
idf.py -p "$PORT" flash

# Step 6: Flash SPIFFS partition
echo -e "${BLUE}💾 Flashing SPIFFS to device at 0x370000...${NC}"
python -m esptool \
    --chip esp32s3 \
    -p "$PORT" \
    -b 460800 \
    --before=default_reset \
    --after=hard_reset \
    write_flash 0x370000 spiffs.bin

echo ""
echo -e "${GREEN}════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}  ✅ Complete! Device flashed successfully${NC}"
echo -e "${GREEN}════════════════════════════════════════════════════${NC}"
echo ""
echo "To monitor serial output:"
echo "  idf.py -p $PORT monitor"
