#!/bin/bash
# Quick test script for digi-dash development environment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================="
echo "  Digi-Dash Environment Test"
echo "========================================="
echo ""

# Test 1: SDL2 Simulator
echo "📺 Testing SDL2 Simulator..."
if [ -f "simulator/build/digi-dash-simulator" ]; then
    cd simulator/build
    echo "   Running simulator for 5 seconds..."
    timeout 5 ./digi-dash-simulator 2>&1 | head -10
    cd ../..
    echo "   ✅ SDL2 Simulator works!"
else
    echo "   ❌ SDL2 Simulator not built. Run: cd simulator/build && cmake .. && make"
fi
echo ""

# Test 2: ESP-IDF
echo "🔧 Testing ESP-IDF..."
if [ -d "$HOME/esp/esp-idf" ]; then
    source "$HOME/esp/esp-idf/export.sh" > /dev/null 2>&1
    IDF_VER=$(idf.py --version 2>&1 | head -1)
    echo "   $IDF_VER"
    echo "   ✅ ESP-IDF installed!"
else
    echo "   ❌ ESP-IDF not found. Install: cd ~/esp && git clone ... (see COPILOT_CONTEXT.md)"
fi
echo ""

# Test 3: ESP32 Firmware Build
echo "🔨 Testing ESP32 Firmware Build..."
if [ -f "esp32-qemu/digi-dash-esp32/build/digi-dash-esp32.elf" ]; then
    SIZE=$(stat -f%z "esp32-qemu/digi-dash-esp32/build/digi-dash-esp32.elf" 2>/dev/null || stat -c%s "esp32-qemu/digi-dash-esp32/build/digi-dash-esp32.elf")
    echo "   Firmware size: $(numfmt --to=iec-i --suffix=B $SIZE 2>/dev/null || echo $SIZE bytes)"
    echo "   ✅ ESP32 Firmware built!"
else
    echo "   ❌ ESP32 Firmware not built. Run: cd esp32-qemu/digi-dash-esp32 && idf.py build"
fi
echo ""

# Test 4: QEMU (esp-qemu)
echo "🖥️  Testing QEMU..."
if command -v qemu-system-xtensa &> /dev/null; then
    QEMU_VER=$(qemu-system-xtensa --version 2>&1 | head -1)
    echo "   $QEMU_VER"
    echo "   ✅ QEMU available!"
elif [ -x "$HOME/.espressif/tools/esp_qemu"*/esp32s3/bin/qemu-system-xtensa ]; then
    QEMU_BIN=$(ls $HOME/.espressif/tools/esp_qemu*/esp32s3/bin/qemu-system-xtensa | head -1)
    QEMU_VER=$($QEMU_BIN --version 2>&1 | head -1)
    echo "   $QEMU_VER (from esp-qemu)"
    echo "   ✅ QEMU available via esp-qemu!"
else
    echo "   ❌ QEMU not found. Install via: cd ~/esp/esp-idf && . ./export.sh && python -m idf_tools.py install esp-qemu"
fi
echo ""

# Test 5: SVG Assets
echo "🎨 Checking SVG Assets..."
if [ -f "simulator/assets/dashboard.svg" ]; then
    SIZE=$(stat -f%z "simulator/assets/dashboard.svg" 2>/dev/null || stat -c%s "simulator/assets/dashboard.svg")
    echo "   dashboard.svg: $(numfmt --to=iec-i --suffix=B $SIZE 2>/dev/null || echo $SIZE bytes)"
    echo "   ✅ SVG assets present!"
else
    echo "   ⚠️  No SVG assets found in simulator/assets/"
fi
echo ""

# Summary
echo "========================================="
echo "  Test Summary"
echo "========================================="
echo ""
echo "Next steps:"
echo "  1. Design your SVG dashboard"
echo "  2. Test in SDL2 simulator"
echo "  3. Build ESP32 firmware"
echo "  4. Test in QEMU"
echo "  5. Flash to hardware"
echo ""
echo "See COPILOT_CONTEXT.md for detailed instructions!"
echo ""
