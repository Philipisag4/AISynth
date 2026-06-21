#pragma once

#include <JuceHeader.h>

namespace aisynth {

// =====================================================================
// FXChain – Distortion → Chorus → Delay → Reverb
// All built on juce::dsp for high-quality, low-latency processing.
// =====================================================================
class FXChain {
public:
    void prepare(double sampleRate, int blockSize, int numChannels)
    {
        m_sampleRate = sampleRate;
        m_numChannels = numChannels;

        juce::dsp::ProcessSpec spec{ sampleRate,
                                     static_cast<juce::uint32>(blockSize),
                                     static_cast<juce::uint32>(numChannels) };

        m_chorus.prepare(spec);
        m_delay.prepare(spec);
        m_reverb.setSampleRate(sampleRate);

        // Sensible defaults
        m_chorus.setRate(0.5f);
        m_chorus.setDepth(0.3f);
        m_chorus.setCentreDelay(10.0f);
        m_chorus.setFeedback(0.2f);
        m_chorus.setMix(0.0f);

        m_delay.setMaximumDelayInSamples(static_cast<int>(sampleRate * 2.0));
        m_delay.setDelay(0.0);
        m_delayFeedback = 0.0f;
        m_delayMix = 0.0f;

        m_reverbParams = juce::Reverb::Parameters{};
        m_reverbParams.roomSize = 0.5f;
        m_reverbParams.damping  = 0.5f;
        m_reverbParams.wetLevel = 0.0f;
        m_reverbParams.dryLevel = 1.0f;
        m_reverbParams.width    = 1.0f;
        m_reverbParams.freezeMode = 0.0f;
        m_reverb.setParameters(m_reverbParams);
    }

    void reset()
    {
        m_chorus.reset();
        m_delay.reset();
        for (auto &b : m_delayBuffer) b = 0.0f;
        m_reverb.reset();
    }

    // --- Distortion ---
    void setDistortionDrive(float d) { m_drive = juce::jlimit(1.0f, 20.0f, d); }
    void setDistortionMix(float m)   { m_distortionMix = juce::jlimit(0.0f, 1.0f, m); }

    // --- Chorus ---
    void setChorusRate(float hz)    { m_chorus.setRate(juce::jlimit(0.0f, 10.0f, hz)); }
    void setChorusDepth(float d)    { m_chorus.setDepth(juce::jlimit(0.0f, 1.0f, d)); }
    void setChorusMix(float m)      { m_chorus.setMix(juce::jlimit(0.0f, 1.0f, m)); }

    // --- Delay ---
    void setDelayTime(float seconds)
    {
        m_delayTime = juce::jlimit(0.0f, 2.0f, seconds);
        m_delay.setDelay(m_sampleRate * m_delayTime);
    }
    void setDelayFeedback(float f) { m_delayFeedback = juce::jlimit(0.0f, 0.95f, f); }
    void setDelayMix(float m)      { m_delayMix = juce::jlimit(0.0f, 1.0f, m); }

    // --- Reverb ---
    void setReverbSize(float s)    { m_reverbParams.roomSize = juce::jlimit(0.0f, 1.0f, s); m_reverb.setParameters(m_reverbParams); }
    void setReverbDamping(float d) { m_reverbParams.damping  = juce::jlimit(0.0f, 1.0f, d); m_reverb.setParameters(m_reverbParams); }
    void setReverbMix(float m)
    {
        m_reverbParams.wetLevel = juce::jlimit(0.0f, 1.0f, m);
        m_reverbParams.dryLevel = 1.0f - m_reverbParams.wetLevel;
        m_reverb.setParameters(m_reverbParams);
    }

    void process(juce::AudioBuffer<float> &buffer)
    {
        if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;

        const int numSamples = buffer.getNumSamples();

        // ---- 1. Distortion (sample-wise) ----
        if (m_distortionMix > 0.001f)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto *data = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    const float driven = std::tanh(data[i] * m_drive) / std::tanh(m_drive);
                    data[i] = data[i] * (1.0f - m_distortionMix) + driven * m_distortionMix;
                }
            }
        }

        // ---- 2. Chorus ----
        if (m_chorus.getMix() > 0.001f)
        {
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> ctx(block);
            m_chorus.process(ctx);
        }

        // ---- 3. Delay (feedback, per-channel) ----
        if (m_delayMix > 0.001f)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto *data = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    const float in = data[i];
                    const float delayed = m_delay.popSample(ch);
                    const float out = in + delayed * m_delayMix;
                    m_delay.pushSample(ch, in + delayed * m_delayFeedback);
                    data[i] = out;
                }
            }
        }

        // ---- 4. Reverb ----
        if (m_reverbParams.wetLevel > 0.001f)
        {
            const int numCh = buffer.getNumChannels();
            if (numCh >= 2)
            {
                m_reverb.processStereo(buffer.getReadPointer(0),
                                       buffer.getReadPointer(1),
                                       buffer.getWritePointer(0),
                                       buffer.getWritePointer(1),
                                       numSamples);
            }
            else if (numCh == 1)
            {
                m_reverb.processMono(buffer.getWritePointer(0), numSamples);
            }
        }
    }

private:
    double m_sampleRate = 44100.0;
    int m_numChannels = 2;

    // Distortion (sample-wise tanh – not using a separate WaveShaper)
    float m_drive = 1.0f;
    float m_distortionMix = 0.0f;

    // Chorus
    juce::dsp::Chorus<float> m_chorus;

    // Delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> m_delay{
        static_cast<int>(192000)};
    float m_delayTime = 0.25f;
    float m_delayFeedback = 0.4f;
    float m_delayMix = 0.0f;
    std::array<float, 2> m_delayBuffer{0, 0};

    // Reverb
    juce::Reverb m_reverb;
    juce::Reverb::Parameters m_reverbParams;
};

} // namespace aisynth
