#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace aisynth {

// =====================================================================
// Parameter IDs – central place so the editor and processor agree.
// =====================================================================
namespace PID {
    #define X(name) static const juce::String name = #name;
    X(osc1Wave) X(osc1Unison) X(osc1Detune) X(osc1Sub) X(pulseWidth) X(wavetablePos)
    X(osc2Wave) X(osc2Coarse) X(osc2Level)
    X(filterType) X(filterCutoff) X(filterReso) X(filterDrive) X(filterEnvAmt)
    X(filterAttack) X(filterDecay) X(filterSustain) X(filterRelease)
    X(ampAttack) X(ampDecay) X(ampSustain) X(ampRelease)
    X(distortionDrive) X(distortionMix)
    X(chorusRate) X(chorusDepth) X(chorusMix)
    X(delayTime) X(delayFeedback) X(delayMix)
    X(reverbSize) X(reverbDamping) X(reverbMix)
    X(masterGain) X(glide) X(styleStrength)
    #undef X
}

PluginProcessor::PluginProcessor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Force initial param sync
    pullParamsFromAPVTS();
    applyCurrentParams();
}

juce::AudioProcessorValueTreeState::ParameterLayout
PluginProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // Helper lambdas
    auto choice = [](const String &id, std::initializer_list<const char *> options) {
        StringArray sa;
        for (auto o : options) sa.add(o);
        return std::make_unique<AudioParameterChoice>(ParameterID{id, 1}, id, sa, 0);
    };
    auto flt = [](const String &id, float lo, float hi, float def) {
        return std::make_unique<AudioParameterFloat>(
            ParameterID{id, 1}, id, NormalisableRange<float>(lo, hi), def);
    };

    // Oscillators
    params.push_back(choice(PID::osc1Wave, {"Sine","Tri","Saw","Square","Noise","Wave"}));
    params.push_back(flt(PID::osc1Unison,   1.0f,  7.0f, 1.0f));
    params.push_back(flt(PID::osc1Detune,   0.0f, 100.0f, 0.0f));
    params.push_back(flt(PID::osc1Sub,      0.0f,  1.0f, 0.0f));
    params.push_back(flt(PID::pulseWidth,   0.05f, 0.95f, 0.5f));
    params.push_back(flt(PID::wavetablePos, 0.0f,  1.0f, 0.5f));
    params.push_back(choice(PID::osc2Wave, {"Sine","Tri","Saw","Square","Noise","Wave"}));
    params.push_back(flt(PID::osc2Coarse,  -24.0f, 24.0f, 0.0f));
    params.push_back(flt(PID::osc2Level,    0.0f,  1.0f, 0.5f));

    // Filter
    params.push_back(choice(PID::filterType, {"LPF","HPF","BPF","Notch"}));
    params.push_back(flt(PID::filterCutoff, 20.0f, 18000.0f, 1000.0f));
    params.push_back(flt(PID::filterReso,   0.1f, 20.0f, 0.7f));
    params.push_back(flt(PID::filterDrive,  1.0f, 4.0f, 1.0f));
    params.push_back(flt(PID::filterEnvAmt, 0.0f, 1.0f, 0.5f));
    params.push_back(flt(PID::filterAttack, 0.001f, 5.0f, 0.01f));
    params.push_back(flt(PID::filterDecay,  0.001f, 5.0f, 0.2f));
    params.push_back(flt(PID::filterSustain,0.0f, 1.0f, 0.7f));
    params.push_back(flt(PID::filterRelease,0.001f, 5.0f, 0.3f));

    // Amp env
    params.push_back(flt(PID::ampAttack,  0.001f, 5.0f, 0.01f));
    params.push_back(flt(PID::ampDecay,   0.001f, 5.0f, 0.2f));
    params.push_back(flt(PID::ampSustain, 0.0f, 1.0f, 0.7f));
    params.push_back(flt(PID::ampRelease, 0.001f, 5.0f, 0.3f));

    // FX
    params.push_back(flt(PID::distortionDrive, 1.0f, 20.0f, 1.0f));
    params.push_back(flt(PID::distortionMix,   0.0f, 1.0f, 0.0f));
    params.push_back(flt(PID::chorusRate,  0.0f, 10.0f, 0.5f));
    params.push_back(flt(PID::chorusDepth, 0.0f, 1.0f, 0.3f));
    params.push_back(flt(PID::chorusMix,   0.0f, 1.0f, 0.0f));
    params.push_back(flt(PID::delayTime,     0.0f, 2.0f, 0.25f));
    params.push_back(flt(PID::delayFeedback, 0.0f, 0.95f, 0.4f));
    params.push_back(flt(PID::delayMix,      0.0f, 1.0f, 0.0f));
    params.push_back(flt(PID::reverbSize,    0.0f, 1.0f, 0.5f));
    params.push_back(flt(PID::reverbDamping, 0.0f, 1.0f, 0.5f));
    params.push_back(flt(PID::reverbMix,     0.0f, 1.0f, 0.0f));

    // Master
    params.push_back(flt(PID::masterGain, 0.0f, 1.0f, 0.8f));
    params.push_back(flt(PID::glide,      0.0f, 1.0f, 0.0f));
    params.push_back(flt(PID::styleStrength, 0.0f, 1.0f, 1.0f));

    return { params.begin(), params.end() };
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_synth.prepare(sampleRate, samplesPerBlock, juce::jmin(getTotalNumOutputChannels(), 2));
    m_spectrum.prepare(sampleRate);
    pullParamsFromAPVTS();
    applyCurrentParams();
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    const auto &main = layouts.getMainOutputChannelSet();
    if (main == juce::AudioChannelSet::stereo() || main == juce::AudioChannelSet::mono())
        return true;
    return false;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    pullParamsFromAPVTS();
    applyCurrentParams();

    m_synth.process(buffer, midi);

    // Push mono-summed samples to the spectrum analyzer
    const int numSamples = buffer.getNumSamples();
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            mono += buffer.getSample(ch, i);
        mono /= juce::jmax(1, buffer.getNumChannels());
        m_spectrum.pushSample(mono);
        m_latestSample.store(mono);
    }
}

