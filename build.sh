#!/bin/zsh

# Exit on errors, undefined vars, and failed pipes
set -euo pipefail

BUILD_DIR="build"
SPV_DIR="src/spvs"

echo "Checking dependencies..."

for cmd in cmake glslc; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "Error: '$cmd' is not installed."
        exit 1
    fi
done

echo "Creating directories..."
mkdir -p "$SPV_DIR"

echo "Compiling shaders..."
glslc src/shaders/shader.vert -o "$SPV_DIR/vert.spv"
glslc src/shaders/shader.frag -o "$SPV_DIR/frag.spv"
glslc src/shaders/pressureSolve.comp -o "$SPV_DIR/pressureSolve.spv"
glslc src/shaders/advectVelocities.comp -o "$SPV_DIR/advectVelocities.spv"
glslc src/shaders/advectSmoke.comp -o "$SPV_DIR/advectSmoke.spv"
glslc src/shaders/updateVelocities.comp -o "$SPV_DIR/updateVelocities.spv"

echo "Configuring CMake..."
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build "$BUILD_DIR" --parallel

echo "Running..."
EXECUTABLE="$BUILD_DIR/vk_app"

if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: executable not found at $EXECUTABLE"
    exit 1
fi

"$EXECUTABLE"