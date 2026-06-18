#!/bin/bash

# Sandbox Build Script
# Compiles standalone sandbox executable with no external dependencies

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
RESULTS_DIR="${SCRIPT_DIR}/results"

# Create directories
mkdir -p "${BUILD_DIR}"
mkdir -p "${RESULTS_DIR}"

echo "Building sandbox..."
cd "${BUILD_DIR}"

# Compile with C++11, no external deps (just stdlib)
g++ -std=c++11 -O2 -Wall \
    -o sandbox \
    "${SCRIPT_DIR}/sandbox.cpp"

echo "Build complete: ${BUILD_DIR}/sandbox"
echo ""
echo "Run with: cd ${BUILD_DIR} && ./sandbox"
