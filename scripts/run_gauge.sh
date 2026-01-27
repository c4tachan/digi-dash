#!/bin/bash
# Process SVG to gauge and display in simulator

set -e

PROJECT_DIR="/home/catachan/projects/digi-dash"
SVG_FILE="${1:-assets/dashboard_tiny.svg}"
GAUGE_FILE="${SVG_FILE%.*}.gauge"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║         Digi-Dash: SVG → Gauge → Display                  ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Check if SVG exists
if [ ! -f "$PROJECT_DIR/$SVG_FILE" ]; then
    echo "❌ SVG file not found: $SVG_FILE"
    echo "   Please place your SVG in the assets/ directory"
    exit 1
fi

echo "📄 SVG file: $SVG_FILE"
echo ""

# Step 1: Build (if needed)
if [ ! -f "$PROJECT_DIR/build/simulator/digi-dash-simulator" ]; then
    echo "🔨 Building simulator..."
    cd "$PROJECT_DIR"
    bash scripts/build_sim.sh > /dev/null 2>&1
    echo "   ✅ Build complete"
fi

# Step 2: Convert SVG to gauge
echo "🎨 Converting SVG to gauge format..."
cd "$PROJECT_DIR"
python3 tools/svg_preprocessor/svg_parser.py "$SVG_FILE" "$GAUGE_FILE"
echo "   ✅ Generated: $GAUGE_FILE"
echo ""

# Step 3: Run simulator
echo "🚀 Launching simulator..."
echo "   Press Q, ESC, or close window to exit"
echo ""
"$PROJECT_DIR/build/simulator/digi-dash-simulator" "$GAUGE_FILE"
