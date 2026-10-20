# Building FIELDROOM on Windows 10/11 (x86-64)

## Prerequisites
- Visual Studio 2022 (Community, Professional, or Enterprise) with "Desktop development with C++" workload
- CMake 3.22 or higher
- Git

## Step-by-Step Build Command

Open PowerShell or Developer Command Prompt for VS 2022:

```powershell
# 1. Clone or navigate to the repository
cd FIELDROOM

# 2. Configure CMake with Visual Studio 17 2022 generator
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

# 3. Compile the VST3 plugin and DSP test suite
cmake --build build --config Release --parallel

# 4. Run automated DSP unit tests
ctest --test-dir build -C Release --output-on-failure
```

## VST3 Artifact Location
Upon successful compilation, the VST3 bundle is located at:
`build/FIELDROOM_artefacts/Release/VST3/FIELDROOM.vst3`

The build automatically installs the `.vst3` folder to the default 64-bit Windows VST3 directory:
`C:\Program Files\Common Files\VST3\FIELDROOM.vst3`
