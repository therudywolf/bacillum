#pragma once

#include "params/Params.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <utility>

namespace bacillum::presets
{
    // Code-defined factory bank + APVTS apply/reset. Message-thread only
    // (preset switching is never done on the audio thread), so std::vector /
    // String allocation here is fine.
    //
    // A preset is a sparse list of (paramID → raw value) overrides applied on
    // top of a full reset-to-defaults, so presets stay small and forward
    // compatible: params a preset doesn't mention fall back to their default.
    class PresetManager
    {
    public:
        struct Override { juce::String id; float value; };
        struct Preset   { juce::String name; juce::String category; std::vector<Override> overrides; };

        PresetManager (juce::AudioProcessor& proc, juce::AudioProcessorValueTreeState& state)
            : processor (proc), apvts (state)
        {
            build();
        }

        [[nodiscard]] int getNumPresets() const { return (int) presets.size(); }
        [[nodiscard]] int getCurrentIndex() const { return current; }

        [[nodiscard]] juce::StringArray getDisplayNames() const
        {
            juce::StringArray out;
            for (const auto& p : presets)
                out.add (p.category + " / " + p.name);
            return out;
        }

        [[nodiscard]] juce::String getName (int i) const
        {
            return juce::isPositiveAndBelow (i, (int) presets.size()) ? presets[(size_t) i].name : juce::String();
        }

        [[nodiscard]] juce::String getCategory (int i) const
        {
            return juce::isPositiveAndBelow (i, (int) presets.size()) ? presets[(size_t) i].category : juce::String();
        }

        void load (int index)
        {
            if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
            current = index;

            resetAllToDefault();

            for (const auto& ov : presets[(size_t) index].overrides)
                if (auto* p = apvts.getParameter (ov.id))
                    p->setValueNotifyingHost (p->convertTo0to1 (ov.value));
        }

        void loadNext() { load ((current + 1) % getNumPresets()); }
        void loadPrev() { load ((current - 1 + getNumPresets()) % getNumPresets()); }

    private:
        void resetAllToDefault()
        {
            for (auto* p : processor.getParameters())
                p->setValueNotifyingHost (p->getDefaultValue());
        }

        // Convenience aliases for terse preset tables.
        static float W (params::Waveform w)    { return (float) (int) w; }
        static float SW (params::SubWaveform w) { return (float) (int) w; }
        static float SO (params::SubOctave o)   { return (float) (int) o; }
        static float FM (params::FilterMode m)  { return (float) (int) m; }
        static float PM (params::PolyMode m)    { return (float) (int) m; }
        static float NT (params::NoiseType n)   { return (float) (int) n; }
        static float CM (params::ChorusMode m)  { return (float) (int) m; }
        static float LS (params::LfoShape s)    { return (float) (int) s; }
        static float SD (params::SyncDivision d){ return (float) (int) d; }
        static float AM (params::ArpMode m)     { return (float) (int) m; }
        static float FR (params::FilterRouting r){ return (float) (int) r; }
        static float ST (params::SaturatorType t){ return (float) (int) t; }
        static float MS (params::ModSource s)    { return (float) (int) s; }
        static float MD (params::ModDest d)      { return (float) (int) d; }

