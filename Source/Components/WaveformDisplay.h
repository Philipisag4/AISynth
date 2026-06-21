#pragma once

#include <JuceHeader.h>

namespace aisynth {

// =====================================================================
// WaveformDisplay – scrolling oscilloscope that listens to the audio
// thread via a lock-free ring buffer.
// =====================================================================
class WaveformDisplay : public juce::Component,
                        private juce::Timer {
public:
    using GetSampleFn = std::function<float()>;

    WaveformDisplay(GetSampleFn fn) : m_getSample(std::move(fn))
    {
        startTimerHz(60);
    }

    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(juce::Colour(0xff111111));
        g.fillRoundedRectangle(r, 4.0f);

        // Center line
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawHorizontalLine(static_cast<int>(r.getCentreY()), r.getX(), r.getRight());

        // Waveform path
        const int w = static_cast<int>(r.getWidth());
        juce::Path p;
        p.startNewSubPath(r.getX(), r.getCentreY());
        for (int i = 0; i < w; ++i)
        {
            const float s = (i < static_cast<int>(m_buffer.size())) ? m_buffer[i] : 0.0f;
            const float y = r.getCentreY() + s * r.getHeight() * 0.45f;
            p.lineTo(r.getX() + i, y);
        }
        g.setColour(juce::Colour(0xff4ade80));
        g.strokePath(p, juce::PathStrokeType(1.5f));

        g.setColour(juce::Colour(0xffe0e0e0));
        g.drawRoundedRectangle(r, 4.0f, 1.0f);
    }

private:
    void timerCallback() override
    {
        // Pull a window of samples
        const int w = getWidth();
        if (w <= 0) return;
        m_buffer.resize(static_cast<size_t>(w));
        for (int i = 0; i < w; ++i)
            m_buffer[i] = m_getSample();
        repaint();
    }

    GetSampleFn m_getSample;
    std::vector<float> m_buffer;
};

} // namespace aisynth
