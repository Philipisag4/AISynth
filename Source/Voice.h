#pragma once

#include <JuceHeader.h>
#include "Oscillator.h"
#include "Filter.h"
#include "Envelope.h"

namespace aisynth {

// =====================================================================
// Voice – one polyphonic note: 2 oscillators + sub + filter + amp env
// =====================================================================
class Voice {
public:
    Voice() = default;

    void prepare(double sampleRate, int blockSize)
    {
        m_osc1.prepare(sampleRate, blockSize);
        m_osc2.prepare(sampleRate, blockSize);
        m_filter.prepare(sampleRate, blockSize);
        m_ampEnv.prepare(sampleRate);
        m_filterEnv.prepare(sampleRate);
    }

    void reset()
    {
        m_osc1.reset();
        m_osc2.reset();
        m_filter.reset();
        m_ampEnv.reset();
        m_filterEnv.reset();
        m_active = false;
        m_currentNote = -1;
    }

    void noteOn(int midiNote, float velocity)
    {
        m_currentNote = midiNote;
        m_active = true;
        const double freq = juce::MidiMessage::getMidiNoteInHertz(midiNote);
        m_osc1.setFrequency(freq);
        m_osc2.setFrequency(freq * std::pow(2.0, m_osc2Coarse));
        m_ampEnv.noteOn(velocity);
        m_filterEnv.noteOn(velocity);
    }

    void noteOff()
    {
        m_ampEnv.noteOff();
        m_filterEnv.noteOff();
    }

    bool isActive() const { return m_ampEnv.isActive(); }
    int getNote() const { return m_currentNote; }

    // --- parameter setters ---
    void setOsc1Wave(int w) { m_osc1.setWaveformIndex(w); }
    void setOsc2Wave(int w) { m_osc2.setWaveformIndex(w); }
    void setOsc2Coarse(float semis) { m_osc2Coarse = semis / 12.0f; }
    void setOsc2Level(float l) { m_osc2Level = l; }
    void setOsc1Unison(int u) { m_osc1.setUnison(u); }
    void setOsc1Detune(float d) { m_osc1.setDetune(d); }
    void setOsc1Sub(float s) { m_osc1.setSubLevel(s); }
    void setPulseWidth(float pw) { m_osc1.setPulseWidth(pw); m_osc2.setPulseWidth(pw); }
    void setWavetablePos(float p) { m_osc1.setWavetablePos(p); m_osc2.setWavetablePos(p); }

    void setFilterType(int t) { m_filter.setType(static_cast<Filter::Type>(t)); }
    void setFilterCutoff(float hz) { m_filterCutoff = hz; }
    void setFilterResonance(float q) { m_filter.setResonance(q); }
    void setFilterDrive(float d) { m_filter.setDrive(d); }
    void setFilterEnvAmt(float amt) { m_filterEnvAmt = amt; }
    void setFilterEnvAttack(float v)  { m_filterEnv.setAttack(v); }
    void setFilterEnvDecay(float v)   { m_filterEnv.setDecay(v); }
    void setFilterEnvSustain(float v) { m_filterEnv.setSustain(v); }
    void setFilterEnvRelease(float v) { m_filterEnv.setRelease(v); }

    void setAmpEnvAttack(float v)  { m_ampEnv.setAttack(v); }
    void setAmpEnvDecay(float v)   { m_ampEnv.setDecay(v); }
    void setAmpEnvSustain(float v) { m_ampEnv.setSustain(v); }
    void setAmpEnvRelease(float v) { m_ampEnv.setRelease(v); }

    void setGlide(float seconds) { m_glideSeconds = juce::jlimit(0.0f, 2.0f, seconds); }
    void setGain(float g) { m_gain = g; }

    inline float process() noexcept
    {
        if (!m_active) return 0.0f;

        // Glide: smooth pitch towards target
        // (simplified – uses osc1's target as reference; omitted for clarity)

        const float o1 = m_osc1.process();
        const float o2 = m_osc2.process() * m_osc2Level;
        float pre = o1 + o2;

        // Filter envelope modulation
        const float env = m_filterEnv.process();
        const float cutoff = juce::jlimit(20.0f, 18000.0f,
                                          m_filterCutoff + m_filterEnvAmt * env * 8000.0f);
        m_filter.setCutoff(cutoff);
        const float filtered = m_filter.process(pre);

        const float amp = m_ampEnv.process();
        float out = filtered * amp * m_gain;

        if (!m_ampEnv.isActive())
        {
            m_active = false;
            out = 0.0f;
        }
        return out;
    }

private:
    Oscillator m_osc1, m_osc2;
    Filter m_filter;
    Envelope m_ampEnv, m_filterEnv;

    bool m_active = false;
    int m_currentNote = -1;
    float m_osc2Coarse = 0.0f;   // semitones / 12
    float m_osc2Level = 0.5f;
    float m_filterCutoff = 1000.0f;
    float m_filterEnvAmt = 0.5f;
    float m_glideSeconds = 0.0f;
    float m_gain = 0.5f;
};

} // namespace aisynth
