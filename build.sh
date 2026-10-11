#!/bin/bash
# CTR Native Expanded Build Script (Linux)
# Requires gcc-multilib and X11/GL/ALSA dev packages

set -e

if ! command -v gcc &>/dev/null; then
    echo "ERROR: gcc not found"
    echo "Install: sudo apt install gcc-multilib"
    echo "         sudo apt install libx11-dev libxext-dev libgl1-mesa-dev libasound2-dev libudev-dev libdbus-1-dev"
    exit 1
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_C_FLAGS="-m32" -DCMAKE_EXE_LINKER_FLAGS="-m32"
cmake --build build -j"$(nproc)"

echo ""
echo "Build succeeded: build/ctr_native"
