#!/bin/bash
# WSL helper: Install build dependencies on Ubuntu/Debian

echo "Installing build dependencies..."

sudo apt-get update
sudo apt-get install -y \
    cmake \
    ninja-build \
    clang-format \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-ttf-dev \
    python3-dev \
    python3-pip \
    git \
    build-essential

echo "Installing Python tools for preprocessing..."
pip3 install pillow svgwrite

echo "Done! You may also need to install ESP-IDF separately for firmware builds."
