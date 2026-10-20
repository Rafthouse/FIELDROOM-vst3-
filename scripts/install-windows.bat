@echo off
:: FIELDROOM 1-Click Windows VST3 Installer
echo ==========================================================
echo   FIELDROOM VST3 1-Click Installer for Windows
echo ==========================================================
echo.

set "DEST=%COMMONPROGRAMFILES%\VST3\FIELDROOM.vst3"
set "SRC=%~dp0..\build\FIELDROOM_artefacts\Release\VST3\FIELDROOM.vst3"

if not exist "%SRC%" (
    set "SRC=%~dp0FIELDROOM.vst3"
)

if not exist "%SRC%" (
    echo [ERROR] FIELDROOM.vst3 not found! Please build the plugin first.
    pause
    exit /b 1
)

echo --> Installing to: %DEST%
if not exist "%COMMONPROGRAMFILES%\VST3" mkdir "%COMMONPROGRAMFILES%\VST3"
xcopy /E /I /Y /Q "%SRC%" "%DEST%"

if %ERRORLEVEL% equ 0 (
    echo.
    echo ==========================================================
    echo   INSTALLATION SUCCESSFUL!
    echo   FIELDROOM.vst3 is ready to use in your DAW.
    echo ==========================================================
) else (
    echo.
    echo [ERROR] Installation failed. Try running this script as Administrator.
)

pause
