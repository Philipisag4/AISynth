#pragma once

#include <JuceHeader.h>
#include "Wavetable.h"

namespace aisynth {

// =====================================================================
// Oscillator – hybrid subtractive + wavetable oscillator
// Supports 5 morphable waveforms (sine/tri/saw/square/noise) plus
// unison detune, pulse-width, and a secondary sub oscillator.
// =====================================================================
class Oscillator {
public:
    enum class Waveform { Sine = 0, Triangle, Saw, Square, Noise, Wavetable };

    Oscillator() = default;
    ~Oscillator() = default;

    void prepare(double sampleRate, int /*blockSize*/)
    {
        m_sampleRate = sampleRate;
        m_wavetable.prepare(sampleRate);
        reset();
    }

    void reset()
    {
        m_phase = 0.0;
        m_phaseInc = 0.0;
        m_subPhase = 0.0;
        for (auto &s : m_lastUnison)
            s = 0.0f;
    }

    void setFrequency(double freqHz)
    {
        jassert(freqHz > 0.0);
        m_frequency = juce::jlimit(20.0, 20000.0, freqHz);
        m_phaseInc = m_frequency / m_sampleRate;
    }

    void setWaveform(Waveform w) { m_waveform = w; }
    void setWaveformIndex(int idx)
    {
        if (idx >= 0 && idx < 6)
            m_waveform = static_cast<Waveform>(idx);
    }

    void setPulseWidth(float pw) { m_pulseWidth = juce::jlimit(0.05f, 0.95f, pw); }
    void setUnison(int count) { m_unison = juce::jlimit(1, 7, count); }
    void setDetune(float cents) { m_detune = juce::jlimit(0.0f, 100.0f, cents); }
    void setMix(float m) { m_mix = juce::jlimit(0.0f, 1.0f, m); }
    void setSubLevel(float s) { m_subLevel = juce::jlimit(0.0f, 1.0f, s); }
    void setSubOctave(int oct) { m_subOctave = juce::jlimit(-2, 2, oct); }
    void setWavetablePos(float pos) { m_wavetable.setPosition(pos); }

    // Returns one sample in [-1.5, 1.5] (allowing unison sum)
    inline float process() noexcept
    {
        float sample = 0.0f;
        const float baseFreq = static_cast<float>(m_frequency);

        if (m_unison == 1)
        {
            sample = renderWave(m_phase, m_waveform) * m_mix;
        }
        else
        {
            // Spread unison voices around the base frequency
            for (int i = 0; i < m_unison; ++i)
            {
                const float spread = (m_unison == 1) ? 0.0f
                    : (static_cast<float>(i) / static_cast<float>(m_unison - 1) - 0.5f) * 2.0f;
                const double detuneHz = baseFreq * (std::pow(2.0, (m_detune * spread) / 1200.0) - 1.0);
                const double voiceFreq = baseFreq + detuneHz;
                const double inc = voiceFreq / m_sampleRate;
                double voicePhase = m_phase + spread * 0.5;
                voicePhase -= std::floor(voicePhase);
                sample += renderWave(voicePhase, m_waveform) * (1.0f / m_unison) * m_mix;
                (void) inc;
            }
        }

        // Advance master phase
        m_phase += m_phaseInc;
        if (m_phase >= 1.0) m_phase -= std::floor(m_phase);

        // Sub oscillator
        if (m_subLevel > 0.001f)
        {
            m_subPhase += m_phaseInc * std::pow(2.0, -m_subOctave);
            if (m_subPhase >= 1.0) m_subPhase -= std::floor(m_subPhase);
            sample += renderWave(m_subPhase, Waveform::Square) * m_subLevel;
        }

        return sample;
    }

private:
    inline float renderWave(double phase, Waveform w) noexcept
    {
        switch (w)
        {
            case Waveform::Sine:
                return std::sin(2.0 * juce::MathConstants<double>::pi * phase);
            case Waveform::Triangle:
                return static_cast<float>(4.0 * std::abs(phase - 0.5) - 1.0);
            case Waveform::Saw:
                return static_cast<float>(2.0 * phase - 1.0);
            case Waveform::Square:
                return (phase < m_pulseWidth) ? 1.0f : -1.0f;
            case Waveform::Noise:
                return m_noise.nextFloat() * 2.0f - 1.0f;
            case Waveform::Wavetable:
                return m_wavetable.process(phase);
        }
        return 0.0f;
    }

    double m_sampleRate = 44100.0;
    double m_frequency = 440.0;
    double m_phase = 0.0;
    double m_phaseInc = 0.0;
    double m_subPhase = 0.0;
    Waveform m_waveform = Waveform::Saw;
    float m_pulseWidth = 0.5f;
    int m_unison = 1;
    float m_detune = 0.0f;
    float m_mix = 1.0f;
    float m_subLevel = 0.0f;
    int m_subOctave = -1;
    float m_lastUnison[7] = {0};
    juce::Random m_noise;
    Wavetable m_wavetable;
};

} // namespace aisynth
