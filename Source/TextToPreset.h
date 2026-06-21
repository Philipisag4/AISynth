#pragma once

#include <JuceHeader.h>
#include "SynthEngine.h"

namespace aisynth {

// =====================================================================
// TextToPreset
// ---------------------------------------------------------------------
// A deterministic, on-device text→parameter inference engine.
//
// Pipeline:
//   1. Tokenize the user prompt (lowercase, split on whitespace/punct).
//   2. Score the prompt against 5 genre prototypes, 5 mood prototypes,
//      and 4 instrument-type prototypes using weighted keyword matching.
//   3. Linearly blend the parameter vectors of the top prototypes
//      weighted by their scores (soft attention).
//   4. Apply style_strength slider: lerp between current manual params
//      and the AI params.
//
// All scoring is local; no network or ML model is required. This keeps
// the plugin real-time, deterministic, and host-independent.
// =====================================================================
class TextToPreset {
public:
    struct GenrePack {
        juce::String name;
        std::vector<juce::String> keywords;
        SynthEngine::Params params;
    };

    struct MoodPack {
        juce::String name;
        std::vector<juce::String> keywords;
        // Delta applied on top of the genre baseline
        std::function<void(SynthEngine::Params &)> apply;
    };

    struct InstrumentPack {
        juce::String name;
        std::vector<juce::String> keywords;
        std::function<void(SynthEngine::Params &)> apply;
    };

    TextToPreset()
    {
        buildGenres();
        buildMoods();
        buildInstruments();
    }

    // Run inference on a user prompt. Returns the AI params (before
    // style_strength blending). The caller blends with current params.
    SynthEngine::Params infer(const juce::String &prompt) const
    {
        const auto tokens = tokenize(prompt.toLowerCase().toStdString());

        // Score genres
        auto genreScores = scorePrototypes(tokens, m_genres);
        auto moodScores  = scorePrototypes(tokens, m_moods);
        auto instScores  = scorePrototypes(tokens, m_instruments);

        // Softmax-normalize
        normalize(genreScores);
        normalize(moodScores);
        normalize(instScores);

        // Start from a neutral baseline (Trap-like default)
        SynthEngine::Params p = m_genres.front().params;

        // Weighted blend of all genre prototypes
        for (size_t i = 0; i < m_genres.size(); ++i)
            blend(p, m_genres[i].params, genreScores[i]);

        // Apply mood deltas (weighted)
        for (size_t i = 0; i < m_moods.size(); ++i)
        {
            if (moodScores[i] > 0.001f)
            {
                // Apply with strength proportional to score
                SynthEngine::Params tmp = p;
                m_moods[i].apply(tmp);
                blend(p, tmp, moodScores[i]);
            }
        }

        // Apply instrument deltas (only the strongest match)
        {
            int bestIdx = 0;
            float bestScore = 0.0f;
            for (size_t i = 0; i < m_instruments.size(); ++i)
            {
                if (instScores[i] > bestScore)
                {
                    bestScore = instScores[i];
                    bestIdx = static_cast<int>(i);
                }
            }
            if (bestScore > 0.0f)
                m_instruments[bestIdx].apply(p);
        }

        return p;
    }

    // Blend AI output with current manual params based on style_strength
    // (0.0 = fully manual, 1.0 = fully AI)
    static SynthEngine::Params applyStyleStrength(const SynthEngine::Params &current,
                                                   const SynthEngine::Params &aiParams,
                                                   float styleStrength)
    {
        styleStrength = juce::jlimit(0.0f, 1.0f, styleStrength);
        SynthEngine::Params out;

        #define LERP(field) \
            out.field = current.field * (1.0f - styleStrength) + aiParams.field * styleStrength;

        LERP(osc1Wave)
        LERP(osc1Unison)
        LERP(osc1Detune)
        LERP(osc1Sub)
        LERP(pulseWidth)
        LERP(wavetablePos)
        LERP(osc2Wave)
        LERP(osc2Coarse)
        LERP(osc2Level)
        LERP(filterType)
        LERP(filterCutoff)
        LERP(filterReso)
        LERP(filterDrive)
        LERP(filterEnvAmt)
        LERP(filterAttack)
        LERP(filterDecay)
        LERP(filterSustain)
        LERP(filterRelease)
        LERP(ampAttack)
        LERP(ampDecay)
        LERP(ampSustain)
        LERP(ampRelease)
        LERP(distortionDrive)
        LERP(distortionMix)
        LERP(chorusRate)
        LERP(chorusDepth)
        LERP(chorusMix)
        LERP(delayTime)
        LERP(delayFeedback)
        LERP(delayMix)
        LERP(reverbSize)
        LERP(reverbDamping)
        LERP(reverbMix)
        LERP(masterGain)
        LERP(glide)
        out.styleStrength = styleStrength;
        #undef LERP
        return out;
    }

private:
    // -----------------------------------------------------------------
    // Tokenizer: lowercase + split on non-alphanumeric
    // -----------------------------------------------------------------
    static std::vector<std::string> tokenize(const std::string &s)
    {
        std::vector<std::string> out;
        std::string cur;
        for (char c : s)
        {
            if (std::isalnum(static_cast<unsigned char>(c)))
                cur.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            else
            {
                if (!cur.empty()) { out.push_back(cur); cur.clear(); }
            }
        }
        if (!cur.empty()) out.push_back(cur);
        return out;
    }

