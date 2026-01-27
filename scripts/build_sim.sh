#!/bin/bash
# Build and run the Linux simulator

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/simulator/build"

echo "Cleaning previous build..."
rm -rf "$BUILD_DIR"

echo "Creating build directory..."
mkdir -p "$BUILD_DIR"

echo "Running CMake..."
cd "$BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE=Debug ..

echo "Building..."
make -j$(nproc)

echo "Build complete!"
echo "Run the simulator with: $BUILD_DIR/digi-dash-simulator"
