#!/usr/bin/env bash
# FIELDROOM macOS Universal 2 (Apple Silicon arm64 + Intel x86_64) VST3 & AU Build Script
set -e

echo "=========================================================="
echo "Building FIELDROOM VST3/AU for macOS (Universal 2: arm64 + x86_64)"
echo "=========================================================="

BUILD_DIR="build"
mkdir -p "${BUILD_DIR}"

echo "--> Configuring CMake with Ninja generator..."
cmake -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
    -DFIELDROOM_COPY_PLUGIN_AFTER_BUILD=OFF

echo "--> Compiling FIELDROOM..."
cmake --build "${BUILD_DIR}" --config Release --parallel

echo "--> Ad-hoc signing Universal 2 bundles for macOS Gatekeeper / Apple Silicon..."
codesign --force --deep --sign - "${BUILD_DIR}/FIELDROOM_artefacts/Release/VST3/FIELDROOM.vst3"
codesign --force --deep --sign - "${BUILD_DIR}/FIELDROOM_artefacts/Release/AU/FIELDROOM.component"

echo "--> Running automated DSP unit tests..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "=========================================================="
echo "BUILD COMPLETE!"
echo "VST3: ${BUILD_DIR}/FIELDROOM_artefacts/Release/VST3/FIELDROOM.vst3"
echo "AU:   ${BUILD_DIR}/FIELDROOM_artefacts/Release/AU/FIELDROOM.component"
echo "=========================================================="
