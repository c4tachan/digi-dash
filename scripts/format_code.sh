#!/bin/bash
# Format C++ code with clang-format

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Formatting engine code..."
find "$PROJECT_DIR/engine" -name "*.h" -o -name "*.cpp" | xargs clang-format -i

echo "Formatting simulator code..."
find "$PROJECT_DIR/simulator" -name "*.h" -o -name "*.cpp" | xargs clang-format -i

echo "Formatting firmware code..."
find "$PROJECT_DIR/firmware" -name "*.h" -o -name "*.cpp" | xargs clang-format -i

echo "Done!"