        void build()
        {
            namespace id = params::ids;
            using W_ = params::Waveform;
            using SWv = params::SubWaveform;
            using SOc = params::SubOctave;
            using FMo = params::FilterMode;
            using PMo = params::PolyMode;
            using CMo = params::ChorusMode;
            using LSh = params::LfoShape;
            using SDv = params::SyncDivision;
            using AMo = params::ArpMode;

            presets.clear();

            // -- INIT -------------------------------------------------------
            presets.push_back ({ "Init", "Init", {} });

            // -- BASS -------------------------------------------------------
            presets.push_back ({ "Wolf Bass", "Bass", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.9f },
                { id::osc2Waveform, W(W_::Square) }, { id::osc2Pitch, 0.0f }, { id::osc2Detune, 0.0f }, { id::osc2Level, 0.5f },
                { id::subLevel, 0.7f }, { id::subWaveform, SW(SWv::Sine) }, { id::subOctave, SO(SOc::Minus1) },
                { id::filterMode, FM(FMo::Ladder24) }, { id::filterCutoff, 600.0f }, { id::filterRes, 0.35f },
                { id::filterEnvAmt, 0.5f }, { id::filterDecay, 0.25f }, { id::filterSustain, 0.2f },
                { id::ampAttack, 0.002f }, { id::ampDecay, 0.30f }, { id::ampSustain, 0.6f }, { id::ampRelease, 0.15f },
                { id::polyMode, PM(PMo::Mono) }, { id::glideTime, 0.03f } } });