    // -----------------------------------------------------------------
    // Prototype scoring: count keyword hits per prototype. Returns the
    // score array in the same order as the prototype vector.
    // -----------------------------------------------------------------
    template <typename T>
    static std::vector<float> scorePrototypes(const std::vector<std::string> &tokens,
                                              const std::vector<T> &protos)
    {
        std::vector<float> scores(protos.size(), 0.0f);
        for (size_t i = 0; i < protos.size(); ++i)
        {
            for (const auto &kw : protos[i].keywords)
            {
                const auto kwStr = kw.toLowerCase().toStdString();
                for (const auto &t : tokens)
                {
                    if (t == kwStr || t.find(kwStr) != std::string::npos
                        || kwStr.find(t) != std::string::npos)
                    {
                        scores[i] += 1.0f;
                    }
                }
            }
        }
        return scores;
    }

    static void normalize(std::vector<float> &v)
    {
        float sum = 0.0f;
        for (float x : v) sum += x;
        if (sum > 0.0f)
            for (float &x : v) x /= sum;
        else
            for (float &x : v) x = 1.0f / v.size();
    }

    static void blend(SynthEngine::Params &dst, const SynthEngine::Params &src, float w)
    {
        if (w <= 0.0f) return;
        #define BL(field) dst.field = dst.field * (1.0f - w) + src.field * w;
        BL(osc1Wave)
        BL(osc1Unison)
        BL(osc1Detune)
        BL(osc1Sub)
        BL(pulseWidth)
        BL(wavetablePos)
        BL(osc2Wave)
        BL(osc2Coarse)
        BL(osc2Level)
        BL(filterType)
        BL(filterCutoff)
        BL(filterReso)
        BL(filterDrive)
        BL(filterEnvAmt)
        BL(filterAttack)
        BL(filterDecay)
        BL(filterSustain)
        BL(filterRelease)
        BL(ampAttack)
        BL(ampDecay)
        BL(ampSustain)
        BL(ampRelease)
        BL(distortionDrive)
        BL(distortionMix)
        BL(chorusRate)
        BL(chorusDepth)
        BL(chorusMix)
        BL(delayTime)
        BL(delayFeedback)
        BL(delayMix)
        BL(reverbSize)
        BL(reverbDamping)
        BL(reverbMix)
        BL(masterGain)
        BL(glide)
        #undef BL
    }

