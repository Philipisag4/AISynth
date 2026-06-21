# AI Synth

A professional-grade **AI text-to-preset synthesizer** plugin for FL Studio (VST3)
and other DAWs. Type a natural-language prompt like *"dark trap bass"* or
*"emotional cinematic pad"* and the on-device AI engine maps it onto a full
synth preset in real time — no network, no latency, no DAW restart.

![AI Synth](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-blue)
![Format](https://img.shields.io/badge/format-VST3%20%7C%20Standalone-orange)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Features

### AI Text-to-Preset Engine
- **On-device NLP** parses genre + mood + instrument-type from any text prompt.
- **5 genre prototypes**: Trap, EDM, Lo-Fi, Cinematic, Techno
- **5 mood modifiers**: Dark, Happy, Emotional, Aggressive, Eerie
- **5 instrument archetypes**: Bass, Lead, Pad, Pluck, Stab
- **Style Strength slider** blends AI inference with manual control (0 = full manual, 1 = full AI).

### Audio Engine
- Built on the **JUCE framework** (C++17).
- **Hybrid subtractive + wavetable** oscillator system with:
  - 6 morphable waveforms (Sine / Tri / Saw / Square / Noise / Wavetable)
  - 1–7 unison voices with detune
  - Pulse-width modulation, sub oscillator (−2 octaves)
- 4-pole resonant **ladder filter** (LPF / HPF / BPF / Notch) with drive.
- Two independent **ADSR envelopes** (amp + filter) with depth.
- Full **FX chain**: Distortion → Chorus → Delay (with feedback) → Reverb.
- **16-voice polyphony** with voice stealing.

### UI
- Modern **dark DAW-style** interface (1180 × 720, resizable).
- Central **text input box** with Generate button.
- Real-time **knob controls** for every parameter (VST3-automatable).
- **Spectrum analyzer** (FFT) + **waveform display**.
- **Preset browser** with factory + user presets.

### Workflow
- **Undo / Redo** (32-step history).
- **Random preset** generator.
- **Genre preset packs** built in (Trap / EDM / Cinematic / Lo-Fi / Techno).
- **Save / Load** user presets as JSON in `%APPDATA%\AI Synth\Presets\`.

---

## Project Structure

```
AISynth/
├── CMakeLists.txt              # JUCE-based CMake build
├── Source/
│   ├── PluginProcessor.{h,cpp}     # VST3 entry point + parameter store
│   ├── PluginEditor.{h,cpp}        # UI / dark theme
│   ├── SynthEngine.{h,cpp}         # Polyphonic synth + parameter store
│   ├── Voice.{h,cpp}               # Single polyphonic voice
│   ├── Oscillator.{h,cpp}          # Hybrid wavetable oscillator
│   ├── Wavetable.{h,cpp}           # 4-table morphable wavetable
│   ├── Filter.{h,cpp}              # Resonant ladder filter
│   ├── Envelope.{h,cpp}            # ADSR envelope
│   ├── FXChain.{h,cpp}             # Distortion + Chorus + Delay + Reverb
│   ├── TextToPreset.{h,cpp}        # AI text → preset inference
│   ├── PresetManager.{h,cpp}       # Save/load/random/undo/redo
│   ├── SpectrumAnalyzer.{h,cpp}    # FFT spectrum for UI
│   └── Components/
│       ├── KnobComponent.{h,cpp}
│       ├── SpectrumDisplay.{h,cpp}
│       ├── WaveformDisplay.{h,cpp}
│       ├── PresetBrowser.{h,cpp}
│       └── TextInputBox.{h,cpp}
├── Installer/
│   └── installer.iss           # Inno Setup Windows .exe installer
├── Presets/                    # Genre preset packs (JSON)
│   ├── trap.json
│   ├── edm.json
│   └── cinematic.json
├── Assets/
│   └── icon.ico                # Application icon (placeholder)
├── README.md                   # this file
└── BUILD.md                    # Build instructions (Windows / macOS / Linux)
```

---

## Quick Start (User)

1. Run **`AI Synth Setup.exe`**.
2. The installer copies the VST3 to `%COMMONPROGRAMFILES%\VST3\` and the standalone app to `%PROGRAMFILES%\AI Synth Labs\AI Synth\`.
3. Open FL Studio → **Options → File → Manage plugins → Rescan**.
4. Add **AI Synth** to a mixer insert or channel rack.
5. Type a prompt like `"aggressive cyberpunk lead"` → press **Enter** (or click **GENERATE**).
6. Play MIDI notes via the piano roll or your controller.

---

## Example Prompts

| Prompt                                | Result                              |
| ------------------------------------- | ----------------------------------- |
| `dark trap bass`                      | Low-passed saw + sub, 808-style     |
| `emotional cinematic pad`             | Slow-attack wavetable + long reverb |
| `aggressive cyberpunk lead`           | High-resonance square + distortion  |
| `lofi chill keys`                     | Triangle + chorus + tape delay      |
| `techno stab`                         | Short env, punchy filter            |
| `eerie ethereal atmospheric pad`      | Detuned wavetable, long tails       |

Try combinations like `"happy bright pluck arp"` or `"industrial techno sub bass"`.

---

## Compatibility

- **DAW**: FL Studio 20+, Ableton Live 10+, Cubase 10+, Studio One 4+, Reaper 6+ (any VST3 host)
- **OS**: Windows 10/11 (64-bit), macOS 10.13+, Linux (tested on Ubuntu 20.04+)
- **Format**: VST3 (32-voice polyphonic synthesizer)

---

## License

MIT License — see `LICENSE` for details.
