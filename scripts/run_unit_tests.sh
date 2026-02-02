#!/bin/bash
set -e
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_BUILD_DIR="$PROJECT_DIR/simulator/build_test"
mkdir -p "$TEST_BUILD_DIR"
cd "$TEST_BUILD_DIR"
cmake ../test -DCMAKE_BUILD_TYPE=Debug
cmake --build . --target unit_tests -j
./unit_tests
