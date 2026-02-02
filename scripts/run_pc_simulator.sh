#!/bin/bash
# Run PC simulator

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${PROJECT_DIR}/simulator/build/digi-dash-simulator"

if [ ! -x "$BIN" ]; then
    "$PROJECT_DIR/scripts/build_pc_simulator.sh"
fi

"$BIN"
