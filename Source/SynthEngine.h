#pragma once

#include <JuceHeader.h>
#include "Voice.h"
#include "FXChain.h"

namespace aisynth {

// =====================================================================
// SynthEngine – owns N voices, parameter store, and the master FX chain.
// The PluginProcessor drives this directly via processBlock().
// =====================================================================
class SynthEngine {
public:
    static constexpr int kMaxVoices = 16;

    // All parameters – kept in plain floats for fast access on the audio
    // thread. The PluginProcessor mirrors the APVTS values into here on
    // every parameter change.
    struct Params {
        // Oscillator 1
        int   osc1Wave = 2;          // saw
        float osc1Unison = 1.0f;
        float osc1Detune = 0.0f;
        float osc1Sub = 0.0f;
        float pulseWidth = 0.5f;
        float wavetablePos = 0.5f;

        // Oscillator 2
        int   osc2Wave = 3;          // square
        float osc2Coarse = 0.0f;
        float osc2Level = 0.5f;

        // Filter
        int   filterType = 0;
        float filterCutoff = 1000.0f;
        float filterReso = 0.7f;
        float filterDrive = 1.0f;
        float filterEnvAmt = 0.5f;

        // Filter envelope
        float filterAttack = 0.01f;
        float filterDecay = 0.2f;
        float filterSustain = 0.7f;
        float filterRelease = 0.3f;

        // Amp envelope
        float ampAttack = 0.01f;
        float ampDecay = 0.2f;
        float ampSustain = 0.7f;
        float ampRelease = 0.3f;

        // FX
        float distortionDrive = 1.0f;
        float distortionMix = 0.0f;
        float chorusRate = 0.5f;
        float chorusDepth = 0.3f;
        float chorusMix = 0.0f;
        float delayTime = 0.25f;
        float delayFeedback = 0.4f;
        float delayMix = 0.0f;
        float reverbSize = 0.5f;
        float reverbDamping = 0.5f;
        float reverbMix = 0.0f;

        // Master
        float masterGain = 0.8f;
        float glide = 0.0f;
        float styleStrength = 1.0f; // AI influence (0 = full manual, 1 = full AI)
    };

    SynthEngine()
    {
        for (int i = 0; i < kMaxVoices; ++i)
            m_voices.push_back(std::make_unique<Voice>());
    }

    void prepare(double sampleRate, int blockSize, int numChannels)
    {
        m_sampleRate = sampleRate;
        m_numChannels = numChannels;

        for (auto &v : m_voices)
            v->prepare(sampleRate, blockSize);

        m_fx.prepare(sampleRate, blockSize, numChannels);

        m_scratchBuffer.setSize(numChannels, blockSize);
        m_scratchBuffer.clear();

        m_stereoBuffer.setSize(numChannels, blockSize);
        m_stereoBuffer.clear();

        applyParams(m_params);
    }

    void reset()
    {
        for (auto &v : m_voices) v->reset();
        m_fx.reset();
    }

    void applyParams(const Params &p)
    {
        m_params = p;
        for (auto &v : m_voices)
        {
            v->setOsc1Wave(p.osc1Wave);
            v->setOsc1Unison(static_cast<int>(p.osc1Unison));
            v->setOsc1Detune(p.osc1Detune);
            v->setOsc1Sub(p.osc1Sub);
            v->setPulseWidth(p.pulseWidth);
            v->setWavetablePos(p.wavetablePos);
            v->setOsc2Wave(p.osc2Wave);
            v->setOsc2Coarse(p.osc2Coarse);
            v->setOsc2Level(p.osc2Level);

            v->setFilterType(p.filterType);
            v->setFilterCutoff(p.filterCutoff);
            v->setFilterResonance(p.filterReso);
            v->setFilterDrive(p.filterDrive);
            v->setFilterEnvAmt(p.filterEnvAmt);

            v->setFilterEnvAttack(p.filterAttack);
            v->setFilterEnvDecay(p.filterDecay);
            v->setFilterEnvSustain(p.filterSustain);
            v->setFilterEnvRelease(p.filterRelease);

            v->setAmpEnvAttack(p.ampAttack);
            v->setAmpEnvDecay(p.ampDecay);
            v->setAmpEnvSustain(p.ampSustain);
            v->setAmpEnvRelease(p.ampRelease);

            v->setGlide(p.glide);
            v->setGain(0.25f); // per-voice scaling
        }

        m_fx.setDistortionDrive(p.distortionDrive);
        m_fx.setDistortionMix(p.distortionMix);
        m_fx.setChorusRate(p.chorusRate);
        m_fx.setChorusDepth(p.chorusDepth);
        m_fx.setChorusMix(p.chorusMix);
        m_fx.setDelayTime(p.delayTime);
        m_fx.setDelayFeedback(p.delayFeedback);
        m_fx.setDelayMix(p.delayMix);
        m_fx.setReverbSize(p.reverbSize);
        m_fx.setReverbDamping(p.reverbDamping);
        m_fx.setReverbMix(p.reverbMix);
    }

    const Params &getParams() const { return m_params; }

    void noteOn(int midiNote, float velocity)
    {
        // Voice stealing: pick an inactive voice, otherwise the oldest
        Voice *target = nullptr;
        for (auto &v : m_voices)
        {
            if (!v->isActive()) { target = v.get(); break; }
        }
        if (!target) target = m_voices.front().get();
        target->reset();
        target->noteOn(midiNote, velocity);
    }

    void noteOff(int midiNote)
    {
        for (auto &v : m_voices)
        {
            if (v->isActive() && v->getNote() == midiNote)
                v->noteOff();
        }
    }

    void allNotesOff()
    {
        for (auto &v : m_voices) v->noteOff();
    }

    void process(juce::AudioBuffer<float> &outBuffer, juce::MidiBuffer &midiMessages)
    {
        const int numSamples = outBuffer.getNumSamples();
        m_stereoBuffer.setSize(outBuffer.getNumChannels(), numSamples, false);
        m_stereoBuffer.clear();

        // Handle MIDI events
        for (const auto meta : midiMessages)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                allNotesOff();
        }

        // Mix all voices into the stereo buffer (mono-summed for now)
        for (auto &v : m_voices)
        {
            if (!v->isActive()) continue;
            for (int i = 0; i < numSamples; ++i)
            {
                const float s = v->process();
                for (int ch = 0; ch < m_stereoBuffer.getNumChannels(); ++ch)
                    m_stereoBuffer.addSample(ch, i, s);
            }
        }

        // Apply FX chain
        m_fx.process(m_stereoBuffer);

        // Master gain → output
        const float gain = m_params.masterGain;
        for (int ch = 0; ch < outBuffer.getNumChannels(); ++ch)
        {
            auto *out = outBuffer.getWritePointer(ch);
            auto *in  = m_stereoBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                out[i] = in[i] * gain;
        }
    }

private:
    double m_sampleRate = 44100.0;
    int m_numChannels = 2;
    Params m_params;

    std::vector<std::unique_ptr<Voice>> m_voices;
    FXChain m_fx;
    juce::AudioBuffer<float> m_scratchBuffer;
    juce::AudioBuffer<float> m_stereoBuffer;
};

} // namespace aisynth
