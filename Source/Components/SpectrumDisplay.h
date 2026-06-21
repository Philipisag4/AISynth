#pragma once

#include <JuceHeader.h>

namespace aisynth {

// =====================================================================
// SpectrumDisplay – real-time FFT spectrum
// Pulls magnitude data from PluginProcessor's SpectrumAnalyzer on a timer.
// =====================================================================
class SpectrumDisplay : public juce::Component,
                        private juce::Timer {
public:
    using GetMagnitudeFn = std::function<void(float *, int)>;

    SpectrumDisplay(GetMagnitudeFn fn) : m_getMagnitude(std::move(fn))
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(juce::Colour(0xff111111));
        g.fillRoundedRectangle(r, 4.0f);

        // Grid
        g.setColour(juce::Colour(0xff2a2a2a));
        for (int i = 1; i < 8; ++i)
        {
            const float y = r.getY() + r.getHeight() * i / 8.0f;
            g.drawHorizontalLine(static_cast<int>(y), r.getX(), r.getRight());
        }

        // Spectrum bars
        const int numBars = 64;
        float mags[numBars];
        m_getMagnitude(mags, numBars);

        const float barW = r.getWidth() / numBars;
        juce::Path path;
        path.startNewSubPath(r.getX(), r.getBottom());
        for (int i = 0; i < numBars; ++i)
        {
            const float x = r.getX() + i * barW + barW * 0.5f;
            const float h = mags[i] * r.getHeight();
            const float y = r.getBottom() - h;
            path.lineTo(x, y);
        }
        path.lineTo(r.getRight(), r.getBottom());
        path.closeSubPath();

        juce::ColourGradient grad(juce::Colour(0xff4fc3f7), r.getX(), r.getBottom(),
                                  juce::Colour(0xffec407a), r.getX(), r.getY(), false);
        g.setGradientFill(grad);
        g.fillPath(path);

        g.setColour(juce::Colour(0xffe0e0e0));
        g.drawRoundedRectangle(r, 4.0f, 1.0f);
    }

private:
    void timerCallback() override { repaint(); }
    GetMagnitudeFn m_getMagnitude;
};

} // namespace aisynth
