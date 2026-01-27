#!/bin/bash
# Build digi-dash firmware with ESP-IDF

set -e

# Check if ESP-IDF is set up
if [ -z "$IDF_PATH" ]; then
    echo "ERROR: IDF_PATH not set"
    echo "Run: source ~/esp-idf-5.5/export.sh"
    exit 1
fi

# Build mode
BUILD_MODE=${1:-build}

case "$BUILD_MODE" in
    build)
        echo "Building firmware..."
        idf.py build
        echo "Build complete. Firmware at: build/digi-dash.elf"
        ;;
    qemu)
        echo "Running in QEMU..."
        idf.py qemu monitor
        ;;
    clean)
        echo "Cleaning build..."
        idf.py fullclean
        ;;
    flash)
        PORT=${2:-/dev/ttyUSB0}
        echo "Flashing to $PORT..."
        idf.py -p "$PORT" flash monitor
        ;;
    *)
        echo "Usage: $0 [build|qemu|clean|flash]"
        echo ""
        echo "  build    - Build firmware (default)"
        echo "  qemu     - Run in QEMU simulator"
        echo "  clean    - Clean build directory"
        echo "  flash    - Flash to hardware"
        exit 1
        ;;
esac
