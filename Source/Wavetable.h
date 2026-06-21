#pragma once

#include <JuceHeader.h>

namespace aisynth {

// =====================================================================
// Wavetable – 256-sample single-cycle wavetable with morph position.
// Morphs between 4 factory tables: sine → triangle → saw → square.
// =====================================================================
class Wavetable {
public:
    static constexpr int kTableSize = 256;

    Wavetable()
    {
        buildTables();
    }

    void prepare(double sampleRate)
    {
        m_sampleRate = sampleRate;
    }

    void setPosition(float pos)
    {
        m_position = juce::jlimit(0.0f, 1.0f, pos);
    }

    inline float process(double phase) noexcept
    {
        const float readPos = static_cast<float>(phase * kTableSize);
        const int idx0 = static_cast<int>(readPos) & (kTableSize - 1);
        const int idx1 = (idx0 + 1) & (kTableSize - 1);
        const float frac = readPos - std::floor(readPos);

        // Crossfade between two adjacent wavetables based on position
        const float tablePos = m_position * (kNumTables - 1);
        const int t0 = static_cast<int>(tablePos);
        const int t1 = juce::jmin(t0 + 1, kNumTables - 1);
        const float tFrac = tablePos - t0;

        const float a = m_tables[t0][idx0] * (1.0f - frac) + m_tables[t0][idx1] * frac;
        const float b = m_tables[t1][idx0] * (1.0f - frac) + m_tables[t1][idx1] * frac;
        return a * (1.0f - tFrac) + b * tFrac;
    }

private:
    void buildTables()
    {
        // Table 0: sine
        for (int i = 0; i < kTableSize; ++i)
            m_tables[0][i] = std::sin(2.0 * juce::MathConstants<double>::pi * i / kTableSize);
        // Table 1: triangle
        for (int i = 0; i < kTableSize; ++i)
        {
            const double p = static_cast<double>(i) / kTableSize;
            m_tables[1][i] = 4.0 * std::abs(p - 0.5) - 1.0;
        }
        // Table 2: saw
        for (int i = 0; i < kTableSize; ++i)
            m_tables[2][i] = 2.0 * static_cast<double>(i) / kTableSize - 1.0;
        // Table 3: square
        for (int i = 0; i < kTableSize; ++i)
            m_tables[3][i] = (i < kTableSize / 2) ? 1.0 : -1.0;
    }

    static constexpr int kNumTables = 4;
    float m_tables[kNumTables][kTableSize] = {{0}};
    float m_position = 0.5f;
    double m_sampleRate = 44100.0;
};

} // namespace aisynth
