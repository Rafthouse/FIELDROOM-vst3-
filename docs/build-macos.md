# Building FIELDROOM on macOS (Universal 2: Apple Silicon arm64 + Intel x86_64)

## Prerequisites
- macOS 11.0 Big Sur or later
- Xcode 14+ or Xcode Command Line Tools (`xcode-select --install`)
- CMake 3.22+ (`brew install cmake`)
- Ninja build tool (`brew install ninja`)

## Step-by-Step Build Command

```bash
# 1. Navigate to the repository
cd FIELDROOM

# 2. Configure CMake for Universal 2 (both Apple Silicon arm64 & Intel x86_64)
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"

# 3. Build the VST3 and AU plugins
cmake --build build --config Release --parallel

# 4. Run automated DSP verification tests
ctest --test-dir build --output-on-failure
```

## VST3 & AU Artifact Locations
- VST3: `build/FIELDROOM_artefacts/Release/VST3/FIELDROOM.vst3`
- Audio Unit (AU): `build/FIELDROOM_artefacts/Release/AU/FIELDROOM.component`

Installed to:
- `~/Library/Audio/Plug-Ins/VST3/FIELDROOM.vst3`
- `~/Library/Audio/Plug-Ins/Components/FIELDROOM.component`

## Code Signing & Notarization Preparation
```bash
codesign --force --timestamp --options runtime --sign "Developer ID Application: Your Name (TEAM_ID)" build/FIELDROOM_artefacts/Release/VST3/FIELDROOM.vst3
```
