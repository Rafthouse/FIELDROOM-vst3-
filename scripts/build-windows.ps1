# FIELDROOM Windows Native VST3 Build Script (PowerShell)
# Requires Visual Studio 2022 with C++ Tools, CMake 3.22+, and Git

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Building FIELDROOM VST3 for Windows x86-64" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

$BuildDir = "build"

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Write-Host "--> Configuring CMake project..." -ForegroundColor Yellow
cmake -B $BuildDir -A x64 -DCMAKE_BUILD_TYPE=Release -DFIELDROOM_COPY_PLUGIN_AFTER_BUILD=OFF

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed."
    exit 1
}

Write-Host "--> Compiling FIELDROOM VST3 (Release)..." -ForegroundColor Yellow
cmake --build $BuildDir --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build compilation failed."
    exit 1
}

Write-Host "--> Executing DSP Verification Unit Tests..." -ForegroundColor Yellow
ctest --test-dir $BuildDir -C Release --output-on-failure

Write-Host "==========================================" -ForegroundColor Green
Write-Host "BUILD SUCCESSFUL!" -ForegroundColor Green
Write-Host "VST3 Output: $BuildDir/FIELDROOM_artefacts/Release/VST3/FIELDROOM.vst3" -ForegroundColor Green
Write-Host "Run scripts\install-windows.bat as Administrator to install to C:\Program Files\Common Files\VST3" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green