    // -----------------------------------------------------------------
    // Prototype definitions
    // -----------------------------------------------------------------
    void buildGenres()
    {
        // ---- TRAP ----
        SynthEngine::Params trap{};
        trap.osc1Wave = 2;          // saw
        trap.osc1Unison = 3.0f;
        trap.osc1Detune = 15.0f;
        trap.osc1Sub = 0.6f;
        trap.osc2Wave = 3;          // square
        trap.osc2Level = 0.4f;
        trap.filterType = 0;
        trap.filterCutoff = 220.0f;
        trap.filterReso = 0.85f;
        trap.filterEnvAmt = 0.7f;
        trap.filterAttack = 0.005f;
        trap.filterDecay = 0.15f;
        trap.filterSustain = 0.2f;
        trap.filterRelease = 0.2f;
        trap.ampAttack = 0.005f;
        trap.ampDecay = 0.3f;
        trap.ampSustain = 0.7f;
        trap.ampRelease = 0.2f;
        trap.distortionDrive = 2.5f;
        trap.distortionMix = 0.3f;
        trap.reverbSize = 0.3f;
        trap.reverbMix = 0.1f;
        trap.masterGain = 0.9f;
        m_genres.push_back({"trap", {"trap", "808", "drill", "hiphop", "hip hop", "rap"}, trap});

        // ---- EDM ----
        SynthEngine::Params edm{};
        edm.osc1Wave = 2;
        edm.osc1Unison = 5.0f;
        edm.osc1Detune = 12.0f;
        edm.osc2Wave = 3;
        edm.osc2Level = 0.6f;
        edm.filterCutoff = 800.0f;
        edm.filterReso = 0.6f;
        edm.filterEnvAmt = 0.8f;
        edm.filterAttack = 0.01f;
        edm.filterDecay = 0.4f;
        edm.filterSustain = 0.5f;
        edm.ampAttack = 0.01f;
        edm.ampDecay = 0.4f;
        edm.ampSustain = 0.8f;
        edm.ampRelease = 0.2f;
        edm.delayTime = 0.375f;     // dotted 1/8 at 120bpm
        edm.delayFeedback = 0.4f;
        edm.delayMix = 0.25f;
        edm.reverbSize = 0.6f;
        edm.reverbMix = 0.2f;
        m_genres.push_back({"edm", {"edm", "house", "festival", "electro", "dance", "rave"}, edm});

        // ---- LO-FI ----
        SynthEngine::Params lofi{};
        lofi.osc1Wave = 1;          // triangle
        lofi.osc1Unison = 1.0f;
        lofi.osc2Wave = 0;          // sine sub
        lofi.osc2Level = 0.4f;
        lofi.filterCutoff = 1200.0f;
        lofi.filterReso = 0.3f;
        lofi.ampAttack = 0.02f;
        lofi.ampDecay = 0.5f;
        lofi.ampSustain = 0.6f;
        lofi.ampRelease = 0.6f;
        lofi.chorusRate = 0.3f;
        lofi.chorusDepth = 0.4f;
        lofi.chorusMix = 0.4f;
        lofi.delayTime = 0.5f;
        lofi.delayFeedback = 0.5f;
        lofi.delayMix = 0.25f;
        lofi.reverbSize = 0.7f;
        lofi.reverbMix = 0.35f;
        m_genres.push_back({"lofi", {"lofi", "lo fi", "chill", "chillhop", "jazz", "boom bap"}, lofi});

        // ---- CINEMATIC ----
        SynthEngine::Params cinematic{};
        cinematic.osc1Wave = 5;     // wavetable
        cinematic.wavetablePos = 0.7f;
        cinematic.osc1Unison = 4.0f;
        cinematic.osc1Detune = 8.0f;
        cinematic.osc2Wave = 1;
        cinematic.osc2Level = 0.5f;
        cinematic.filterCutoff = 2500.0f;
        cinematic.filterReso = 0.4f;
        cinematic.ampAttack = 1.2f;
        cinematic.ampDecay = 0.8f;
        cinematic.ampSustain = 0.8f;
        cinematic.ampRelease = 2.0f;
        cinematic.reverbSize = 0.9f;
        cinematic.reverbDamping = 0.3f;
        cinematic.reverbMix = 0.5f;
        cinematic.delayTime = 0.5f;
        cinematic.delayMix = 0.2f;
        cinematic.chorusMix = 0.2f;
        m_genres.push_back({"cinematic", {"cinematic", "film", "movie", "score", "soundtrack", "epic"}, cinematic});

        // ---- TECHNO ----
        SynthEngine::Params techno{};
        techno.osc1Wave = 3;        // square
        techno.osc1Unison = 2.0f;
        techno.osc1Detune = 6.0f;
        techno.osc2Wave = 2;
        techno.osc2Level = 0.3f;
        techno.filterCutoff = 500.0f;
        techno.filterReso = 0.9f;
        techno.filterEnvAmt = 0.6f;
        techno.filterAttack = 0.01f;
        techno.filterDecay = 0.3f;
        techno.filterSustain = 0.3f;
        techno.ampAttack = 0.005f;
        techno.ampDecay = 0.2f;
        techno.ampSustain = 0.9f;
        techno.ampRelease = 0.05f;
        techno.distortionDrive = 3.0f;
        techno.distortionMix = 0.2f;
        techno.delayTime = 0.375f;
        techno.delayFeedback = 0.5f;
        techno.delayMix = 0.2f;
        techno.reverbSize = 0.4f;
        techno.reverbMix = 0.1f;
        m_genres.push_back({"techno", {"techno", "industrial", "acid", "minimal", "underground"}, techno});
    }

