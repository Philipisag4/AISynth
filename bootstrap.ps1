# =============================================================================
#  AI Synth – Bootstrap Installer (PowerShell)
# =============================================================================
#  One-command install for Windows. Run from an elevated PowerShell:
#
#      ./bootstrap.ps1
#
#  What it does:
#    1. Checks for an existing C++ compiler (MSVC Build Tools OR MinGW)
#    2. If neither is found, asks which to install:
#         a) Visual Studio Build Tools (~3 GB, official Microsoft)
#         b) LLVM-MinGW (~200 MB, lightweight GCC/Clang)
#    3. Configures CMake (auto-fetches JUCE)
#    4. Builds the VST3 plugin + Standalone app
#    5. Installs the VST3 to the Windows VST3 folder
#    6. (Optional) Builds the .exe installer if Inno Setup is installed
#
#  No admin rights needed for the build itself, but installing the VST3 to
#  %COMMONPROGRAMFILES%\VST3\ requires elevation.
# =============================================================================

# Require PowerShell 5.1+
#Requires -Version 5.1

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# ---------- Helper functions ----------
function Write-Step  { param([string]$msg) Write-Host "`n=== $msg ===" -ForegroundColor Cyan }
function Write-OK    { param([string]$msg) Write-Host "  [OK] $msg" -ForegroundColor Green }
function Write-Warn  { param([string]$msg) Write-Host "  [!]  $msg" -ForegroundColor Yellow }
function Write-Err   { param([string]$msg) Write-Host "  [X]  $msg" -ForegroundColor Red }

function Test-Command {
    param([string]$cmd)
    return [bool](Get-Command $cmd -ErrorAction SilentlyContinue)
}

function Find-MSVC {
    # Look for vcvarsall.bat in standard VS install locations
    $paths = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\*\VC\Auxiliary\Build\vcvarsall.bat"
    )
    foreach ($p in $paths) {
        if (Test-Path $p) { return $p }
    }
    return $null
}

function Find-MinGW {
    foreach ($cand in @("gcc.exe", "clang.exe")) {
        $g = Get-Command $cand -ErrorAction SilentlyContinue
        if ($g) {
            $ver = & $g.Source --version 2>$null | Select-Object -First 1
            if ($ver -match "mingw|MinGW|LLVM") { return $g.Source }
        }
    }
    return $null
}

# ---------- 1. Banner ----------
Write-Host ""
Write-Host "  =======================================" -ForegroundColor Cyan
Write-Host "    AI Synth - Bootstrap Installer" -ForegroundColor Cyan
Write-Host "  =======================================" -ForegroundColor Cyan
Write-Host ""

# ---------- 2. Locate C++ compiler ----------
Write-Step "Checking for C++ compiler"

$msvcPath  = Find-MSVC
$mingwPath = Find-MinGW

