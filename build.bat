@echo off
REM ============================================================================
REM  AI Synth – one-shot build script for Windows
REM  Usage: build.bat [Release|Debug]
REM ============================================================================
setlocal
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Release

echo.
echo === Configuring CMake (Visual Studio 17 2022 -A x64) ===
cmake -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo.
    echo [ERROR] CMake configuration failed. Make sure Visual Studio 2022 with
    echo         the C++ workload is installed.
    exit /b 1
)

echo.
echo === Building VST3 plugin (%CONFIG%) ===
cmake --build build --config %CONFIG% --target AISynth_VST3
if errorlevel 1 (
    echo.
    echo [ERROR] VST3 build failed.
    exit /b 1
)

echo.
echo === Building Standalone app (%CONFIG%) ===
cmake --build build --config %CONFIG% --target AISynth_Standalone
if errorlevel 1 (
    echo.
    echo [ERROR] Standalone build failed.
    exit /b 1
)

echo.
echo === Build succeeded ===
echo.
echo VST3 plugin : build\AISynth_artefacts\%CONFIG%\VST3\AI Synth.vst3
echo Standalone  : build\AISynth_artefacts\%CONFIG%\Standalone\AI Synth.exe
echo.
echo Next steps:
echo   1. Copy the .vst3 bundle to %COMMONPROGRAMFILES%\VST3\
echo   2. (or) Open Installer\installer.iss in Inno Setup to build the installer
echo.
endlocal
