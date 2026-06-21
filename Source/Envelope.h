#pragma once

#include <JuceHeader.h>

namespace aisynth {

// =====================================================================
// Envelope – fast ADSR with adjustable curve and velocity sensitivity
// =====================================================================
class Envelope {
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void prepare(double sampleRate)
    {
        m_sampleRate = sampleRate;
        m_sampleTime = 1.0 / sampleRate;
        recalcRates();
    }

    void reset()
    {
        m_stage = Stage::Idle;
        m_value = 0.0f;
    }

    void setAttack(float seconds)  { m_attack = juce::jlimit(0.001f, 30.0f, seconds); recalcRates(); }
    void setDecay(float seconds)   { m_decay = juce::jlimit(0.001f, 30.0f, seconds);  recalcRates(); }
    void setSustain(float level)   { m_sustain = juce::jlimit(0.0f, 1.0f, level); }
    void setRelease(float seconds) { m_release = juce::jlimit(0.001f, 30.0f, seconds); recalcRates(); }

    void noteOn(float velocity)
    {
        m_velocity = velocity;
        m_stage = Stage::Attack;
        if (m_attackRate > 0) m_value = 0.0f;
    }

    void noteOff()
    {
        if (m_stage != Stage::Idle)
            m_stage = Stage::Release;
    }

    inline float process() noexcept
    {
        switch (m_stage)
        {
            case Stage::Idle:
                m_value = 0.0f;
                break;
            case Stage::Attack:
                m_value += m_attackRate;
                if (m_value >= 1.0f)
                {
                    m_value = 1.0f;
                    m_stage = Stage::Decay;
                }
                break;
            case Stage::Decay:
                m_value -= m_decayRate;
                if (m_value <= m_sustain)
                {
                    m_value = m_sustain;
                    m_stage = Stage::Sustain;
                }
                break;
            case Stage::Sustain:
                m_value = m_sustain;
                break;
            case Stage::Release:
                m_value -= m_releaseRate;
                if (m_value <= 0.0f)
                {
                    m_value = 0.0f;
                    m_stage = Stage::Idle;
                }
                break;
        }
        return m_value * m_velocity;
    }

    bool isActive() const { return m_stage != Stage::Idle; }
    Stage getStage() const { return m_stage; }

private:
    void recalcRates()
    {
        m_attackRate  = static_cast<float>(m_sampleTime / juce::jmax(0.001, m_attack));
        m_decayRate   = static_cast<float>(m_sampleTime / juce::jmax(0.001, m_decay));
        m_releaseRate = static_cast<float>(m_sampleTime / juce::jmax(0.001, m_release));
    }

    double m_sampleRate = 44100.0;
    double m_sampleTime = 1.0 / 44100.0;
    Stage m_stage = Stage::Idle;
    float m_value = 0.0f;
    float m_velocity = 1.0f;
    float m_attack = 0.01f, m_decay = 0.2f, m_sustain = 0.7f, m_release = 0.3f;
    float m_attackRate = 0.0f, m_decayRate = 0.0f, m_releaseRate = 0.0f;
};

} // namespace aisynth