    void buildMoods()
    {
        m_moods.push_back({"dark", {"dark", "evil", "scary", "horror", "grim", "sinister"},
            [](SynthEngine::Params &p) {
                p.filterCutoff = juce::jmax(80.0f, p.filterCutoff * 0.5f);
                p.filterReso = juce::jmax(p.filterReso, 0.85f);
                p.distortionDrive = juce::jmax(p.distortionDrive, 2.5f);
                p.distortionMix = juce::jmax(p.distortionMix, 0.3f);
                p.reverbSize = juce::jmax(p.reverbSize, 0.6f);
                p.reverbDamping = juce::jmin(p.reverbDamping, 0.3f);
                p.osc1Detune = juce::jmax(p.osc1Detune, 10.0f);
            }});

        m_moods.push_back({"happy", {"happy", "bright", "joyful", "uplifting", "sunny", "cheerful"},
            [](SynthEngine::Params &p) {
                p.filterCutoff = juce::jmin(8000.0f, p.filterCutoff * 1.8f);
                p.filterReso = juce::jmin(p.filterReso, 0.4f);
                p.osc1Wave = 2;          // saw
                p.osc2Wave = 3;
                p.osc2Coarse = 7.0f;     // perfect fifth up
                p.chorusMix = juce::jmax(p.chorusMix, 0.3f);
                p.delayMix = juce::jmax(p.delayMix, 0.2f);
                p.reverbMix = juce::jmax(p.reverbMix, 0.25f);
            }});

        m_moods.push_back({"emotional", {"emotional", "sad", "melancholic", "melancholy", "deep", "yearning"},
            [](SynthEngine::Params &p) {
                p.ampAttack = juce::jmax(p.ampAttack, 0.3f);
                p.ampRelease = juce::jmax(p.ampRelease, 1.0f);
                p.filterAttack = juce::jmax(p.filterAttack, 0.3f);
                p.filterCutoff = juce::jmin(3000.0f, p.filterCutoff * 0.8f);
                p.reverbSize = juce::jmax(p.reverbSize, 0.8f);
                p.reverbMix = juce::jmax(p.reverbMix, 0.4f);
                p.delayMix = juce::jmax(p.delayMix, 0.3f);
                p.delayFeedback = juce::jmax(p.delayFeedback, 0.5f);
            }});

        m_moods.push_back({"aggressive", {"aggressive", "hard", "heavy", "brutal", "metal", "distorted", "angry"},
            [](SynthEngine::Params &p) {
                p.distortionDrive = juce::jmax(p.distortionDrive, 5.0f);
                p.distortionMix = juce::jmax(p.distortionMix, 0.5f);
                p.filterReso = juce::jmax(p.filterReso, 0.9f);
                p.ampAttack = juce::jmin(p.ampAttack, 0.005f);
                p.ampRelease = juce::jmin(p.ampRelease, 0.1f);
                p.filterEnvAmt = juce::jmax(p.filterEnvAmt, 0.8f);
                p.osc1Unison = juce::jmax(p.osc1Unison, 3.0f);
            }});

        m_moods.push_back({"eerie", {"eerie", "creepy", "unsettling", "mysterious", "ethereal", "atmospheric"},
            [](SynthEngine::Params &p) {
                p.osc1Wave = 5;          // wavetable
                p.wavetablePos = 0.85f;
                p.osc1Detune = juce::jmax(p.osc1Detune, 8.0f);
                p.osc2Coarse = 0.5f;     // microtonal detune for beating
                p.ampAttack = juce::jmax(p.ampAttack, 0.5f);
                p.ampRelease = juce::jmax(p.ampRelease, 1.5f);
                p.reverbSize = juce::jmax(p.reverbSize, 0.9f);
                p.reverbMix = juce::jmax(p.reverbMix, 0.5f);
                p.chorusMix = juce::jmax(p.chorusMix, 0.4f);
                p.filterCutoff = juce::jmin(2000.0f, p.filterCutoff * 0.7f);
            }});
    }

