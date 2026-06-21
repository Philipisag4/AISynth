#pragma once

#include <JuceHeader.h>

namespace aisynth {

// =====================================================================
// KnobComponent – rotary slider with label, value display, and value
// tooltip. Used for every synthesizer parameter.
// =====================================================================
class KnobComponent : public juce::Component {
public:
    KnobComponent(const juce::String &labelText,
                  juce::AudioProcessorValueTreeState &apvts,
                  const juce::String &paramID)
        : m_label(labelText), m_paramID(paramID)
    {
        m_slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        m_slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
        m_slider.setColour(juce::Slider::textBoxTextColourId,    juce::Colour(0xffe0e0e0));
        m_slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff2a2a2a));
        m_slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1a1a1a));
        m_slider.setColour(juce::Slider::rotarySliderFillColourId,   juce::Colour(0xff4fc3f7));
        m_slider.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff3a3a3a));
        addAndMakeVisible(m_slider);

        m_labelComp.setText(labelText, juce::dontSendNotification);
        m_labelComp.setJustificationType(juce::Justification::centred);
        m_labelComp.setColour(juce::Label::textColourId, juce::Colour(0xffb0b0b0));
        m_labelComp.setFont(juce::Font(12.0f, juce::Font::bold));
        addAndMakeVisible(m_labelComp);

        m_attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, paramID, m_slider);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        m_labelComp.setBounds(r.removeFromTop(18));
        m_slider.setBounds(r);
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colour(0xff1f1f1f));
        g.setColour(juce::Colour(0xff2c2c2c));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f, 1.0f);
    }

private:
    juce::Slider m_slider;
    juce::Label  m_labelComp;
    juce::String m_label;
    juce::String m_paramID;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_attachment;
};

} // namespace aisynth
