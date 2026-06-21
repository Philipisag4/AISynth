#include "PluginEditor.h"

namespace aisynth {

namespace {
    constexpr int kWidth  = 1180;
    constexpr int kHeight = 720;

    juce::String pid(const char *p) { return p; }
}

PluginEditor::PluginEditor(PluginProcessor &p)
    : juce::AudioProcessorEditor(&p),
      m_processor(p),
      m_textInput([this](const juce::String &s) { generateFromPrompt(s); }),
      m_styleKnob("AI Strength", p.getAPVTS(), pid("styleStrength")),
      m_spectrum([this](float *dst, int n) {
          m_processor.getSpectrum().copyMagnitude(dst, n);
      }),
      m_waveform([this]() -> float {
          return m_processor.getLatestSample();
      }),
      m_presetBrowser(p.getPresets(),
                      [this](const juce::String &n) { loadPreset(n); },
                      [this](const juce::String &n) { savePreset(n); },
                      [this](const juce::String &n) { deletePreset(n); }),
      // OSC1
      m_osc1Wave    ("Osc1 Wave",    p.getAPVTS(), pid("osc1Wave")),
      m_osc1Unison  ("Unison",       p.getAPVTS(), pid("osc1Unison")),
      m_osc1Detune  ("Detune",       p.getAPVTS(), pid("osc1Detune")),
      m_osc1Sub     ("Sub",          p.getAPVTS(), pid("osc1Sub")),
      m_pulseWidth  ("Pulse Width",  p.getAPVTS(), pid("pulseWidth")),
      m_wavetablePos("Wave Pos",     p.getAPVTS(), pid("wavetablePos")),
      // OSC2
      m_osc2Wave    ("Osc2 Wave",    p.getAPVTS(), pid("osc2Wave")),
      m_osc2Coarse  ("Coarse",       p.getAPVTS(), pid("osc2Coarse")),
      m_osc2Level   ("Level",        p.getAPVTS(), pid("osc2Level")),
      // Filter
      m_fltType     ("Type",         p.getAPVTS(), pid("filterType")),
      m_fltCutoff   ("Cutoff",       p.getAPVTS(), pid("filterCutoff")),
      m_fltReso     ("Reso",         p.getAPVTS(), pid("filterReso")),
      m_fltDrive    ("Drive",        p.getAPVTS(), pid("filterDrive")),
      m_fltEnvAmt   ("Env Amt",      p.getAPVTS(), pid("filterEnvAmt")),
      m_fltAttack   ("Attack",       p.getAPVTS(), pid("filterAttack")),
      m_fltDecay    ("Decay",        p.getAPVTS(), pid("filterDecay")),
      m_fltSustain  ("Sustain",      p.getAPVTS(), pid("filterSustain")),
      m_fltRelease  ("Release",      p.getAPVTS(), pid("filterRelease")),
      // Amp
      m_ampAttack   ("Attack",       p.getAPVTS(), pid("ampAttack")),
      m_ampDecay    ("Decay",        p.getAPVTS(), pid("ampDecay")),
      m_ampSustain  ("Sustain",      p.getAPVTS(), pid("ampSustain")),
      m_ampRelease  ("Release",      p.getAPVTS(), pid("ampRelease")),
      // FX
      m_distDrive   ("Drive",        p.getAPVTS(), pid("distortionDrive")),
      m_distMix     ("Mix",          p.getAPVTS(), pid("distortionMix")),
      m_chorusRate  ("Rate",         p.getAPVTS(), pid("chorusRate")),
      m_chorusDepth ("Depth",        p.getAPVTS(), pid("chorusDepth")),
      m_chorusMix   ("Mix",          p.getAPVTS(), pid("chorusMix")),
      m_delayTime   ("Time",         p.getAPVTS(), pid("delayTime")),
      m_delayFeedback("Feedback",    p.getAPVTS(), pid("delayFeedback")),
      m_delayMix    ("Mix",          p.getAPVTS(), pid("delayMix")),
      m_reverbSize  ("Size",         p.getAPVTS(), pid("reverbSize")),
      m_reverbDamping("Damping",     p.getAPVTS(), pid("reverbDamping")),
      m_reverbMix   ("Mix",          p.getAPVTS(), pid("reverbMix")),
      // Master
      m_masterGain  ("Master",       p.getAPVTS(), pid("masterGain")),
      m_glide       ("Glide",        p.getAPVTS(), pid("glide"))
{
    // Header
    addAndMakeVisible(m_textInput);
    addAndMakeVisible(m_styleKnob);
    addAndMakeVisible(m_randomBtn);
    addAndMakeVisible(m_undoBtn);
    addAndMakeVisible(m_redoBtn);
    addAndMakeVisible(m_saveBtn);

    m_randomBtn.onClick = [this] { randomize(); };
    m_undoBtn.onClick   = [this] { undo(); };
    m_redoBtn.onClick   = [this] { redo(); };
    m_saveBtn.onClick   = [this] {
        // Use the preset browser's name editor
        savePreset("User Preset " + juce::String(juce::Time::getCurrentTime().toMilliseconds()));
    };

    for (auto *btn : { &m_randomBtn, &m_undoBtn, &m_redoBtn, &m_saveBtn })
    {
        btn->setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff333333));
        btn->setColour(juce::TextButton::textColourOffId,  juce::Colour(0xffe0e0e0));
        btn->setColour(juce::TextButton::textColourOnId,   juce::Colour(0xff4fc3f7));
    }

    // Visualizers
    addAndMakeVisible(m_spectrum);
    addAndMakeVisible(m_waveform);

    // Preset browser
    addAndMakeVisible(m_presetBrowser);

    // All knobs
    for (auto *k : std::initializer_list<juce::Component*>{
        &m_osc1Wave, &m_osc1Unison, &m_osc1Detune, &m_osc1Sub, &m_pulseWidth, &m_wavetablePos,
        &m_osc2Wave, &m_osc2Coarse, &m_osc2Level,
        &m_fltType, &m_fltCutoff, &m_fltReso, &m_fltDrive, &m_fltEnvAmt,
        &m_fltAttack, &m_fltDecay, &m_fltSustain, &m_fltRelease,
        &m_ampAttack, &m_ampDecay, &m_ampSustain, &m_ampRelease,
        &m_distDrive, &m_distMix, &m_chorusRate, &m_chorusDepth, &m_chorusMix,
        &m_delayTime, &m_delayFeedback, &m_delayMix,
        &m_reverbSize, &m_reverbDamping, &m_reverbMix,
        &m_masterGain, &m_glide
    })
        addAndMakeVisible(k);

    setSize(kWidth, kHeight);
    setResizable(true, true);
    setResizeLimits(900, 600, 1600, 1000);
}

