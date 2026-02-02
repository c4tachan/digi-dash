#!/bin/bash
# Build svg_preprocessor tool

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/tools/svg_preprocessor/build"
THORVG_ROOT_VALUE="${THORVG_ROOT:-$HOME/thorvg/install}"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DCMAKE_BUILD_TYPE=Release \
      -DTHORVG_ROOT="${THORVG_ROOT_VALUE}" \
      ..

make -j$(nproc)

echo "✅ svg_preprocessor built at: $BUILD_DIR/svg_preprocessor"
