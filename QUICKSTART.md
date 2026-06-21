# AI Synth — Quick Start

Pick **one** of the three paths below. All three end with the same result:
the AI Synth VST3 installed in FL Studio.

---

## Path 1: Zero Install — Cloud Build (easiest, recommended)

No compiler, no download, no setup. GitHub builds it for you.

1. Create a free GitHub account (if you don't have one).
2. Go to your AI Synth repo on GitHub (or fork it).
3. Click the **Actions** tab → **Build AI Synth** → **Run workflow**.
4. Wait ~10 minutes for the cloud build to finish.
5. Click the completed run → scroll to **Artifacts** → download **AI-Synth-Installer**.
6. Unzip and run `AI Synth Setup.exe`.
7. Done. Open FL Studio, rescan plugins, done.

> To get a proper release with a downloadable installer link:
> Create a git tag like `v1.0.0` and push it. The workflow will auto-build and publish a GitHub Release with the installer attached.

---

## Path 2: One-Click Local Install (no Visual Studio 2022 needed)

Double-click `install.bat`. The PowerShell bootstrap script will:

1. Detect if you have a C++ compiler. If not, ask you to install one of:
   - **Visual Studio Build Tools** (~3 GB, official Microsoft — NOT the 31 GB full IDE)
   - **LLVM-MinGW** (~200 MB, lightweight — recommended for fast install)
2. Auto-install CMake if missing (you already have it).
3. Build the VST3 plugin + standalone app.
4. Install the VST3 to `%COMMONPROGRAMFILES%\VST3\`.
5. Optionally build the `.exe` installer if Inno Setup is installed.

### Step-by-step

```bat
:: 1. Unzip the project
:: 2. Double-click install.bat  (or run from CMD:)
cd path\to\AISynth
install.bat
```

When prompted, choose option **2 (LLVM-MinGW)** for the fastest install (~200 MB).
Total download: ~200 MB MinGW + CMake (already installed) + JUCE source (auto-fetched, ~50 MB).

---

## Path 3: Manual Build with VS Code + CMake (what you already have)

You already have VS Code + CMake — that's enough. You just need a C++ compiler.
The lightest option is LLVM-MinGW (~200 MB) or VS Build Tools (~3 GB).

### One-time setup (5 minutes)

```powershell
# Install MinGW (lightweight, ~200 MB) — preferred for VS Code users
winget install MartinStorsjo.LLVM-MinGW.UCRT

# Install Ninja (optional but recommended)
winget install Ninja-build.Ninja

# Install Inno Setup (only needed if you want to build the installer .exe)
winget install JRSoftware.InnoSetup
```

### Build (in VS Code)

1. Open the `AISynth/` folder in VS Code.
2. Install the **CMake Tools** extension (Microsoft).
3. `Ctrl+Shift+P` → **CMake: Configure**.
4. Select the **LLVM-MinGW** kit (or **Visual Studio Build Tools 2022 - amd64** if you installed that).
5. `Ctrl+Shift+P` → **CMake: Build All Targets**.
6. Outputs:
   - `build/AISynth_artefacts/Release/VST3/AI Synth.vst3`
   - `build/AISynth_artefacts/Release/Standalone/AI Synth.exe`

### Install the VST3

```powershell
# Copy the VST3 to the system VST3 folder (run as Admin)
copy-item -recurse "build\AISynth_artefacts\Release\VST3\AI Synth.vst3" "$env:COMMONPROGRAMFILES\VST3\"
```

### Build the installer (optional)

```powershell
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" Installer\installer.iss
# Output: Installer\Output\AI Synth Setup.exe
```

### Use in FL Studio

1. Open FL Studio
2. **Options → File → Manage plugins → Rescan**
3. Channel rack → **+** → **AI Synth**
4. Type `"dark trap bass"` → click **GENERATE**
5. Play notes via piano roll or MIDI controller

---

## Comparison Table

| Path | Download size | Time | Best for |
|------|--------------|------|----------|
| 1. Cloud build | 0 MB | 10 min | Non-developers, no local toolchain |
| 2. install.bat + MinGW | ~250 MB | 15 min | Quick local install, minimal download |
| 2. install.bat + VS Build Tools | ~3 GB | 25 min | Local install with official MSVC |
| 3. VS Code + MinGW (manual) | ~250 MB | 15 min | Developers who want full control |

---

## Troubleshooting

**"winget is not recognized"**
Winget comes with Windows 11 and modern Windows 10. Update via Microsoft Store (search "App Installer"). On older systems, install tools manually from their websites.

**"Build failed: JUCE fetch error"**
The CMake build downloads JUCE from GitHub. If your network blocks it, download JUCE manually and configure with:
```powershell
cmake -B build -DAISYNTH_USE_SYSTEM_JUCE=ON -DJUCE_PATH=C:\path\to\JUCE
```

**"Plugin not visible in FL Studio"**
1. Confirm the `.vst3` file is in `C:\Program Files\Common Files\VST3\`
2. FL Studio → Options → File → Manage plugins → Find more plugins → Rescan
3. Restart FL Studio

**"Antivirus flagged the build"**
False positive — common for unsigned VST3 builds. Add the build folder to your AV exclusions, or use Path 1 (cloud-built binaries are signed by GitHub Actions).

---

## Need help?

See `BUILD.md` for detailed build docs, or open an issue on the GitHub repo.
