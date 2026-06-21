@echo off
REM ============================================================================
REM  AI Synth - One-click installer for Windows
REM
REM  Just double-click this file. It will:
REM    1. Launch the PowerShell bootstrap script
REM    2. Bootstrap detects/installs a lightweight C++ compiler
REM    3. Builds the VST3 plugin
REM    4. Installs it to the VST3 folder
REM    5. (Optional) Builds the .exe installer if Inno Setup is present
REM
REM  No need to install Visual Studio 2022 (31 GB). The bootstrap will offer
REM  to install either:
REM    - Visual Studio Build Tools (~3 GB, official) OR
REM    - LLVM-MinGW (~200 MB, lightweight)
REM ============================================================================

setlocal

cd /d "%~dp0"

echo.
echo  Launching AI Synth bootstrap installer...
echo.

REM Allow script execution for this session only
powershell -ExecutionPolicy Bypass -NoProfile -File "%~dp0bootstrap.ps1"

if errorlevel 1 (
    echo.
    echo  [ERROR] Bootstrap failed. See messages above.
    pause
    exit /b 1
)

echo.
pause
endlocal
