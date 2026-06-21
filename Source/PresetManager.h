#pragma once

#include <JuceHeader.h>
#include "SynthEngine.h"

namespace aisynth {

// =====================================================================
// PresetManager
// ---------------------------------------------------------------------
// Persists presets as JSON in the user's app-data folder.
// Provides:
//   - factory genre packs (Trap / EDM / Cinematic / Lo-Fi / Techno)
//   - user save / load / delete
//   - undo / redo (last 32 states)
//   - random preset generator
// =====================================================================
class PresetManager {
public:
    struct PresetEntry {
        juce::String name;
        juce::String category;       // "Factory" or "User"
        juce::String prompt;         // optional text prompt that generated it
        SynthEngine::Params params;
    };

    PresetManager()
    {
        m_userDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("AI Synth").getChildFile("Presets");
        m_userDir.createDirectory();

        loadFactoryPresets();
        loadUserPresets();
    }

    // ---- Preset queries ----
    const std::vector<PresetEntry> &getPresets() const { return m_allPresets; }
    juce::StringArray getPresetNames() const
    {
        juce::StringArray names;
        for (const auto &p : m_allPresets) names.add(p.name);
        return names;
    }
    const PresetEntry *findPreset(const juce::String &name) const
    {
        for (const auto &p : m_allPresets)
            if (p.name == name) return &p;
        return nullptr;
    }

    // ---- Save / Load ----
    bool saveUserPreset(const juce::String &name,
                        const SynthEngine::Params &params,
                        const juce::String &prompt = "")
    {
        auto json = paramsToJson(params, prompt);
        auto file = m_userDir.getChildFile(name + ".json");
        if (!file.replaceWithText(json)) return false;

        // Insert or replace in user list
        PresetEntry entry{name, "User", prompt, params};
        for (auto &p : m_userPresets)
        {
            if (p.name == name) { p = entry; rebuildAll(); return true; }
        }
        m_userPresets.push_back(entry);
        rebuildAll();
        return true;
    }

    bool deleteUserPreset(const juce::String &name)
    {
        auto file = m_userDir.getChildFile(name + ".json");
        file.deleteFile();
        m_userPresets.erase(
            std::remove_if(m_userPresets.begin(), m_userPresets.end(),
                           [&](const PresetEntry &e) { return e.name == name; }),
            m_userPresets.end());
        rebuildAll();
        return true;
    }

    bool loadPreset(const juce::String &name, SynthEngine::Params &outParams) const
    {
        const PresetEntry *entry = findPreset(name);
        if (!entry) return false;
        outParams = entry->params;
        return true;
    }

    // ---- Undo / Redo ----
    void pushUndoState(const SynthEngine::Params &p)
    {
        m_undoStack.push_back(p);
        if (m_undoStack.size() > 32) m_undoStack.erase(m_undoStack.begin());
        m_redoStack.clear();
    }

    bool undo(SynthEngine::Params &outCurrent)
    {
        if (m_undoStack.empty()) return false;
        m_redoStack.push_back(outCurrent);
        outCurrent = m_undoStack.back();
        m_undoStack.pop_back();
        return true;
    }

    bool redo(SynthEngine::Params &outCurrent)
    {
        if (m_redoStack.empty()) return false;
        m_undoStack.push_back(outCurrent);
        outCurrent = m_redoStack.back();
        m_redoStack.pop_back();
        return true;
    }

