#!/bin/bash
# CTR Native Build Script (macOS, Apple Silicon)
# Requires Xcode Command Line Tools and CMake (e.g. via Homebrew: brew install cmake)

set -e

if ! command -v clang &>/dev/null; then
    echo "ERROR: clang not found"
    echo "Install Xcode Command Line Tools: xcode-select --install"
    exit 1
fi

if ! command -v cmake &>/dev/null; then
    echo "ERROR: cmake not found"
    echo "Install: brew install cmake"
    exit 1
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
cmake --build build -j

echo ""
echo "Build succeeded: build/ctr_native"
