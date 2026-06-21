# AI Synth

A professional-grade **AI text-to-preset synthesizer** plugin for FL Studio (VST3).
Type a natural-language prompt like *"dark trap bass"* or *"emotional cinematic pad"*
and the on-device AI engine maps it onto a full synth preset in real time —
no network, no latency, no DAW restart.

---

## ⚡ Quick Install (pick one)

| Path | What | Download | Best for |
|------|------|----------|----------|
| **1** | [Cloud build via GitHub Actions](.github/workflows/build.yml) — fork repo, push, download installer | **0 MB** | Easiest. No compiler needed. |
| **2** | Double-click [`install.bat`](install.bat) — auto-installs lightweight MinGW (~200 MB) and builds | ~250 MB | Quick local install |
| **3** | VS Code + CMake + MinGW (manual) — see [`QUICKSTART.md`](QUICKSTART.md) | ~250 MB | Developers wanting control |

➡️ **See [`QUICKSTART.md`](QUICKSTART.md) for full step-by-step instructions.**

> **No Visual Studio 2022 (31 GB) needed!**
> The bootstrap script offers **Visual Studio Build Tools** (~3 GB) or **LLVM-MinGW** (~200 MB) instead.

---

## Features

### AI Text-to-Preset Engine
- **On-device NLP** parses genre + mood + instrument-type from any text prompt
- **5 genres**: Trap, EDM, Lo-Fi, Cinematic, Techno
- **5 moods**: Dark, Happy, Emotional, Aggressive, Eerie
- **5 instruments**: Bass, Lead, Pad, Pluck, Stab
- **Style Strength slider** blends AI inference with manual control (0 = full manual, 1 = full AI)

### Audio Engine
- Built on **JUCE framework** (C++17)
- **Hybrid subtractive + wavetable** oscillator (6 waveforms, unison 1-7, sub osc, PWM)
- 4-mode resonant **ladder filter** with drive and dedicated filter envelope
- Two **ADSR envelopes** (amp + filter)
- Full **FX chain**: Distortion → Chorus → Delay → Reverb
- **16-voice polyphony** with voice stealing

### UI
- Modern **dark DAW-style** interface (1180×720, resizable)
- Central **text input box** + Generate button
- ~30 real-time **VST3-automatable knobs**
- **FFT spectrum analyzer** + **waveform display**
- **Preset browser** with factory + user presets

### Workflow
- **Undo / Redo** (32-step history)
- **Random preset** generator
- **Genre preset packs** (Trap / EDM / Cinematic)
- **Save / Load** user presets as JSON

---

## Example Prompts

| Prompt | Result |
|--------|--------|
| `dark trap bass` | Low-passed saw + sub, 808-style |
| `emotional cinematic pad` | Slow-attack wavetable + long reverb |
| `aggressive cyberpunk lead` | High-resonance square + distortion |
| `lofi chill keys` | Triangle + chorus + tape delay |
| `techno stab` | Short env, punchy filter |
| `eerie ethereal atmospheric pad` | Detuned wavetable, long tails |

Try combinations like `"happy bright pluck arp"` or `"industrial techno sub bass"`.

---

## Project Structure

```
AISynth/
├── .github/workflows/build.yml    # Cloud build (zero-install path)
├── Source/
│   ├── PluginProcessor.{h,cpp}    # VST3 entry point + parameter store
│   ├── PluginEditor.{h,cpp}       # UI / dark theme
│   ├── SynthEngine.{h,cpp}        # Polyphonic synth + parameter store
│   ├── Voice.{h,cpp}              # Single polyphonic voice
│   ├── Oscillator.{h,cpp}         # Hybrid wavetable oscillator
│   ├── Wavetable.{h,cpp}          # 4-table morphable wavetable
│   ├── Filter.{h,cpp}             # Resonant ladder filter
│   ├── Envelope.{h,cpp}           # ADSR envelope
│   ├── FXChain.{h,cpp}            # Distortion + Chorus + Delay + Reverb
│   ├── TextToPreset.{h,cpp}       # AI text → preset inference
│   ├── PresetManager.{h,cpp}      # Save/load/random/undo/redo
│   ├── SpectrumAnalyzer.{h,cpp}   # FFT spectrum for UI
│   └── Components/                # Knob, spectrum, waveform, browser, text input
├── Installer/installer.iss        # Inno Setup Windows installer script
├── Presets/                       # Genre preset packs (JSON)
├── install.bat                    # One-click installer launcher
├── bootstrap.ps1                  # PowerShell bootstrap (auto-installs compiler)
├── CMakeLists.txt                 # CMake build (auto-fetches JUCE 7.0.12)
├── QUICKSTART.md                  # 3-path install guide (READ THIS FIRST)
├── BUILD.md                       # Detailed build instructions
└── README.md                      # This file
```

---

## Compatibility

- **DAW**: FL Studio 20+, Ableton Live 10+, Cubase 10+, Studio One 4+, Reaper 6+ (any VST3 host)
- **OS**: Windows 10/11 (64-bit), macOS 10.13+, Linux (Ubuntu 20.04+)
- **Format**: VST3 (16-voice polyphonic synthesizer)

---

## License

MIT License — see [`LICENSE`](LICENSE).
