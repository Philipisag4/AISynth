#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Components/KnobComponent.h"
#include "Components/SpectrumDisplay.h"
#include "Components/WaveformDisplay.h"
#include "Components/PresetBrowser.h"
#include "Components/TextInputBox.h"

namespace aisynth {

// =====================================================================
// PluginEditor – the main UI
// Layout:
//   ┌──────────────────────────────────────────────────────────────┐
//   │ HEADER: title + AI text input + Generate + Style Strength    │
//   ├──────────────────────────────┬───────────────────────────────┤
//   │ OSC 1 + OSC 2 + Filter       │  Spectrum Analyzer            │
//   │ ADSR + Filter Env            │  Waveform Display             │
//   │ FX Chain (Dist/Chorus/Delay/ │  Preset Browser               │
//   │   Reverb)                    │                               │
//   │ Master + Glide               │                               │
//   │ Action buttons (Random/Undo/ │                               │
//   │   Redo/Save)                 │                               │
//   └──────────────────────────────┴───────────────────────────────┘
// =====================================================================
class PluginEditor : public juce::AudioProcessorEditor {
public:
    explicit PluginEditor(PluginProcessor &);
    ~PluginEditor() override = default;

    void paint(juce::Graphics &) override;
    void resized() override;

private:
    void generateFromPrompt(const juce::String &prompt);
    void loadPreset(const juce::String &name);
    void savePreset(const juce::String &name);
    void deletePreset(const juce::String &name);
    void randomize();
    void undo();
    void redo();

    PluginProcessor &m_processor;

    // Header
    TextInputBox m_textInput;
    KnobComponent m_styleKnob;
    juce::TextButton m_randomBtn{"Random"}, m_undoBtn{"Undo"}, m_redoBtn{"Redo"},
                     m_saveBtn{"Save Preset"};

    // Visualizers
    SpectrumDisplay m_spectrum;
    WaveformDisplay m_waveform;

    // Preset browser
    PresetBrowser m_presetBrowser;

    // Knobs – grouped by section
    // OSC1
    KnobComponent m_osc1Wave, m_osc1Unison, m_osc1Detune, m_osc1Sub, m_pulseWidth, m_wavetablePos;
    // OSC2
    KnobComponent m_osc2Wave, m_osc2Coarse, m_osc2Level;
    // Filter
    KnobComponent m_fltType, m_fltCutoff, m_fltReso, m_fltDrive, m_fltEnvAmt,
                  m_fltAttack, m_fltDecay, m_fltSustain, m_fltRelease;
    // Amp env
    KnobComponent m_ampAttack, m_ampDecay, m_ampSustain, m_ampRelease;
    // FX
    KnobComponent m_distDrive, m_distMix, m_chorusRate, m_chorusDepth, m_chorusMix,
                  m_delayTime, m_delayFeedback, m_delayMix,
                  m_reverbSize, m_reverbDamping, m_reverbMix;
    // Master
    KnobComponent m_masterGain, m_glide;

    // Lock-free sample reader for the waveform display
    juce::AbstractFifo m_waveFifo{2048};
    std::array<float, 2048> m_waveBuffer{};
    std::atomic<int> m_waveWritePos{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace aisynth
