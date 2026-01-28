#!/bin/bash
# Test script to verify pixel rendering with detailed sampling

cd /home/catachan/projects/digi-dash

echo "Testing pixel values at various locations in the rendered tile..."
echo ""
echo "Circle geometry: center=(360,360), radius=360"
echo "First tile: y=0 to y=59"
echo ""
echo "Expected filled pixels:"
echo "  - (360, 0): top of circle edge"
echo "  - (360, 30): well inside circle"  
echo "  - (216, 30): left edge at y=30"
echo "  - (504, 30): right edge at y=30"
echo ""
echo "Expected empty pixels:"
echo "  - (0, 0): far outside circle"
echo "  - (100, 0): outside circle at y=0"
echo ""

# Run simulator with output
./build/simulator/digi-dash-simulator 2>&1 | head -10
