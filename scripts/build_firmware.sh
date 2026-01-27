#!/bin/bash
# Build and run digi-dash firmware in QEMU

set -e

# Check if ESP-IDF is set up
if [ -z "$IDF_PATH" ]; then
    echo "ESP-IDF not found. Please run:"
    echo "  source \$HOME/esp-idf-5.5.2/export.sh"
    exit 1
fi

# Clean previous build if requested
if [ "$1" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# Create build directory
mkdir -p build

# Build the firmware
echo "Building firmware..."
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-esp32s3.cmake \
       -DCMAKE_BUILD_TYPE=Release \
       -DIDF_TARGET=esp32s3 \
       ..
make -j4
cd ..

echo "Build complete!"
echo ""
echo "To run in QEMU:"
echo "  idf.py qemu monitor"
echo ""
echo "To flash to hardware:"
echo "  idf.py -p /dev/ttyUSB0 flash monitor"
