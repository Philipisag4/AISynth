#pragma once

#include <JuceHeader.h>

namespace aisynth {

// =====================================================================
// TextInputBox – styled text editor with placeholder + Enter-to-trigger
// =====================================================================
class TextInputBox : public juce::Component {
public:
    using SubmitCallback = std::function<void(const juce::String &)>;

    TextInputBox(SubmitCallback cb) : m_onSubmit(std::move(cb))
    {
        m_editor.setTextToShowWhenEmpty("Describe your sound... (e.g. 'dark trap bass', 'emotional cinematic pad', 'aggressive cyberpunk lead')",
                                        juce::Colour(0xff707070));
        m_editor.setFont(juce::Font(15.0f));
        m_editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0d0d0d));
        m_editor.setColour(juce::TextEditor::textColourId,       juce::Colour(0xffe0e0e0));
        m_editor.setColour(juce::TextEditor::outlineColourId,    juce::Colour(0xff4fc3f7));
        m_editor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff4fc3f7));
        m_editor.setJustification(juce::Justification::centredLeft);
        m_editor.setIndents(12, 8);
        m_editor.setMultiLine(false);
        m_editor.setReturnKeyStartsNewLine(false);
        m_editor.onReturnKey = [this] {
            m_onSubmit(m_editor.getText());
        };
        addAndMakeVisible(m_editor);

        m_generateButton.setButtonText("GENERATE");
        m_generateButton.setColour(juce::TextButton::buttonColourId,    juce::Colour(0xff4fc3f7));
        m_generateButton.setColour(juce::TextButton::buttonOnColourId,  juce::Colour(0xff0288d1));
        m_generateButton.setColour(juce::TextButton::textColourOffId,   juce::Colour(0xff0a0a0a));
        m_generateButton.setColour(juce::TextButton::textColourOnId,    juce::Colour(0xff0a0a0a));
        m_generateButton.onClick = [this] {
            m_onSubmit(m_editor.getText());
        };
        addAndMakeVisible(m_generateButton);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        m_generateButton.setBounds(r.removeFromRight(120).reduced(4));
        m_editor.setBounds(r.reduced(4));
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colour(0xff1f1f1f));
    }

private:
    juce::TextEditor   m_editor;
    juce::TextButton   m_generateButton;
    SubmitCallback     m_onSubmit;
};

} // namespace aisynth
