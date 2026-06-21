# Build Instructions

This document describes how to build AI Synth from source on Windows, macOS, and Linux.

---

## Prerequisites (Windows – recommended path)

1. **Windows 10 or 11 (64-bit)**
2. **Visual Studio 2022** with the *Desktop development with C++* workload installed (includes MSVC + Windows SDK).
3. **CMake 3.22+** – install from <https://cmake.org/download/>.
4. **Git** – install from <https://git-scm.com/>.
5. **Inno Setup 6.x** – for the Windows `.exe` installer. Install from <https://jrsoftware.org/isdl.php>.

> JUCE itself is fetched automatically by CMake — no manual JUCE download needed.

---

## Step 1 – Clone and configure

```bat
git clone <your-repo-url> AISynth
cd AISynth
cmake -B build -G "Visual Studio 17 2022" -A x64
```

> If you want to use a locally-installed JUCE instead of FetchContent:
> ```bat
> cmake -B build -G "Visual Studio 17 2022" -A x64 ^
>   -DAISYNTH_USE_SYSTEM_JUCE=ON -DJUCE_PATH=C:/dev/JUCE
> ```

---

## Step 2 – Build the VST3 plugin and standalone app

```bat
cmake --build build --config Release --target AISynth_VST3
cmake --build build --config Release --target AISynth_Standalone
```

The outputs will be at:
- VST3 plugin: `build/AISynth_artefacts/Release/VST3/AI Synth.vst3`
- Standalone:  `build/AISynth_artefacts/Release/Standalone/AI Synth.exe`

---

## Step 3 – Install the VST3 plugin (manually, without installer)

Copy the VST3 bundle to the standard Windows VST3 folder:

```bat
xcopy "build\AISynth_artefacts\Release\VST3\AI Synth.vst3" ^
      "%COMMONPROGRAMFILES%\VST3\" /E /I /Y
```

Then rescan plugins in FL Studio:
- FL Studio → **Options → File → Manage plugins → Find more plugins → Rescan**.

---

## Step 4 – Build the Windows installer (.exe)

1. Open **Inno Setup Compiler**.
2. Open `Installer/installer.iss`.
3. Press **Ctrl+F9** (Compile).
4. The installer is written to `Installer/Output/AI Synth Setup.exe`.

> The installer copies the VST3 to `%COMMONPROGRAMFILES%\VST3\` and the standalone app to `%PROGRAMFILES%\AI Synth Labs\AI Synth\`, then creates Start Menu + optional Desktop shortcuts and an uninstall entry.

---

## Build on macOS

```bash
git clone <your-repo-url> AISynth
cd AISynth
cmake -B build -G "Xcode"
cmake --build build --config Release --target AISynth_VST3
cmake --build build --config Release --target AISynth_Standalone
```

The VST3 is written to `build/AISynth_artefacts/Release/VST3/AI Synth.vst3`.
Copy it to `~/Library/Audio/Plug-Ins/VST3/` (user) or `/Library/Audio/Plug-Ins/VST3/` (system).

> For AU format, add `AU` to the `FORMATS` list in `CMakeLists.txt`.

---

## Build on Linux

```bash
git clone <your-repo-url> AISynth
cd AISynth
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build --target AISynth_VST3
cmake --build build --target AISynth_Standalone
```

Install to `~/.vst3/`:

```bash
mkdir -p ~/.vst3
cp -r "build/AISynth_artefacts/Release/VST3/AI Synth.vst3" ~/.vst3/
```

> Linux builds require `libfreetype6-dev`, `libx11-dev`, `libxcomposite-dev`, `libxcursor-dev`, `libxext-dev`, `libxinerama-dev`, `libxrandr-dev`, `libxrender-dev`, `libwebkit2gtk-4.1-dev` (or `4.0`), `libasound2-dev`, and `libcurl4-openssl-dev`.

---

## Verifying FL Studio Compatibility

After installing the VST3:

1. Launch FL Studio.
2. Open the **Plugin Database** (left panel → Plugin Database → Installed → Generators).
3. Locate **AI Synth**.
4. Drag it onto the Channel Rack.
5. Type a prompt in the central text box and click **Generate**.
6. Test polyphony by drawing notes in the Piano Roll.
7. Verify MIDI input from your controller works (Plugin → **Wrap as MIDI input**).

---

## Troubleshooting

### "JUCE not found"
The CMake build fetches JUCE 7.0.12 from GitHub automatically. If the fetch fails (offline build), pass `-DAISYNTH_USE_SYSTEM_JUCE=ON -DJUCE_PATH=/path/to/JUCE`.

### "VST3 SDK missing"
JUCE ships the VST3 SDK, so this should not happen. If you see this error, update JUCE to 7.0.5+.

### "Plugin not visible in FL Studio"
- Confirm the `.vst3` file is in `%COMMONPROGRAMFILES%\VST3\` (use `echo %COMMONPROGRAMFILES%`).
- In FL Studio: **Options → File → Manage plugins → Rescan**.
- If FL Studio is 32-bit, you need a 32-bit build of the plugin. AI Synth ships 64-bit only by default.

### "Standalone app crashes on launch"
Run from a terminal to see the error:
```bat
"AI Synth.exe" --verbose
```
Most often the crash is a missing audio device — open Preferences and select your audio interface.

### "Installer fails with 'Access denied'"
Re-run the installer as Administrator (right-click → Run as administrator).

---

## Build Matrix (CI suggestions)

| OS      | Toolchain                  | Targets                  |
| ------- | -------------------------- | ------------------------ |
| Windows | VS 2022 + CMake 3.22       | VST3, Standalone         |
| macOS   | Xcode 14+ + CMake 3.22     | VST3, AU, Standalone     |
| Linux   | GCC 11+ / Clang 14+ + Ninja| VST3, Standalone         |

---

## Source Code Layout Reference

| Module             | Source File              | Purpose                                     |
| ------------------ | ------------------------ | ------------------------------------------- |
| Plugin entry       | `PluginProcessor.cpp`    | VST3 host interface, parameter store        |
| UI                 | `PluginEditor.cpp`       | Dark-theme UI, text input, knobs            |
| Synth engine       | `SynthEngine.h`          | Polyphonic voice manager + FX routing       |
| Voice              | `Voice.h`                | One note: 2 osc + filter + 2 env            |
| Oscillator         | `Oscillator.h`           | Hybrid wavetable/subtractive osc            |
| Wavetable          | `Wavetable.h`            | 4-table morphable wavetable                 |
| Filter             | `Filter.h`               | Resonant IIR ladder filter                  |
| Envelope           | `Envelope.h`             | ADSR with velocity                          |
| FX                 | `FXChain.h`              | Distortion + Chorus + Delay + Reverb        |
| AI engine          | `TextToPreset.h`         | On-device text → preset inference           |
| Preset manager     | `PresetManager.h`        | Save/load/random/undo/redo                  |
| Spectrum analyzer  | `SpectrumAnalyzer.h`     | FFT for the UI                              |
| UI components      | `Components/*.h`          | Knob, spectrum, waveform, preset browser    |
| Installer          | `Installer/installer.iss`| Inno Setup script for `.exe` installer      |
