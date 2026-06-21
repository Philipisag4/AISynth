#pragma once

#include <JuceHeader.h>
#include "SynthEngine.h"
#include "TextToPreset.h"
#include "PresetManager.h"
#include "SpectrumAnalyzer.h"

namespace aisynth {

// =====================================================================
// PluginProcessor – the VST3 entry point. Owns:
//   * AudioProcessorValueTreeState (parameter store / DAW automation)
//   * SynthEngine (polyphonic synth + FX)
//   * TextToPreset (AI inference)
//   * PresetManager (factory + user presets, undo/redo)
//   * SpectrumAnalyzer (for the UI)
// =====================================================================
class PluginProcessor : public juce::AudioProcessor {
public:
    PluginProcessor();
    ~PluginProcessor() override = default;

    // ---- AudioProcessor overrides ----
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AI Synth"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "AI Synth"; }
    void changeProgramName(int, const juce::String &) override {}

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    // ---- Public accessors for the editor ----
    juce::AudioProcessorValueTreeState &getAPVTS() { return m_apvts; }
    SynthEngine &getSynth() { return m_synth; }
    TextToPreset &getAI() { return m_ai; }
    PresetManager &getPresets() { return m_presets; }
    SpectrumAnalyzer &getSpectrum() { return m_spectrum; }

    // Returns the most recent audio sample (lock-free read for UI)
    float getLatestSample() const { return m_latestSample.load(); }

    // ---- AI generation (called from editor) ----
    void generateFromPrompt(const juce::String &prompt);

    // ---- Parameter helpers ----
    void pushParamsToAPVTS(const SynthEngine::Params &p);
    void pullParamsFromAPVTS();
    void applyCurrentParams();

    // ---- For UI to know when AI last generated ----
    juce::String getLastPrompt() const { return m_lastPrompt; }
    std::atomic<bool> aiGeneratedFlag{false};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState m_apvts;
    SynthEngine m_synth;
    TextToPreset m_ai;
    PresetManager m_presets;
    SpectrumAnalyzer m_spectrum;
    juce::String m_lastPrompt;
    std::atomic<float> m_latestSample{0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

} // namespace aisynth