if ($msvcPath) {
    Write-OK "Found Visual Studio C++ at: $msvcPath"
    $toolchain = "msvc"
} elseif ($mingwPath) {
    Write-OK "Found MinGW/Clang at: $mingwPath"
    $toolchain = "mingw"
} else {
    Write-Warn "No C++ compiler found."

    Write-Host ""
    Write-Host "  Choose an option to install:" -ForegroundColor Yellow
    Write-Host "    [1] Visual Studio 2022 Build Tools  (~3 GB download, official Microsoft)"
    Write-Host "    [2] LLVM-MinGW                       (~200 MB download, lightweight GCC+Clang)"
    Write-Host "    [3] Cancel"
    Write-Host ""
    $choice = Read-Host "Enter choice [1/2/3]"

    switch ($choice) {
        "1" {
            Write-Step "Installing Visual Studio 2022 Build Tools via winget"
            # This installs ONLY the C++ build tools, not the full IDE
            winget install --id Microsoft.VisualStudio.2022.BuildTools `
                           --override "--quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" `
                           --accept-package-agreements --accept-source-agreements
            if ($LASTEXITCODE -ne 0) {
                Write-Err "winget install failed. Try installing manually from:"
                Write-Host "    https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022"
                exit 1
            }
            # Refresh PATH and re-detect
            $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
            $msvcPath = Find-MSVC
            if (-not $msvcPath) {
                Write-Err "Build Tools installed but vcvarsall.bat not found. Please restart this script."
                exit 1
            }
            $toolchain = "msvc"
            Write-OK "MSVC Build Tools installed."
        }
        "2" {
            Write-Step "Installing LLVM-MinGW via winget"
            winget install --id MartinStorsjo.LLVM-MinGW.UCRT `
                           --accept-package-agreements --accept-source-agreements
            if ($LASTEXITCODE -ne 0) {
                Write-Err "winget install failed. Try downloading from:"
                Write-Host "    https://github.com/mstorsjo/llvm-mingw/releases"
                exit 1
            }
            $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
            $mingwPath = Find-MinGW
            if (-not $mingwPath) {
                Write-Warn "MinGW installed but not on PATH. Add it manually and re-run."
                exit 1
            }
            $toolchain = "mingw"
            Write-OK "LLVM-MinGW installed."
        }
        default {
            Write-Host "Cancelled."
            exit 0
        }
    }
}

# ---------- 3. Check for CMake ----------
Write-Step "Checking for CMake"
if (-not (Test-Command "cmake")) {
    Write-Warn "CMake not found. Installing via winget..."
    winget install --id Kitware.CMake --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) {
        Write-Err "CMake install failed. Install manually from https://cmake.org/download/"
        exit 1
    }
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
}
$cmakeVer = & cmake --version | Select-Object -First 1
Write-OK "CMake: $cmakeVer"

# ---------- 4. Configure CMake ----------
Write-Step "Configuring CMake"
$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $projectDir

if ($toolchain -eq "msvc") {
    & cmake -B build -G "Visual Studio 17 2022" -A x64
} else {
    # MinGW/Clang – use Ninja if available, otherwise MinGW Makefiles
    if (Test-Command "ninja") {
        & cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    } else {
        & cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    }
}
if ($LASTEXITCODE -ne 0) {
    Write-Err "CMake configuration failed."
    exit 1
}

# ---------- 5. Build VST3 ----------
Write-Step "Building VST3 plugin"
& cmake --build build --config Release --target AISynth_VST3 --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Err "VST3 build failed."
    exit 1
}
Write-OK "VST3 built."

# ---------- 6. Build Standalone ----------
Write-Step "Building Standalone app"
& cmake --build build --config Release --target AISynth_Standalone --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Warn "Standalone build failed (continuing anyway)."
} else {
    Write-OK "Standalone built."
}

# ---------- 7. Locate build artifacts ----------
$vst3Path = Get-ChildItem -Path "build" -Recurse -Filter "AI Synth.vst3" -Directory | Select-Object -First 1
$standalonePath = Get-ChildItem -Path "build" -Recurse -Filter "AI Synth.exe" | Select-Object -First 1

if (-not $vst3Path) {
    Write-Err "Could not find built VST3. Check build output above."
    exit 1
}

Write-OK "VST3 path: $($vst3Path.FullName)"

# ---------- 8. Ask whether to install VST3 ----------
Write-Step "Install VST3 to system folder?"
Write-Host "  This copies the plugin to: $env:COMMONPROGRAMFILES\VST3\"
Write-Host "  (FL Studio will detect it automatically on next scan.)"
$installChoice = Read-Host "Install now? [Y/n]"
if ($installChoice -eq "" -or $installChoice -match "^[yY]") {
    $dest = "$env:COMMONPROGRAMFILES\VST3"
    if (-not (Test-Path $dest)) { New-Item -ItemType Directory -Path $dest | Out-Null }

    # Need admin rights for %COMMONPROGRAMFILES%
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    if (-not $isAdmin) {
        Write-Warn "Admin rights needed. Re-launching elevated..."
        Start-Process -FilePath "powershell.exe" -Verb RunAs -ArgumentList "-NoExit", "-Command", "Copy-Item -Recurse -Force '$($vst3Path.FullName)' '$dest\'; Write-Host 'Installed!' -ForegroundColor Green; Pause"
    } else {
        Copy-Item -Recurse -Force $vst3Path.FullName $dest
        Write-OK "VST3 installed to $dest"
    }
} else {
    Write-Host "  Skipped. You can copy manually:" -ForegroundColor Yellow
    Write-Host "    '$($vst3Path.FullName)' -> '$env:COMMONPROGRAMFILES\VST3\'"
}

# ---------- 9. Optional: build installer ----------
Write-Step "Check for Inno Setup"
$innoPath = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
if (Test-Path $innoPath) {
    Write-OK "Inno Setup found. Building installer..."
    Push-Location "Installer"
    & $innoPath "installer.iss"
    Pop-Location
    $installerExe = "Installer\Output\AI Synth Setup.exe"
    if (Test-Path $installerExe) {
        Write-OK "Installer built: $installerExe"
        $openFolder = Read-Host "Open folder containing installer? [Y/n]"
        if ($openFolder -eq "" -or $openFolder -match "^[yY]") {
            Invoke-Item "Installer\Output"
        }
    }
} else {
    Write-Warn "Inno Setup not installed. Skipping installer build."
    Write-Host "  To build a one-click .exe installer, install Inno Setup:"
    Write-Host "    winget install JRSoftware.InnoSetup"
    Write-Host "  Then re-run this script."
}

# ---------- Done ----------
Write-Host ""
Write-Host "  =======================================" -ForegroundColor Green
Write-Host "    All done!" -ForegroundColor Green
Write-Host "  =======================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Next steps:"
Write-Host "    1. Open FL Studio"
Write-Host "    2. Options -> File -> Manage plugins -> Rescan"
Write-Host "    3. Add 'AI Synth' to a channel"
Write-Host "    4. Type 'dark trap bass' and click GENERATE"
Write-Host ""
