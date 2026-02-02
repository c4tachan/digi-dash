#!/bin/bash
# Run svg_preprocessor tool and copy output to simulator assets

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SVG_FILE="${1:-assets/dashboard_tiny.svg}"
GAUGE_FILE="${2:-${SVG_FILE%.*}.gauge}"
BIN="${PROJECT_DIR}/tools/svg_preprocessor/build/svg_preprocessor"

if [ ! -x "$BIN" ]; then
    "$PROJECT_DIR/scripts/build_preprocessor.sh"
fi

cd "$PROJECT_DIR"
"$BIN" "$SVG_FILE" "$GAUGE_FILE"

echo "✅ Generated: $GAUGE_FILE"

# Ensure gauge is available to simulator and top-level assets
echo "📁 Copying gauge to asset locations..."
mkdir -p "$PROJECT_DIR/assets"
cp -f "$GAUGE_FILE" "$PROJECT_DIR/assets/$(basename "$GAUGE_FILE")"

# Also place into simulator assets directory (source) if exists
if [ -d "$PROJECT_DIR/simulator/assets" ]; then
    cp -f "$GAUGE_FILE" "$PROJECT_DIR/simulator/assets/$(basename "$GAUGE_FILE")"
fi

# If simulator has already been built, update build/assets so runtime picks up new file
if [ -d "$PROJECT_DIR/simulator/build/assets" ]; then
    cp -f "$GAUGE_FILE" "$PROJECT_DIR/simulator/build/assets/$(basename "$GAUGE_FILE")"
fi

echo "   ✅ Copied to:"
echo "      - $PROJECT_DIR/assets/$(basename "$GAUGE_FILE")"
if [ -d "$PROJECT_DIR/simulator/assets" ]; then
    echo "      - $PROJECT_DIR/simulator/assets/$(basename "$GAUGE_FILE")"
fi
if [ -d "$PROJECT_DIR/simulator/build/assets" ]; then
    echo "      - $PROJECT_DIR/simulator/build/assets/$(basename "$GAUGE_FILE")"
fi

echo ""
echo "Tip: if you run the simulator after an existing build, the updated file will be used from simulator/build/assets." 
