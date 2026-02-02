#!/bin/bash
# Build PC simulator

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"$PROJECT_DIR/scripts/build_sim.sh"