            presets.push_back ({ "Reese", "Bass", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.9f },
                { id::osc2Waveform, W(W_::Saw) }, { id::osc2Pitch, 0.0f }, { id::osc2Detune, 25.0f }, { id::osc2Level, 0.9f },
                { id::unisonCount, 4.0f }, { id::unisonDetune, 18.0f }, { id::unisonSpread, 0.7f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 700.0f }, { id::filterRes, 0.2f },
                { id::ampAttack, 0.005f }, { id::ampSustain, 0.85f }, { id::ampRelease, 0.25f },
                { id::chorusMix, 0.3f }, { id::chorusMode, CM(CMo::Chorus) } } });

            presets.push_back ({ "Acid 303", "Bass", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.95f },
                { id::osc2Level, 0.0f },
                { id::filterMode, FM(FMo::Ladder24) }, { id::filterCutoff, 500.0f }, { id::filterRes, 0.8f },
                { id::filterEnvAmt, 0.6f }, { id::filterDecay, 0.22f }, { id::filterSustain, 0.1f },
                { id::ampAttack, 0.002f }, { id::ampDecay, 0.30f }, { id::ampSustain, 0.7f }, { id::ampRelease, 0.12f },
                { id::polyMode, PM(PMo::Mono) }, { id::glideTime, 0.06f } } });

            presets.push_back ({ "Sub Drop", "Bass", {
                { id::osc1Waveform, W(W_::Sine) }, { id::osc1Level, 0.8f },
                { id::subLevel, 0.9f }, { id::subWaveform, SW(SWv::Sine) }, { id::subOctave, SO(SOc::Minus2) },
                { id::filterMode, FM(FMo::LP12) }, { id::filterCutoff, 2000.0f },
                { id::ampAttack, 0.002f }, { id::ampDecay, 0.5f }, { id::ampSustain, 0.4f }, { id::ampRelease, 0.3f },
                { id::polyMode, PM(PMo::Mono) }, { id::glideTime, 0.08f } } });

            // -- LEAD -------------------------------------------------------
            presets.push_back ({ "Hyper Lead", "Lead", {
                { id::osc1Waveform, W(W_::HyperSaw) }, { id::osc1Level, 0.9f },
                { id::hyperDetune, 0.6f }, { id::hyperMix, 0.6f },
                { id::osc2Waveform, W(W_::HyperSaw) }, { id::osc2Pitch, 0.0f }, { id::osc2Detune, 0.0f }, { id::osc2Level, 0.6f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 14000.0f }, { id::filterRes, 0.1f },
                { id::ampAttack, 0.01f }, { id::ampSustain, 0.85f }, { id::ampRelease, 0.4f },
                { id::delayMix, 0.25f }, { id::delaySync, SD(SDv::D1_8) },
                { id::reverbMix, 0.25f }, { id::reverbSize, 0.7f } } });

            presets.push_back ({ "Sync Lead", "Lead", {
                { id::osc1Waveform, W(W_::Square) }, { id::osc1Pulsewidth, 0.3f }, { id::osc1Level, 0.9f },
                { id::osc2Waveform, W(W_::Saw) }, { id::osc2Pitch, 7.0f }, { id::osc2Level, 0.5f },
                { id::filterMode, FM(FMo::Ladder24) }, { id::filterCutoff, 9000.0f }, { id::filterRes, 0.25f },
                { id::filterEnvAmt, 0.3f },
                { id::ampAttack, 0.005f }, { id::ampSustain, 0.8f }, { id::ampRelease, 0.3f },
                { id::polyMode, PM(PMo::Mono) }, { id::glideTime, 0.04f },
                { id::delayMix, 0.2f }, { id::delaySync, SD(SDv::D1_4Dot) } } });

            presets.push_back ({ "Vibrato Lead", "Lead", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.9f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 8000.0f },
                { id::lfo1Shape, LS(LSh::Sine) }, { id::lfo1Rate, 5.5f }, { id::lfo1ToPitch, 0.3f }, { id::lfo1FadeIn, 0.4f },
                { id::ampAttack, 0.02f }, { id::ampSustain, 0.85f }, { id::ampRelease, 0.4f },
                { id::polyMode, PM(PMo::Legato) }, { id::glideTime, 0.05f },
                { id::reverbMix, 0.3f } } });

            // -- PAD --------------------------------------------------------
            presets.push_back ({ "Super Saw Pad", "Pad", {
                { id::osc1Waveform, W(W_::HyperSaw) }, { id::hyperDetune, 0.8f }, { id::hyperMix, 0.5f }, { id::osc1Level, 0.85f },
                { id::osc2Waveform, W(W_::HyperSaw) }, { id::osc2Pitch, 12.0f }, { id::osc2Level, 0.5f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 6000.0f },
                { id::ampAttack, 1.2f }, { id::ampDecay, 1.0f }, { id::ampSustain, 0.8f }, { id::ampRelease, 1.5f },
                { id::chorusMix, 0.4f }, { id::reverbMix, 0.5f }, { id::reverbSize, 0.85f } } });

            presets.push_back ({ "Warm Pad", "Pad", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.8f },
                { id::osc2Waveform, W(W_::Saw) }, { id::osc2Pitch, 0.0f }, { id::osc2Detune, 8.0f }, { id::osc2Level, 0.8f },
                { id::subLevel, 0.3f },
                { id::filterMode, FM(FMo::LP12) }, { id::filterCutoff, 3000.0f },
                { id::ampAttack, 0.8f }, { id::ampDecay, 1.0f }, { id::ampSustain, 0.7f }, { id::ampRelease, 1.2f },
                { id::chorusMix, 0.3f }, { id::reverbMix, 0.5f }, { id::reverbSize, 0.8f } } });

            presets.push_back ({ "Glass Pad", "Pad", {
                { id::osc1Waveform, W(W_::Triangle) }, { id::osc1Level, 0.8f },
                { id::osc2Waveform, W(W_::Sine) }, { id::osc2Pitch, 12.0f }, { id::osc2Level, 0.5f },
                { id::filterMode, FM(FMo::LP12) }, { id::filterCutoff, 7000.0f },
                { id::lfo1Shape, LS(LSh::Sine) }, { id::lfo1Rate, 0.3f }, { id::lfo1ToCutoff, 1.2f },
                { id::ampAttack, 1.0f }, { id::ampSustain, 0.8f }, { id::ampRelease, 1.6f },
                { id::reverbMix, 0.55f }, { id::reverbSize, 0.9f } } });

            // -- KEYS / PLUCK ----------------------------------------------
            presets.push_back ({ "Glass Keys", "Keys", {
                { id::osc1Waveform, W(W_::Triangle) }, { id::osc1Level, 0.85f },
                { id::osc2Waveform, W(W_::Sine) }, { id::osc2Pitch, 12.0f }, { id::osc2Level, 0.5f },
                { id::filterMode, FM(FMo::LP12) }, { id::filterCutoff, 8000.0f },
                { id::ampAttack, 0.005f }, { id::ampDecay, 0.8f }, { id::ampSustain, 0.3f }, { id::ampRelease, 0.5f },
                { id::reverbMix, 0.35f } } });

            presets.push_back ({ "Trance Pluck", "Pluck", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.9f },
                { id::osc2Waveform, W(W_::Saw) }, { id::osc2Pitch, 0.0f }, { id::osc2Detune, 10.0f }, { id::osc2Level, 0.7f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 1200.0f }, { id::filterRes, 0.3f },
                { id::filterEnvAmt, 0.7f }, { id::filterDecay, 0.25f }, { id::filterSustain, 0.0f },
                { id::ampAttack, 0.001f }, { id::ampDecay, 0.30f }, { id::ampSustain, 0.0f }, { id::ampRelease, 0.2f },
                { id::delayMix, 0.3f }, { id::delaySync, SD(SDv::D1_8) }, { id::reverbMix, 0.3f } } });

            presets.push_back ({ "Bell Pluck", "Pluck", {
                { id::osc1Waveform, W(W_::Sine) }, { id::osc1Level, 0.9f },
                { id::osc2Waveform, W(W_::Sine) }, { id::osc2Pitch, 19.0f }, { id::osc2Level, 0.5f },
                { id::filterMode, FM(FMo::LP12) }, { id::filterCutoff, 9000.0f },
                { id::ampAttack, 0.001f }, { id::ampDecay, 0.45f }, { id::ampSustain, 0.0f }, { id::ampRelease, 0.35f },
                { id::reverbMix, 0.4f }, { id::reverbSize, 0.7f } } });

            // -- BRASS / STRINGS -------------------------------------------
            presets.push_back ({ "Synth Brass", "Brass", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.85f },
                { id::osc2Waveform, W(W_::Saw) }, { id::osc2Detune, 6.0f }, { id::osc2Level, 0.85f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 2000.0f },
                { id::filterEnvAmt, 0.4f }, { id::filterAttack, 0.08f }, { id::filterDecay, 0.3f }, { id::filterSustain, 0.6f },
                { id::ampAttack, 0.05f }, { id::ampSustain, 0.85f }, { id::ampRelease, 0.3f } } });

            presets.push_back ({ "Octave Strings", "Strings", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.8f },
                { id::osc2Waveform, W(W_::Saw) }, { id::osc2Pitch, -12.0f }, { id::osc2Detune, 5.0f }, { id::osc2Level, 0.7f },
                { id::unisonCount, 3.0f }, { id::unisonDetune, 10.0f }, { id::unisonSpread, 0.6f },
                { id::filterMode, FM(FMo::LP12) }, { id::filterCutoff, 4500.0f },
                { id::ampAttack, 0.4f }, { id::ampSustain, 0.85f }, { id::ampRelease, 0.8f },
                { id::chorusMix, 0.35f }, { id::reverbMix, 0.4f } } });

            // -- ARP / SEQ --------------------------------------------------
            presets.push_back ({ "Arp Up", "Arp", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.9f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 3000.0f }, { id::filterRes, 0.2f },
                { id::filterEnvAmt, 0.4f }, { id::filterDecay, 0.2f }, { id::filterSustain, 0.2f },
                { id::ampAttack, 0.002f }, { id::ampDecay, 0.2f }, { id::ampSustain, 0.3f }, { id::ampRelease, 0.2f },
                { id::arpOn, 1.0f }, { id::arpMode, AM(AMo::Up) }, { id::arpRate, SD(SDv::D1_16) },
                { id::arpOctaves, 2.0f }, { id::arpGate, 0.5f },
                { id::delayMix, 0.3f }, { id::delaySync, SD(SDv::D1_8) }, { id::reverbMix, 0.2f } } });

            presets.push_back ({ "Arp Trance", "Arp", {
                { id::osc1Waveform, W(W_::HyperSaw) }, { id::hyperDetune, 0.5f }, { id::osc1Level, 0.85f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 6000.0f }, { id::filterRes, 0.2f },
                { id::ampAttack, 0.002f }, { id::ampDecay, 0.25f }, { id::ampSustain, 0.25f }, { id::ampRelease, 0.2f },
                { id::arpOn, 1.0f }, { id::arpMode, AM(AMo::UpDown) }, { id::arpRate, SD(SDv::D1_16) },
                { id::arpOctaves, 2.0f }, { id::arpGate, 0.7f },
                { id::delayMix, 0.3f }, { id::delaySync, SD(SDv::D1_8) }, { id::reverbMix, 0.3f } } });

            presets.push_back ({ "Random Seq", "Sequence", {
                { id::osc1Waveform, W(W_::Square) }, { id::osc1Pulsewidth, 0.5f }, { id::osc1Level, 0.9f },
                { id::filterMode, FM(FMo::Ladder24) }, { id::filterCutoff, 2500.0f }, { id::filterRes, 0.4f },
                { id::filterEnvAmt, 0.5f }, { id::filterDecay, 0.15f }, { id::filterSustain, 0.1f },
                { id::ampAttack, 0.001f }, { id::ampDecay, 0.18f }, { id::ampSustain, 0.0f }, { id::ampRelease, 0.15f },
                { id::arpOn, 1.0f }, { id::arpMode, AM(AMo::Random) }, { id::arpRate, SD(SDv::D1_16) },
                { id::arpOctaves, 2.0f }, { id::arpGate, 0.45f },
                { id::delayMix, 0.25f }, { id::delaySync, SD(SDv::D1_16) } } });

            // -- FX / MOTION ------------------------------------------------
            presets.push_back ({ "Wobble Bass", "Bass", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.9f },
                { id::subLevel, 0.5f },
                { id::filterMode, FM(FMo::Ladder24) }, { id::filterCutoff, 800.0f }, { id::filterRes, 0.45f },
                { id::lfo1Shape, LS(LSh::Sine) }, { id::lfo1Sync, SD(SDv::D1_8) }, { id::lfo1ToCutoff, 3.0f },
                { id::ampAttack, 0.003f }, { id::ampSustain, 0.9f }, { id::ampRelease, 0.2f },
                { id::polyMode, PM(PMo::Mono) } } });

            presets.push_back ({ "Phaser Keys", "Keys", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.8f },
                { id::osc2Waveform, W(W_::Saw) }, { id::osc2Detune, 8.0f }, { id::osc2Level, 0.7f },
                { id::filterMode, FM(FMo::LP12) }, { id::filterCutoff, 5000.0f },
                { id::chorusMode, CM(CMo::Phaser) }, { id::chorusMix, 0.5f }, { id::chorusRate, 0.4f }, { id::chorusFeedback, 0.5f },
                { id::ampAttack, 0.01f }, { id::ampDecay, 0.8f }, { id::ampSustain, 0.5f }, { id::ampRelease, 0.6f },
                { id::reverbMix, 0.3f } } });

            presets.push_back ({ "Noise Sweep", "FX", {
                { id::osc1Level, 0.0f }, { id::osc2Level, 0.0f },
                { id::noiseLevel, 0.9f }, { id::noiseType, NT(params::NoiseType::White) },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 800.0f }, { id::filterRes, 0.5f },
                { id::filterEnvAmt, 0.9f }, { id::filterAttack, 1.5f }, { id::filterSustain, 1.0f },
                { id::ampAttack, 1.0f }, { id::ampSustain, 1.0f }, { id::ampRelease, 1.5f },
                { id::reverbMix, 0.5f }, { id::reverbSize, 0.9f } } });

            using FRo = params::FilterRouting;
            using STy = params::SaturatorType;
            using MSr = params::ModSource;
            using MDt = params::ModDest;

            // -- Patches showcasing the v0.4–v0.6 engine ------------------
            presets.push_back ({ "WT Sweep Lead", "Lead", {
                { id::osc1Waveform, W(W_::Wavetable) }, { id::wavetablePos, 0.0f }, { id::osc1Level, 0.9f },
                { id::osc2Level, 0.0f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 9000.0f },
                { id::lfo2Shape, LS(LSh::Triangle) }, { id::lfo2Rate, 0.25f },
                { id::modSrc[0], MS(MSr::Lfo2) }, { id::modDst[0], MD(MDt::Osc1Level) }, { id::modDepth[0], 0.0f },
                // LFO2 scans the wavetable position via a matrix slot.
                { id::modSrc[1], MS(MSr::Lfo2) }, { id::modDst[1], MD(MDt::Osc1PW) }, { id::modDepth[1], 0.0f },
                { id::ampAttack, 0.02f }, { id::ampSustain, 0.85f }, { id::ampRelease, 0.4f },
                { id::reverbMix, 0.3f }, { id::delayMix, 0.2f }, { id::delaySync, SD(SDv::D1_8) } } });

            presets.push_back ({ "Sync Stab", "Lead", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.95f },
                { id::osc2Waveform, W(W_::Saw) }, { id::osc2Pitch, 0.0f }, { id::osc2Level, 0.0f },
                { id::oscSync, 1.0f },
                { id::lfo1Shape, LS(LSh::SawDown) }, { id::lfo1Rate, 6.0f }, { id::lfo1ToPitch, 7.0f },
                { id::modSrc[0], MS(MSr::Env1) }, { id::modDst[0], MD(MDt::Osc2Pitch) }, { id::modDepth[0], 0.5f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 6000.0f },
                { id::ampAttack, 0.001f }, { id::ampDecay, 0.25f }, { id::ampSustain, 0.0f }, { id::ampRelease, 0.2f },
                { id::filterDecay, 0.25f }, { id::filterEnvAmt, 0.4f },
                { id::reverbMix, 0.25f } } });

            presets.push_back ({ "FM Bell", "Pluck", {
                { id::osc1Waveform, W(W_::Sine) }, { id::osc1Level, 0.9f },
                { id::osc2Waveform, W(W_::Sine) }, { id::osc2Pitch, 14.0f }, { id::osc2Level, 0.0f },
                { id::oscFM, 0.6f },
                { id::modSrc[0], MS(MSr::Env1) }, { id::modDst[0], MD(MDt::Osc2Pitch) }, { id::modDepth[0], 0.3f },
                { id::filterMode, FM(FMo::LP12) }, { id::filterCutoff, 10000.0f },
                { id::ampAttack, 0.001f }, { id::ampDecay, 0.5f }, { id::ampSustain, 0.0f }, { id::ampRelease, 0.5f },
                { id::filterDecay, 0.4f }, { id::filterEnvAmt, 0.3f },
                { id::reverbMix, 0.4f }, { id::reverbSize, 0.75f } } });

            presets.push_back ({ "Ring Metallic", "FX", {
                { id::osc1Waveform, W(W_::Square) }, { id::osc1Level, 0.7f },
                { id::osc2Waveform, W(W_::Saw) }, { id::osc2Pitch, 7.0f }, { id::osc2Detune, 11.0f }, { id::osc2Level, 0.0f },
                { id::oscRing, 0.8f },
                { id::filterMode, FM(FMo::BP12) }, { id::filterCutoff, 2000.0f }, { id::filterRes, 0.4f },
                { id::ampAttack, 0.002f }, { id::ampDecay, 0.6f }, { id::ampSustain, 0.2f }, { id::ampRelease, 0.5f },
                { id::reverbMix, 0.45f }, { id::reverbSize, 0.85f } } });

            presets.push_back ({ "Dual Split Bass", "Bass", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.9f },
                { id::osc2Waveform, W(W_::Square) }, { id::osc2Pitch, 0.0f }, { id::osc2Level, 0.8f },
                { id::subLevel, 0.5f }, { id::noiseLevel, 0.15f },
                { id::filterRouting, FR(FRo::Split) },
                { id::filterMode, FM(FMo::Ladder24) }, { id::filterCutoff, 500.0f }, { id::filterRes, 0.3f },
                { id::filter2Mode, FM(FMo::HP12) }, { id::filter2Cutoff, 300.0f },
                { id::satType, ST(STy::Tanh) }, { id::satAmount, 0.4f },
                { id::ampAttack, 0.002f }, { id::ampDecay, 0.3f }, { id::ampSustain, 0.7f }, { id::ampRelease, 0.15f },
                { id::polyMode, PM(PMo::Mono) }, { id::glideTime, 0.03f } } });

            presets.push_back ({ "Serial Growl", "Bass", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.95f },
                { id::subLevel, 0.4f },
                { id::filterRouting, FR(FRo::Serial) },
                { id::filterMode, FM(FMo::Ladder24) }, { id::filterCutoff, 700.0f }, { id::filterRes, 0.5f },
                { id::filter2Mode, FM(FMo::LP12) }, { id::filter2Cutoff, 1500.0f },
                { id::satType, ST(STy::Foldback) }, { id::satAmount, 0.35f },
                { id::lfo1Shape, LS(LSh::Sine) }, { id::lfo1Sync, SD(SDv::D1_8) }, { id::lfo1ToCutoff, 2.5f },
                { id::ampSustain, 0.9f }, { id::polyMode, PM(PMo::Mono) }, { id::glideTime, 0.04f } } });

            presets.push_back ({ "Crush Lead", "Lead", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.9f },
                { id::osc2Waveform, W(W_::Square) }, { id::osc2Detune, 12.0f }, { id::osc2Level, 0.6f },
                { id::filterRouting, FR(FRo::Serial) },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 4000.0f },
                { id::satType, ST(STy::BitCrush) }, { id::satAmount, 0.4f },
                { id::filter2Mode, FM(FMo::LP12) }, { id::filter2Cutoff, 9000.0f },
                { id::ampAttack, 0.005f }, { id::ampSustain, 0.85f }, { id::ampRelease, 0.3f },
                { id::delayMix, 0.25f }, { id::delaySync, SD(SDv::D1_8) }, { id::reverbMix, 0.3f } } });

            presets.push_back ({ "Evolving Pad", "Pad", {
                { id::osc1Waveform, W(W_::Wavetable) }, { id::wavetablePos, 0.2f }, { id::osc1Level, 0.85f },
                { id::osc2Waveform, W(W_::Saw) }, { id::osc2Detune, 9.0f }, { id::osc2Level, 0.5f },
                { id::filterMode, FM(FMo::LP24) }, { id::filterCutoff, 4000.0f },
                { id::lfo3Shape, LS(LSh::Sine) }, { id::lfo3Rate, 0.15f },
                { id::modSrc[0], MS(MSr::Lfo3) }, { id::modDst[0], MD(MDt::Cutoff) },  { id::modDepth[0], 0.4f },
                { id::modSrc[1], MS(MSr::Lfo3) }, { id::modDst[1], MD(MDt::Pan) },     { id::modDepth[1], 0.5f },
                { id::env3Attack, 2.0f }, { id::env3Release, 2.0f },
                { id::modSrc[2], MS(MSr::Env3) }, { id::modDst[2], MD(MDt::Osc2Level) }, { id::modDepth[2], 0.4f },
                { id::ampAttack, 1.2f }, { id::ampSustain, 0.8f }, { id::ampRelease, 1.8f },
                { id::chorusMix, 0.35f }, { id::reverbMix, 0.55f }, { id::reverbSize, 0.9f } } });

            presets.push_back ({ "Aftertouch Vox", "Pad", {
                { id::osc1Waveform, W(W_::Wavetable) }, { id::wavetablePos, 0.75f }, { id::osc1Level, 0.85f },
                { id::filterMode, FM(FMo::LP12) }, { id::filterCutoff, 3000.0f },
                { id::modSrc[0], MS(MSr::Aftertouch) }, { id::modDst[0], MD(MDt::Cutoff) }, { id::modDepth[0], 0.5f },
                { id::modSrc[1], MS(MSr::Aftertouch) }, { id::modDst[1], MD(MDt::Lfo1Rate) }, { id::modDepth[1], 0.3f },
                { id::lfo1Shape, LS(LSh::Sine) }, { id::lfo1Rate, 4.0f }, { id::lfo1ToPitch, 0.2f },
                { id::ampAttack, 0.6f }, { id::ampSustain, 0.85f }, { id::ampRelease, 1.0f },
                { id::reverbMix, 0.5f } } });

            presets.push_back ({ "Velo Pluck", "Pluck", {
                { id::osc1Waveform, W(W_::Saw) }, { id::osc1Level, 0.9f },
                { id::filterMode, FM(FMo::Ladder24) }, { id::filterCutoff, 1200.0f }, { id::filterRes, 0.3f },
                { id::modSrc[0], MS(MSr::Velocity) }, { id::modDst[0], MD(MDt::Cutoff) }, { id::modDepth[0], 0.6f },
                { id::filterEnvAmt, 0.5f }, { id::filterDecay, 0.2f }, { id::filterSustain, 0.0f },
                { id::ampAttack, 0.001f }, { id::ampDecay, 0.25f }, { id::ampSustain, 0.0f }, { id::ampRelease, 0.2f },
                { id::reverbMix, 0.3f } } });

            presets.push_back ({ "Random Pluck Seq", "Sequence", {
                { id::osc1Waveform, W(W_::Square) }, { id::osc1Pulsewidth, 0.35f }, { id::osc1Level, 0.9f },
                { id::filterMode, FM(FMo::Ladder24) }, { id::filterCutoff, 2000.0f }, { id::filterRes, 0.45f },
                { id::modSrc[0], MS(MSr::Random) }, { id::modDst[0], MD(MDt::Cutoff) }, { id::modDepth[0], 0.5f },
                { id::modSrc[1], MS(MSr::Random) }, { id::modDst[1], MD(MDt::Pan) },    { id::modDepth[1], 0.6f },
                { id::ampAttack, 0.001f }, { id::ampDecay, 0.18f }, { id::ampSustain, 0.0f }, { id::ampRelease, 0.15f },
                { id::arpOn, 1.0f }, { id::arpMode, AM(AMo::Up) }, { id::arpRate, SD(SDv::D1_16) }, { id::arpOctaves, 2.0f },
                { id::delayMix, 0.3f }, { id::delaySync, SD(SDv::D1_16) } } });

            presets.push_back ({ "Plate Keys", "Keys", {
                { id::osc1Waveform, W(W_::Triangle) }, { id::osc1Level, 0.85f },
                { id::osc2Waveform, W(W_::Sine) }, { id::osc2Pitch, 12.0f }, { id::osc2Level, 0.45f },
                { id::filterMode, FM(FMo::LP12) }, { id::filterCutoff, 7000.0f },
                { id::ampAttack, 0.004f }, { id::ampDecay, 0.7f }, { id::ampSustain, 0.35f }, { id::ampRelease, 0.6f },
                { id::reverbMix, 0.5f }, { id::reverbSize, 0.85f }, { id::reverbDamping, 0.3f },
                { id::eqHighGain, 3.0f }, { id::eqHighFreq, 6000.0f } } });
        }

        juce::AudioProcessor& processor;
        juce::AudioProcessorValueTreeState& apvts;
        std::vector<Preset> presets;
        int current { 0 };
    };
}