void PluginEditor::paint(juce::Graphics &g)
{
    // Background
    juce::ColourGradient bg(juce::Colour(0xff1a1a1a), 0, 0,
                            juce::Colour(0xff0d0d0d), 0, static_cast<float>(getHeight()), false);
    g.setGradientFill(bg);
    g.fillAll();

    // Header band
    g.setColour(juce::Colour(0xff161616));
    g.fillRect(0, 0, getWidth(), 80);

    // Title
    g.setColour(juce::Colour(0xff4fc3f7));
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("AI SYNTH", 16, 8, 200, 22, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff707070));
    g.setFont(juce::Font(11.0f));
    g.drawText("Text-to-Preset Synthesizer  v1.0", 16, 30, 300, 16, juce::Justification::centredLeft);

    // Section headers
    auto drawHeader = [&](const juce::String &title, int x, int y, int w) {
        g.setColour(juce::Colour(0xff4fc3f7));
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(title, x + 8, y, w - 16, 20, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawHorizontalLine(y + 22, static_cast<float>(x + 8), static_cast<float>(x + w - 8));
    };

    drawHeader("OSCILLATOR 1", 10, 90, 220);
    drawHeader("OSCILLATOR 2", 240, 90, 220);
    drawHeader("FILTER",       470, 90, 220);
    drawHeader("AMP ENVELOPE", 470, 280, 220);
    drawHeader("FX CHAIN",     700, 90, 470);
    drawHeader("MASTER",       240, 280, 220);
}