    // ---- Random preset ----
    SynthEngine::Params randomPreset(juce::Random &rng) const
    {
        SynthEngine::Params p;
        p.osc1Wave = rng.nextInt(6);
        p.osc2Wave = rng.nextInt(6);
        p.osc1Unison = static_cast<float>(rng.nextInt(7) + 1);
        p.osc1Detune = rng.nextFloat() * 30.0f;
        p.osc1Sub = rng.nextFloat() * 0.8f;
        p.pulseWidth = 0.3f + rng.nextFloat() * 0.4f;
        p.wavetablePos = rng.nextFloat();
        p.osc2Coarse = (rng.nextFloat() - 0.5f) * 24.0f;
        p.osc2Level = rng.nextFloat() * 0.8f;
        p.filterType = rng.nextInt(4);
        p.filterCutoff = 100.0f + rng.nextFloat() * 8000.0f;
        p.filterReso = 0.2f + rng.nextFloat() * 0.7f;
        p.filterDrive = 1.0f + rng.nextFloat() * 2.0f;
        p.filterEnvAmt = rng.nextFloat();
        p.filterAttack = rng.nextFloat() * 0.5f;
        p.filterDecay = rng.nextFloat() * 0.5f;
        p.filterSustain = rng.nextFloat();
        p.filterRelease = rng.nextFloat() * 0.5f;
        p.ampAttack = rng.nextFloat() * 0.5f;
        p.ampDecay = rng.nextFloat() * 0.5f;
        p.ampSustain = 0.5f + rng.nextFloat() * 0.5f;
        p.ampRelease = rng.nextFloat() * 1.0f;
        p.distortionDrive = 1.0f + rng.nextFloat() * 3.0f;
        p.distortionMix = rng.nextFloat() * 0.4f;
        p.chorusRate = rng.nextFloat() * 2.0f;
        p.chorusDepth = rng.nextFloat();
        p.chorusMix = rng.nextFloat() * 0.5f;
        p.delayTime = rng.nextFloat() * 0.75f;
        p.delayFeedback = rng.nextFloat() * 0.7f;
        p.delayMix = rng.nextFloat() * 0.4f;
        p.reverbSize = rng.nextFloat();
        p.reverbDamping = rng.nextFloat();
        p.reverbMix = rng.nextFloat() * 0.5f;
        p.masterGain = 0.6f + rng.nextFloat() * 0.4f;
        p.glide = rng.nextFloat() * 0.3f;
        p.styleStrength = 1.0f;
        return p;
    }

private:
    // ---- JSON serialization ----
    static juce::String paramsToJson(const SynthEngine::Params &p, const juce::String &prompt)
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject;
        obj->setProperty("prompt", prompt);
        obj->setProperty("osc1Wave",      p.osc1Wave);
        obj->setProperty("osc1Unison",    p.osc1Unison);
        obj->setProperty("osc1Detune",    p.osc1Detune);
        obj->setProperty("osc1Sub",       p.osc1Sub);
        obj->setProperty("pulseWidth",    p.pulseWidth);
        obj->setProperty("wavetablePos",  p.wavetablePos);
        obj->setProperty("osc2Wave",      p.osc2Wave);
        obj->setProperty("osc2Coarse",    p.osc2Coarse);
        obj->setProperty("osc2Level",     p.osc2Level);
        obj->setProperty("filterType",    p.filterType);
        obj->setProperty("filterCutoff",  p.filterCutoff);
        obj->setProperty("filterReso",    p.filterReso);
        obj->setProperty("filterDrive",   p.filterDrive);
        obj->setProperty("filterEnvAmt",  p.filterEnvAmt);
        obj->setProperty("filterAttack",  p.filterAttack);
        obj->setProperty("filterDecay",   p.filterDecay);
        obj->setProperty("filterSustain", p.filterSustain);
        obj->setProperty("filterRelease", p.filterRelease);
        obj->setProperty("ampAttack",     p.ampAttack);
        obj->setProperty("ampDecay",      p.ampDecay);
        obj->setProperty("ampSustain",    p.ampSustain);
        obj->setProperty("ampRelease",    p.ampRelease);
        obj->setProperty("distortionDrive", p.distortionDrive);
        obj->setProperty("distortionMix",   p.distortionMix);
        obj->setProperty("chorusRate",    p.chorusRate);
        obj->setProperty("chorusDepth",   p.chorusDepth);
        obj->setProperty("chorusMix",     p.chorusMix);
        obj->setProperty("delayTime",     p.delayTime);
        obj->setProperty("delayFeedback", p.delayFeedback);
        obj->setProperty("delayMix",      p.delayMix);
        obj->setProperty("reverbSize",    p.reverbSize);
        obj->setProperty("reverbDamping", p.reverbDamping);
        obj->setProperty("reverbMix",     p.reverbMix);
        obj->setProperty("masterGain",    p.masterGain);
        obj->setProperty("glide",         p.glide);
        obj->setProperty("styleStrength", p.styleStrength);
        return juce::JSON::toString(obj.get());
    }

