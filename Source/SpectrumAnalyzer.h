#pragma once

#include <JuceHeader.h>

namespace aisynth {

// =====================================================================
// SpectrumAnalyzer – FFT-based spectrum analyzer (push-only API)
// =====================================================================
class SpectrumAnalyzer {
public:
    static constexpr int kFFTOrder = 11;
    static constexpr int kFFTSize  = 1 << kFFTOrder;

    SpectrumAnalyzer()
    {
        m_window.fill(1.0f);
        for (int i = 0; i < kFFTSize; ++i)
            m_window[i] = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * juce::MathConstants<double>::pi * i / (kFFTSize - 1))));
    }

    void prepare(double sampleRate)
    {
        m_sampleRate = sampleRate;
    }

    void reset()
    {
        std::fill(m_fifo.begin(), m_fifo.end(), 0.0f);
        m_pos = 0;
        std::fill(m_magnitude.begin(), m_magnitude.end(), 0.0f);
    }

    // Push a single sample (mono sum)
    void pushSample(float s)
    {
        m_fifo[m_pos++] = s;
        if (m_pos >= kFFTSize)
        {
            processFFT();
            m_pos = 0;
        }
    }

    // Read magnitude spectrum – bins 0..numBins-1, normalized 0..1
    void copyMagnitude(float *dest, int numBins) const
    {
        const int step = (kFFTSize / 2) / numBins;
        for (int i = 0; i < numBins; ++i)
        {
            // Take max over each bin group (log-feel)
            float m = 0.0f;
            for (int j = 0; j < step; ++j)
                m = juce::jmax(m, m_magnitude[i * step + j]);
            dest[i] = juce::jlimit(0.0f, 1.0f, m);
        }
    }

private:
    void processFFT()
    {
        for (int i = 0; i < kFFTSize; ++i)
            m_fftData[i] = m_fifo[i] * m_window[i];

        m_fft.performFrequencyOnlyForwardTransform(m_fftData.data());

        // Convert to dB-normalized magnitude
        for (int i = 0; i < kFFTSize / 2; ++i)
        {
            const float mag = std::abs(m_fftData[i]) / static_cast<float>(kFFTSize);
            const float db  = (mag > 1e-7f) ? 20.0f * std::log10(mag) : -120.0f;
            const float norm = juce::jmap(db, -80.0f, 0.0f, 0.0f, 1.0f);
            // Smooth
            m_magnitude[i] = juce::jmax(norm, m_magnitude[i] * 0.85f);
        }
    }

    double m_sampleRate = 44100.0;
    int m_pos = 0;
    std::array<float, kFFTSize> m_fifo{{}};
    std::array<float, kFFTSize> m_window{{}};
    std::array<float, 2 * kFFTSize> m_fftData{{}};
    std::array<float, kFFTSize / 2> m_magnitude{{}};
    juce::dsp::FFT m_fft{kFFTOrder};
};

} // namespace aisynth
