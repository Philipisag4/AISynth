#pragma once

#include <JuceHeader.h>
#include "../PresetManager.h"

namespace aisynth {

// =====================================================================
// PresetBrowser – list of presets with category filter, save, delete.
// =====================================================================
class PresetBrowser : public juce::Component,
                      public juce::ListBoxModel {
public:
    using LoadCallback   = std::function<void(const juce::String &)>;
    using SaveCallback   = std::function<void(const juce::String &)>;
    using DeleteCallback = std::function<void(const juce::String &)>;

    PresetBrowser(PresetManager &pm,
                  LoadCallback onLoad,
                  SaveCallback onSave,
                  DeleteCallback onDelete)
        : m_presets(pm),
          m_onLoad(std::move(onLoad)),
          m_onSave(std::move(onSave)),
          m_onDelete(std::move(onDelete))
    {
        m_listBox.setModel(this);
        addAndMakeVisible(m_listBox);

        m_nameEditor.setTextToShowWhenEmpty("Preset name...", juce::Colour(0xff808080));
        m_nameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1a1a1a));
        m_nameEditor.setColour(juce::TextEditor::textColourId,       juce::Colour(0xffe0e0e0));
        m_nameEditor.setColour(juce::TextEditor::outlineColourId,    juce::Colour(0xff3a3a3a));
        addAndMakeVisible(m_nameEditor);

        m_saveButton.setButtonText("Save");
        m_saveButton.onClick = [this] {
            auto name = m_nameEditor.getText().trim();
            if (name.isNotEmpty()) { m_onSave(name); refresh(); }
        };
        addAndMakeVisible(m_saveButton);

        m_deleteButton.setButtonText("Delete");
        m_deleteButton.onClick = [this] {
            auto row = m_listBox.getSelectedRow();
            if (row >= 0)
            {
                const auto &presets = m_presets.getPresets();
                if (row < static_cast<int>(presets.size()) && presets[row].category == "User")
                {
                    m_onDelete(presets[row].name);
                    refresh();
                }
            }
        };
        addAndMakeVisible(m_deleteButton);

        refresh();
    }

    void refresh()
    {
        m_listBox.updateContent();
        m_listBox.repaint();
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto top = r.removeFromTop(28);
        m_nameEditor.setBounds(top.removeFromLeft(top.getWidth() - 140).reduced(2));
        m_saveButton.setBounds(top.removeFromLeft(70).reduced(2));
        m_deleteButton.setBounds(top.removeFromLeft(70).reduced(2));
        m_listBox.setBounds(r.reduced(2));
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colour(0xff1f1f1f));
    }

    // ---- ListBoxModel ----
    int getNumRows() override { return static_cast<int>(m_presets.getPresets().size()); }

    void paintListBoxItem(int row, juce::Graphics &g, int w, int h, bool selected) override
    {
        const auto &presets = m_presets.getPresets();
        if (row < 0 || row >= static_cast<int>(presets.size())) return;
        const auto &p = presets[row];

        g.fillAll(selected ? juce::Colour(0xff2d4a6b) : juce::Colour(0xff181818));
        g.setColour(selected ? juce::Colour(0xffe0e0e0) : juce::Colour(0xffc0c0c0));
        g.setFont(13.0f);
        g.drawText(p.name, 8, 0, w - 16, h, juce::Justification::centredLeft);

        g.setColour(p.category == "Factory" ? juce::Colour(0xff7e57c2)
                                            : juce::Colour(0xff26a69a));
        g.setFont(10.0f);
        g.drawText(p.category, w - 60, 0, 50, h, juce::Justification::centredRight);
    }

    void selectedRowsChanged(int lastRowSelected) override
    {
        const auto &presets = m_presets.getPresets();
        if (lastRowSelected >= 0 && lastRowSelected < static_cast<int>(presets.size()))
            m_onLoad(presets[lastRowSelected].name);
    }

private:
    PresetManager &m_presets;
    LoadCallback   m_onLoad;
    SaveCallback   m_onSave;
    DeleteCallback m_onDelete;
    juce::ListBox  m_listBox;
    juce::TextEditor m_nameEditor;
    juce::TextButton m_saveButton;
    juce::TextButton m_deleteButton;
};

} // namespace aisynth