void PluginProcessor::pullParamsFromAPVTS()
{
    auto get = [&](const juce::String &id, float def) -> float {
        auto *param = m_apvts.getRawParameterValue(id);
        return param ? param->load() : def;
    };
    auto getI = [&](const juce::String &id, int def) -> int {
        auto *param = m_apvts.getRawParameterValue(id);
        return param ? static_cast<int>(param->load()) : def;
    };

    SynthEngine::Params newParams;
    newParams.osc1Wave = getI(PID::osc1Wave, 2);
    newParams.osc1Unison = get(PID::osc1Unison, 1.0f);
    newParams.osc1Detune = get(PID::osc1Detune, 0.0f);
    newParams.osc1Sub = get(PID::osc1Sub, 0.0f);
    newParams.pulseWidth = get(PID::pulseWidth, 0.5f);
    newParams.wavetablePos = get(PID::wavetablePos, 0.5f);
    newParams.osc2Wave = getI(PID::osc2Wave, 3);
    newParams.osc2Coarse = get(PID::osc2Coarse, 0.0f);
    newParams.osc2Level = get(PID::osc2Level, 0.5f);
    newParams.filterType = getI(PID::filterType, 0);
    newParams.filterCutoff = get(PID::filterCutoff, 1000.0f);
    newParams.filterReso = get(PID::filterReso, 0.7f);
    newParams.filterDrive = get(PID::filterDrive, 1.0f);
    newParams.filterEnvAmt = get(PID::filterEnvAmt, 0.5f);
    newParams.filterAttack = get(PID::filterAttack, 0.01f);
    newParams.filterDecay = get(PID::filterDecay, 0.2f);
    newParams.filterSustain = get(PID::filterSustain, 0.7f);
    newParams.filterRelease = get(PID::filterRelease, 0.3f);
    newParams.ampAttack = get(PID::ampAttack, 0.01f);
    newParams.ampDecay = get(PID::ampDecay, 0.2f);
    newParams.ampSustain = get(PID::ampSustain, 0.7f);
    newParams.ampRelease = get(PID::ampRelease, 0.3f);
    newParams.distortionDrive = get(PID::distortionDrive, 1.0f);
    newParams.distortionMix = get(PID::distortionMix, 0.0f);
    newParams.chorusRate = get(PID::chorusRate, 0.5f);
    newParams.chorusDepth = get(PID::chorusDepth, 0.3f);
    newParams.chorusMix = get(PID::chorusMix, 0.0f);
    newParams.delayTime = get(PID::delayTime, 0.25f);
    newParams.delayFeedback = get(PID::delayFeedback, 0.4f);
    newParams.delayMix = get(PID::delayMix, 0.0f);
    newParams.reverbSize = get(PID::reverbSize, 0.5f);
    newParams.reverbDamping = get(PID::reverbDamping, 0.5f);
    newParams.reverbMix = get(PID::reverbMix, 0.0f);
    newParams.masterGain = get(PID::masterGain, 0.8f);
    newParams.glide = get(PID::glide, 0.0f);
    newParams.styleStrength = get(PID::styleStrength, 1.0f);

    m_synth.applyParams(newParams);
}