void PluginEditor::resized()
{
    auto r = getLocalBounds();

    // ---------- Header ----------
    auto header = r.removeFromTop(80).reduced(8);
    auto styleBox = header.removeFromRight(120);
    m_styleKnob.setBounds(styleBox);
    auto actions = header.removeFromRight(360);
    m_saveBtn.setBounds(actions.removeFromRight(90).reduced(2));
    m_redoBtn.setBounds(actions.removeFromRight(70).reduced(2));
    m_undoBtn.setBounds(actions.removeFromRight(70).reduced(2));
    m_randomBtn.setBounds(actions.removeFromRight(80).reduced(2));
    m_textInput.setBounds(header);

    // ---------- Body ----------
    auto body = r.reduced(8);

    // Left column (controls) | Right column (visualizers + presets)
    auto leftCol  = body.removeFromLeft(680);
    auto rightCol = body;

    // Left column rows
    auto osc1Row = leftCol.removeFromTop(180);
    auto osc2Row = leftCol.removeFromTop(180);
    auto fxRow   = leftCol;

    // OSC1 (220 wide, 6 knobs in 2 rows)
    auto o1 = osc1Row.removeFromLeft(220).reduced(4);
    {
        auto row1 = o1.removeFromTop(80);
        auto row2 = o1;
        const int kw = 70, kh = 76;
        m_osc1Wave.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_osc1Unison.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_osc1Detune.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_osc1Sub.setBounds(row2.removeFromLeft(kw).removeFromTop(kh));
        m_pulseWidth.setBounds(row2.removeFromLeft(kw).removeFromTop(kh));
        m_wavetablePos.setBounds(row2.removeFromLeft(kw).removeFromTop(kh));
    }

    // OSC2 (220 wide, 3 knobs)
    auto o2 = osc2Row.removeFromLeft(220).reduced(4);
    {
        auto row1 = o2.removeFromTop(80);
        const int kw = 70, kh = 76;
        m_osc2Wave.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_osc2Coarse.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_osc2Level.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
    }

    // Filter (220 wide, 9 knobs in 2 rows)
    auto flt = osc2Row.removeFromLeft(220).reduced(4);
    {
        auto row1 = flt.removeFromTop(80);
        auto row2 = flt;
        const int kw = 70, kh = 76;
        m_fltType.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_fltCutoff.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_fltReso.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_fltDrive.setBounds(row2.removeFromLeft(kw).removeFromTop(kh));
        m_fltEnvAmt.setBounds(row2.removeFromLeft(kw).removeFromTop(kh));
        // Filter env (smaller)
        // Put remaining 4 in a single row beneath
    }
    // Master + filter env in the remaining bottom space
    auto masterRow = osc2Row.removeFromBottom(80).reduced(4);
    {
        const int kw = 70, kh = 76;
        m_fltAttack.setBounds(masterRow.removeFromLeft(kw).removeFromTop(kh));
        m_fltDecay.setBounds(masterRow.removeFromLeft(kw).removeFromTop(kh));
        m_fltSustain.setBounds(masterRow.removeFromLeft(kw).removeFromTop(kh));
        m_fltRelease.setBounds(masterRow.removeFromLeft(kw).removeFromTop(kh));
    }

    // Master + amp env row (in osc1Row bottom)
    {
        const int kw = 70, kh = 76;
        auto ampRow = osc1Row.removeFromBottom(80).reduced(4);
        m_ampAttack.setBounds(ampRow.removeFromLeft(kw).removeFromTop(kh));
        m_ampDecay.setBounds(ampRow.removeFromLeft(kw).removeFromTop(kh));
        m_ampSustain.setBounds(ampRow.removeFromLeft(kw).removeFromTop(kh));
        m_ampRelease.setBounds(ampRow.removeFromLeft(kw).removeFromTop(kh));
        m_masterGain.setBounds(ampRow.removeFromLeft(kw).removeFromTop(kh));
        m_glide.setBounds(ampRow.removeFromLeft(kw).removeFromTop(kh));
    }

    // FX chain (full fxRow)
    {
        const int kw = 72, kh = 76;
        auto fx = fxRow.reduced(4);
        auto row1 = fx.removeFromTop(80);
        auto row2 = fx.removeFromTop(80);
        auto row3 = fx;
        m_distDrive.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_distMix.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_chorusRate.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_chorusDepth.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_chorusMix.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_delayTime.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_delayFeedback.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_delayMix.setBounds(row1.removeFromLeft(kw).removeFromTop(kh));
        m_reverbSize.setBounds(row2.removeFromLeft(kw).removeFromTop(kh));
        m_reverbDamping.setBounds(row2.removeFromLeft(kw).removeFromTop(kh));
        m_reverbMix.setBounds(row2.removeFromLeft(kw).removeFromTop(kh));
    }

    // ---------- Right column ----------
    auto vis = rightCol.removeFromTop(280).reduced(4);
    m_spectrum.setBounds(vis.removeFromTop(140).reduced(2));
    m_waveform.setBounds(vis.removeFromTop(80).reduced(2));
    m_presetBrowser.setBounds(rightCol.reduced(4));
}

// =====================================================================
// Action handlers
// =====================================================================
void PluginEditor::generateFromPrompt(const juce::String &prompt)
{
    if (prompt.trim().isEmpty()) return;
    m_processor.generateFromPrompt(prompt);
    repaint();
}

void PluginEditor::loadPreset(const juce::String &name)
{
    SynthEngine::Params p;
    if (m_processor.getPresets().loadPreset(name, p))
    {
        m_processor.getPresets().pushUndoState(m_processor.getSynth().getParams());
        m_processor.pushParamsToAPVTS(p);
    }
}

void PluginEditor::savePreset(const juce::String &name)
{
    m_processor.getPresets().saveUserPreset(name, m_processor.getSynth().getParams(),
                                            m_processor.getLastPrompt());
    m_presetBrowser.refresh();
}

void PluginEditor::deletePreset(const juce::String &name)
{
    m_processor.getPresets().deleteUserPreset(name);
    m_presetBrowser.refresh();
}

void PluginEditor::randomize()
{
    juce::Random rng;
    auto p = m_processor.getPresets().randomPreset(rng);
    m_processor.getPresets().pushUndoState(m_processor.getSynth().getParams());
    m_processor.pushParamsToAPVTS(p);
}

void PluginEditor::undo()
{
    auto current = m_processor.getSynth().getParams();
    if (m_processor.getPresets().undo(current))
        m_processor.pushParamsToAPVTS(current);
}

void PluginEditor::redo()
{
    auto current = m_processor.getSynth().getParams();
    if (m_processor.getPresets().redo(current))
        m_processor.pushParamsToAPVTS(current);
}

} // namespace aisynth
