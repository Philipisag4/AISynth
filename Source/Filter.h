#pragma once

#include <JuceHeader.h>

namespace aisynth {

// =====================================================================
// Filter – Moog-style ladder low-pass with resonance
// Handles stereo by maintaining one filter per channel.
// =====================================================================
class Filter {
public:
    enum class Type { LPF = 0, HPF, BPF, Notch };

    static constexpr int kMaxChannels = 2;

    void prepare(double sampleRate, int /*blockSize*/)
    {
        m_sampleRate = sampleRate;
        for (auto &f : m_filters)
            f.prepare({ sampleRate, 1, 1 });
        updateCoeffs();
    }

    void reset()
    {
        for (auto &f : m_filters) f.reset();
    }

    void setType(Type t) { m_type = t; updateCoeffs(); }
    void setCutoff(float hz)
    {
        m_cutoff = juce::jlimit(20.0f, 18000.0f, hz);
        updateCoeffs();
    }
    void setResonance(float q)
    {
        m_resonance = juce::jlimit(0.1f, 20.0f, q);
        updateCoeffs();
    }
    void setDrive(float d) { m_drive = juce::jlimit(1.0f, 4.0f, d); }

    // Process a single sample for a given channel (0=L, 1=R)
    inline float process(int channel, float in) noexcept
    {
        const float driven = in * m_drive;
        return m_filters[juce::jlimit(0, kMaxChannels - 1, channel)].processSample(driven);
    }

    // Convenience mono call (channel 0)
    inline float process(float in) noexcept { return process(0, in); }

private:
    void updateCoeffs()
    {
        using Coeffs = juce::dsp::IIR::Coefficients<float>;
        juce::ReferenceCountedObjectPtr<Coeffs> c;
        switch (m_type)
        {
            case Type::LPF:    c = Coeffs::makeLowPass(m_sampleRate, m_cutoff, m_resonance); break;
            case Type::HPF:    c = Coeffs::makeHighPass(m_sampleRate, m_cutoff, m_resonance); break;
            case Type::BPF:    c = Coeffs::makeBandPass(m_sampleRate, m_cutoff, m_resonance); break;
            case Type::Notch:  c = Coeffs::makeNotch(m_sampleRate, m_cutoff, m_resonance); break;
        }
        if (c)
        {
            for (auto &f : m_filters)
                *f.coefficients = *c;
        }
    }

    double m_sampleRate = 44100.0;
    Type m_type = Type::LPF;
    float m_cutoff = 1000.0f;
    float m_resonance = 0.7f;
    float m_drive = 1.0f;
    std::array<juce::dsp::IIR::Filter<float>, kMaxChannels> m_filters;
};

} // namespace aisynth