void PluginProcessor::applyCurrentParams()
{
    // Already applied in pullParamsFromAPVTS; kept for API symmetry.
}

void PluginProcessor::pushParamsToAPVTS(const SynthEngine::Params &p)
{
    // Set each parameter without triggering a recursive update
    auto set = [&](const juce::String &id, float v) {
        if (auto *param = m_apvts.getParameter(id))
            param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(v));
    };
    auto setI = [&](const juce::String &id, int v) {
        set(id, static_cast<float>(v));
    };

    setI(PID::osc1Wave,       p.osc1Wave);
    set (PID::osc1Unison,     p.osc1Unison);
    set (PID::osc1Detune,     p.osc1Detune);
    set (PID::osc1Sub,        p.osc1Sub);
    set (PID::pulseWidth,     p.pulseWidth);
    set (PID::wavetablePos,   p.wavetablePos);
    setI(PID::osc2Wave,       p.osc2Wave);
    set (PID::osc2Coarse,     p.osc2Coarse);
    set (PID::osc2Level,      p.osc2Level);
    setI(PID::filterType,     p.filterType);
    set (PID::filterCutoff,   p.filterCutoff);
    set (PID::filterReso,     p.filterReso);
    set (PID::filterDrive,    p.filterDrive);
    set (PID::filterEnvAmt,   p.filterEnvAmt);
    set (PID::filterAttack,   p.filterAttack);
    set (PID::filterDecay,    p.filterDecay);
    set (PID::filterSustain,  p.filterSustain);
    set (PID::filterRelease,  p.filterRelease);
    set (PID::ampAttack,      p.ampAttack);
    set (PID::ampDecay,       p.ampDecay);
    set (PID::ampSustain,     p.ampSustain);
    set (PID::ampRelease,     p.ampRelease);
    set (PID::distortionDrive,p.distortionDrive);
    set (PID::distortionMix,  p.distortionMix);
    set (PID::chorusRate,     p.chorusRate);
    set (PID::chorusDepth,    p.chorusDepth);
    set (PID::chorusMix,      p.chorusMix);
    set (PID::delayTime,      p.delayTime);
    set (PID::delayFeedback,  p.delayFeedback);
    set (PID::delayMix,       p.delayMix);
    set (PID::reverbSize,     p.reverbSize);
    set (PID::reverbDamping,  p.reverbDamping);
    set (PID::reverbMix,      p.reverbMix);
    set (PID::masterGain,     p.masterGain);
    set (PID::glide,          p.glide);
    set (PID::styleStrength,  p.styleStrength);
}

void PluginProcessor::generateFromPrompt(const juce::String &prompt)
{
    auto current = m_synth.getParams();
    auto aiParams = m_ai.infer(prompt);

    // Snapshot the current state for undo
    m_presets.pushUndoState(current);

    // Blend by current style_strength
    auto merged = TextToPreset::applyStyleStrength(current, aiParams, current.styleStrength);

    pushParamsToAPVTS(merged);
    pullParamsFromAPVTS();
    m_lastPrompt = prompt;
    aiGeneratedFlag.store(true);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(m_apvts.state.getType()))
    {
        m_apvts.replaceState(juce::ValueTree::fromXml(*xml));
        pullParamsFromAPVTS();
        applyCurrentParams();
    }
}

juce::AudioProcessorEditor *PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

} // namespace aisynth

// =====================================================================
// JUCE plugin entry-point macros
// =====================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new aisynth::PluginProcessor();
}