    void buildInstruments()
    {
        m_instruments.push_back({"bass", {"bass", "sub", "808", "low", "bottom"},
            [](SynthEngine::Params &p) {
                p.osc1Wave = 2;
                p.osc1Sub = juce::jmax(p.osc1Sub, 0.8f);
                p.osc2Level = 0.2f;
                p.filterCutoff = juce::jmin(p.filterCutoff, 400.0f);
                p.filterReso = juce::jmax(p.filterReso, 0.7f);
                p.ampAttack = juce::jmin(p.ampAttack, 0.01f);
                p.ampRelease = juce::jmin(p.ampRelease, 0.15f);
                p.reverbMix = juce::jmin(p.reverbMix, 0.15f);
                p.distortionDrive = juce::jmax(p.distortionDrive, 1.5f);
                p.masterGain = juce::jmax(p.masterGain, 0.95f);
            }});

        m_instruments.push_back({"lead", {"lead", "synth lead", "melody", "topline", "hook"},
            [](SynthEngine::Params &p) {
                p.osc1Wave = 2;
                p.osc1Unison = juce::jmax(p.osc1Unison, 3.0f);
                p.osc2Wave = 3;
                p.osc2Level = 0.5f;
                p.osc2Coarse = 0.0f;
                p.filterCutoff = juce::jmax(p.filterCutoff, 2000.0f);
                p.ampAttack = juce::jmin(p.ampAttack, 0.02f);
                p.ampRelease = juce::jmax(p.ampRelease, 0.2f);
                p.delayMix = juce::jmax(p.delayMix, 0.2f);
                p.reverbMix = juce::jmax(p.reverbMix, 0.2f);
                p.chorusMix = juce::jmax(p.chorusMix, 0.2f);
            }});

        m_instruments.push_back({"pad", {"pad", "ambience", "atmosphere", "texture", "drone"},
            [](SynthEngine::Params &p) {
                p.osc1Wave = 5;          // wavetable
                p.wavetablePos = 0.6f;
                p.osc1Unison = juce::jmax(p.osc1Unison, 5.0f);
                p.osc1Detune = juce::jmax(p.osc1Detune, 12.0f);
                p.osc2Wave = 1;
                p.osc2Level = 0.6f;
                p.filterCutoff = juce::jmin(p.filterCutoff, 3000.0f);
                p.ampAttack = juce::jmax(p.ampAttack, 0.8f);
                p.ampDecay = juce::jmax(p.ampDecay, 0.5f);
                p.ampSustain = juce::jmax(p.ampSustain, 0.8f);
                p.ampRelease = juce::jmax(p.ampRelease, 1.5f);
                p.reverbSize = juce::jmax(p.reverbSize, 0.8f);
                p.reverbMix = juce::jmax(p.reverbMix, 0.4f);
                p.chorusMix = juce::jmax(p.chorusMix, 0.3f);
            }});

        m_instruments.push_back({"pluck", {"pluck", "arp", "arpeggio", "staccato", "stab"},
            [](SynthEngine::Params &p) {
                p.osc1Wave = 2;
                p.osc2Wave = 3;
                p.osc2Level = 0.4f;
                p.filterCutoff = juce::jmax(p.filterCutoff, 1500.0f);
                p.filterEnvAmt = juce::jmax(p.filterEnvAmt, 0.7f);
                p.filterDecay = juce::jmin(p.filterDecay, 0.15f);
                p.ampAttack = juce::jmin(p.ampAttack, 0.005f);
                p.ampDecay = juce::jmin(p.ampDecay, 0.3f);
                p.ampSustain = juce::jmin(p.ampSustain, 0.2f);
                p.ampRelease = juce::jmin(p.ampRelease, 0.2f);
                p.delayMix = juce::jmax(p.delayMix, 0.3f);
                p.reverbMix = juce::jmax(p.reverbMix, 0.2f);
            }});

        m_instruments.push_back({"stab", {"stab", "chord", "stabby", "punchy"},
            [](SynthEngine::Params &p) {
                p.osc1Wave = 2;
                p.osc1Unison = 2.0f;
                p.osc2Wave = 3;
                p.osc2Level = 0.5f;
                p.filterEnvAmt = juce::jmax(p.filterEnvAmt, 0.85f);
                p.filterAttack = juce::jmin(p.filterAttack, 0.005f);
                p.filterDecay = juce::jmin(p.filterDecay, 0.1f);
                p.filterSustain = 0.1f;
                p.ampAttack = juce::jmin(p.ampAttack, 0.005f);
                p.ampDecay = juce::jmin(p.ampDecay, 0.2f);
                p.ampSustain = 0.3f;
                p.ampRelease = juce::jmin(p.ampRelease, 0.15f);
                p.distortionMix = juce::jmax(p.distortionMix, 0.15f);
            }});
    }

    std::vector<GenrePack> m_genres;
    std::vector<MoodPack> m_moods;
    std::vector<InstrumentPack> m_instruments;
};

} // namespace aisynth