    static bool jsonToParams(const juce::String &jsonText, PresetEntry &outEntry)
    {
        auto parsed = juce::JSON::parse(jsonText);
        if (!parsed.isObject()) return false;
        auto *obj = parsed.getDynamicObject();
        if (!obj) return false;

        auto get = [&](const char *k, float def) -> float {
            auto v = obj->getProperty(k);
            return v.isDouble() || v.isInt() ? static_cast<float>(v) : def;
        };
        auto getI = [&](const char *k, int def) -> int {
            auto v = obj->getProperty(k);
            return v.isInt() ? static_cast<int>(v) : def;
        };

        outEntry.prompt = obj->getProperty("prompt");
        SynthEngine::Params &p = outEntry.params;
        p.osc1Wave = getI("osc1Wave", 2);
        p.osc1Unison = get("osc1Unison", 1.0f);
        p.osc1Detune = get("osc1Detune", 0.0f);
        p.osc1Sub = get("osc1Sub", 0.0f);
        p.pulseWidth = get("pulseWidth", 0.5f);
        p.wavetablePos = get("wavetablePos", 0.5f);
        p.osc2Wave = getI("osc2Wave", 3);
        p.osc2Coarse = get("osc2Coarse", 0.0f);
        p.osc2Level = get("osc2Level", 0.5f);
        p.filterType = getI("filterType", 0);
        p.filterCutoff = get("filterCutoff", 1000.0f);
        p.filterReso = get("filterReso", 0.7f);
        p.filterDrive = get("filterDrive", 1.0f);
        p.filterEnvAmt = get("filterEnvAmt", 0.5f);
        p.filterAttack = get("filterAttack", 0.01f);
        p.filterDecay = get("filterDecay", 0.2f);
        p.filterSustain = get("filterSustain", 0.7f);
        p.filterRelease = get("filterRelease", 0.3f);
        p.ampAttack = get("ampAttack", 0.01f);
        p.ampDecay = get("ampDecay", 0.2f);
        p.ampSustain = get("ampSustain", 0.7f);
        p.ampRelease = get("ampRelease", 0.3f);
        p.distortionDrive = get("distortionDrive", 1.0f);
        p.distortionMix = get("distortionMix", 0.0f);
        p.chorusRate = get("chorusRate", 0.5f);
        p.chorusDepth = get("chorusDepth", 0.3f);
        p.chorusMix = get("chorusMix", 0.0f);
        p.delayTime = get("delayTime", 0.25f);
        p.delayFeedback = get("delayFeedback", 0.4f);
        p.delayMix = get("delayMix", 0.0f);
        p.reverbSize = get("reverbSize", 0.5f);
        p.reverbDamping = get("reverbDamping", 0.5f);
        p.reverbMix = get("reverbMix", 0.0f);
        p.masterGain = get("masterGain", 0.8f);
        p.glide = get("glide", 0.0f);
        p.styleStrength = get("styleStrength", 1.0f);
        return true;
    }

    void loadFactoryPresets()
    {
        const juce::String factoryDir = "Factory";  // logical only – embedded
        // Embedded factory presets
        auto addFactory = [&](const juce::String &name, const juce::String &prompt,
                              const SynthEngine::Params &p) {
            m_factoryPresets.push_back({name, factoryDir, prompt, p});
        };

        // Reuse the AI engine to seed factory presets
        TextToPreset ai;
        auto make = [&](const juce::String &name, const juce::String &prompt) {
            PresetEntry e;
            e.name = name; e.category = "Factory"; e.prompt = prompt;
            e.params = ai.infer(prompt);
            m_factoryPresets.push_back(e);
        };

        make("Dark Trap Bass",       "dark trap bass");
        make("808 Sub Bass",         "trap 808 sub bass");
        make("Cinematic Emotion Pad","emotional cinematic pad");
        make("Cyberpunk Lead",       "aggressive cyberpunk lead");
        make("Lo-fi Chill Keys",     "lofi chill keys");
        make("Techno Stab",          "techno stab");
        make("Ethereal Pad",         "eerie ethereal atmospheric pad");
        make("Happy Pluck",          "happy bright pluck arp");
        make("Festival Lead",        "edm festival lead");
        make("Industrial Bass",      "aggressive industrial techno bass");
        make("Cinematic Sub Boom",   "cinematic dark sub boom");
        make("Dreamy Pad",           "emotional dreamy pad");
    }

    void loadUserPresets()
    {
        m_userPresets.clear();
        auto files = m_userDir.findChildFiles(juce::File::findFiles, false, "*.json");
        for (auto &f : files)
        {
            PresetEntry e;
            e.name = f.getFileNameWithoutExtension();
            e.category = "User";
            juce::String text;
            if (f.loadFileAsText(text) && jsonToParams(text, e))
                m_userPresets.push_back(e);
        }
        rebuildAll();
    }

    void rebuildAll()
    {
        m_allPresets.clear();
        for (const auto &p : m_factoryPresets) m_allPresets.push_back(p);
        for (const auto &p : m_userPresets)    m_allPresets.push_back(p);
    }

    juce::File m_userDir;
    std::vector<PresetEntry> m_factoryPresets;
    std::vector<PresetEntry> m_userPresets;
    std::vector<PresetEntry> m_allPresets;

    std::vector<SynthEngine::Params> m_undoStack;
    std::vector<SynthEngine::Params> m_redoStack;
};

} // namespace aisynth
