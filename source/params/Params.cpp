#include "params/Params.h"

namespace bacillum::params
{
    using APVTS  = juce::AudioProcessorValueTreeState;
    using Layout = APVTS::ParameterLayout;
    using PID    = juce::ParameterID;

    static constexpr int kVersionHint = 1;

    static juce::String waveformName(int i)
    {
        switch (static_cast<Waveform>(i))
        {
            case Waveform::Sine:     return "Sine";
            case Waveform::Triangle: return "Triangle";
            case Waveform::Saw:      return "Saw";
            case Waveform::Square:   return "Square";
            case Waveform::HyperSaw: return "HyperSaw";
            default:                 return {};
        }
    }

    static juce::String subWaveformName(int i)
    {
        switch (static_cast<SubWaveform>(i))
        {
            case SubWaveform::Sine:     return "Sine";
            case SubWaveform::Triangle: return "Triangle";
            case SubWaveform::Square:   return "Square";
            default:                    return {};
        }
    }

    static juce::String subOctaveName(int i)
    {
        switch (static_cast<SubOctave>(i))
        {
            case SubOctave::Minus1: return "-1 oct";
            case SubOctave::Minus2: return "-2 oct";
            default:                return {};
        }
    }

    static juce::String chorusModeName(int i)
    {
        switch (static_cast<ChorusMode>(i))
        {
            case ChorusMode::Chorus:  return "Chorus";
            case ChorusMode::Flanger: return "Flanger";
            case ChorusMode::Phaser:  return "Phaser";
            default:                  return {};
        }
    }

    static juce::String polyModeName(int i)
    {
        switch (static_cast<PolyMode>(i))
        {
            case PolyMode::Poly:   return "Poly";
            case PolyMode::Mono:   return "Mono";
            case PolyMode::Legato: return "Legato";
            default:               return {};
        }
    }

    static juce::String noiseTypeName(int i)
    {
        switch (static_cast<NoiseType>(i))
        {
            case NoiseType::White: return "White";
            case NoiseType::Pink:  return "Pink";
            default:               return {};
        }
    }

    static juce::String filterModeName(int i)
    {
        switch (static_cast<FilterMode>(i))
        {
            case FilterMode::LP12:     return "SVF LP12";
            case FilterMode::LP24:     return "SVF LP24";
            case FilterMode::HP12:     return "SVF HP12";
            case FilterMode::BP12:     return "SVF BP12";
            case FilterMode::Notch:    return "SVF Notch";
            case FilterMode::Peak:     return "SVF Peak";
            case FilterMode::Ladder24: return "Moog LP24";
            case FilterMode::Ladder12: return "Moog LP12";
            default:                   return {};
        }
    }

    static juce::String syncDivisionName(int i)
    {
        switch (static_cast<SyncDivision>(i))
        {
            case SyncDivision::Free:      return "Free";
            case SyncDivision::D1_1:      return "1/1";
            case SyncDivision::D1_2:      return "1/2";
            case SyncDivision::D1_4Dot:   return "1/4.";
            case SyncDivision::D1_4:      return "1/4";
            case SyncDivision::D1_4Trip:  return "1/4T";
            case SyncDivision::D1_8Dot:   return "1/8.";
            case SyncDivision::D1_8:      return "1/8";
            case SyncDivision::D1_8Trip:  return "1/8T";
            case SyncDivision::D1_16:     return "1/16";
            case SyncDivision::D1_16Trip: return "1/16T";
            case SyncDivision::D1_32:     return "1/32";
            default:                      return {};
        }
    }

    static juce::String arpModeName(int i)
    {
        switch (static_cast<ArpMode>(i))
        {
            case ArpMode::Up:       return "Up";
            case ArpMode::Down:     return "Down";
            case ArpMode::UpDown:   return "Up/Down";
            case ArpMode::Random:   return "Random";
            case ArpMode::AsPlayed: return "As Played";
            default:                return {};
        }
    }

    static juce::String modSourceName(int i)
    {
        switch (static_cast<ModSource>(i))
        {
            case ModSource::None:       return "—";
            case ModSource::Env1:       return "Filt Env";
            case ModSource::Env2:       return "Amp Env";
            case ModSource::Env3:       return "Env 3";
            case ModSource::Lfo1:       return "LFO 1";
            case ModSource::Lfo2:       return "LFO 2";
            case ModSource::Lfo3:       return "LFO 3";
            case ModSource::Velocity:   return "Velocity";
            case ModSource::Note:       return "Note";
            case ModSource::ModWheel:   return "Mod Wheel";
            case ModSource::PitchBend:  return "Pitch Bend";
            case ModSource::Aftertouch: return "Aftertouch";
            case ModSource::Random:     return "Random";
            case ModSource::Constant:   return "Constant";
            default:                    return {};
        }
    }

    static juce::String modDestName(int i)
    {
        switch (static_cast<ModDest>(i))
        {
            case ModDest::None:       return "—";
            case ModDest::Cutoff:     return "Cutoff";
            case ModDest::Resonance:  return "Resonance";
            case ModDest::Drive:      return "Drive";
            case ModDest::Pitch:      return "Pitch";
            case ModDest::Osc2Pitch:  return "OSC2 Pitch";
            case ModDest::Osc1PW:     return "OSC1 PW";
            case ModDest::Osc2PW:     return "OSC2 PW";
            case ModDest::Osc1Level:  return "OSC1 Level";
            case ModDest::Osc2Level:  return "OSC2 Level";
            case ModDest::SubLevel:   return "Sub Level";
            case ModDest::NoiseLevel: return "Noise Level";
            case ModDest::Pan:        return "Pan";
            case ModDest::Amp:        return "Amp";
            case ModDest::Lfo1Rate:   return "LFO1 Rate";
            case ModDest::Lfo2Rate:   return "LFO2 Rate";
            default:                  return {};
        }
    }

    static juce::String lfoShapeName(int i)
    {
        switch (static_cast<LfoShape>(i))
        {
            case LfoShape::Sine:         return "Sine";
            case LfoShape::Triangle:     return "Triangle";
            case LfoShape::SawUp:        return "Saw Up";
            case LfoShape::SawDown:      return "Saw Down";
            case LfoShape::Square:       return "Square";
            case LfoShape::PWM:          return "PWM";
            case LfoShape::SampleHold:   return "S&H";
            case LfoShape::SmoothRandom: return "Smooth Rnd";
            default:                     return {};
        }
    }

    static juce::StringArray makeChoices(int count, juce::String (*fn)(int))
    {
        juce::StringArray out;
        for (int i = 0; i < count; ++i)
            out.add(fn(i));
        return out;
    }

    Layout createLayout()
    {
        Layout layout;

        const auto timeRange    = juce::NormalisableRange<float>{ 0.001f, 10.0f, 0.0001f, 0.3f };
        const auto cutoffRange  = juce::NormalisableRange<float>{ 20.0f, 20000.0f, 0.0f, 0.3f };
        const auto cents        = juce::NormalisableRange<float>{ -50.0f, 50.0f, 0.1f };
        const auto semi         = juce::NormalisableRange<float>{ -24.0f, 24.0f, 1.0f };
        const auto pw           = juce::NormalisableRange<float>{ 0.05f, 0.95f, 0.001f };
        const auto unit01       = juce::NormalisableRange<float>{ 0.0f, 1.0f, 0.001f };
        const auto bipolar      = juce::NormalisableRange<float>{ -1.0f, 1.0f, 0.001f };
        const auto lfoRate      = juce::NormalisableRange<float>{ 0.05f, 30.0f, 0.001f, 0.3f };
        const auto delaySec     = juce::NormalisableRange<float>{ 0.01f, 2.0f, 0.001f, 0.5f };
        const auto fbRange      = juce::NormalisableRange<float>{ 0.0f, 0.95f, 0.001f };
        const auto dampRange    = juce::NormalisableRange<float>{ 500.0f, 20000.0f, 1.0f, 0.3f };
        const auto octBipolar   = juce::NormalisableRange<float>{ -5.0f, 5.0f, 0.01f };
        const auto semiBipolar  = juce::NormalisableRange<float>{ -12.0f, 12.0f, 0.01f };

        const auto wavesArray   = makeChoices(static_cast<int>(Waveform::NumWaveforms),  waveformName);
        const auto noiseArray   = makeChoices(static_cast<int>(NoiseType::NumNoiseTypes), noiseTypeName);
        const auto filterModes  = makeChoices(static_cast<int>(FilterMode::NumModes),    filterModeName);
        const auto polyModes    = makeChoices(static_cast<int>(PolyMode::NumModes),      polyModeName);
        const auto lfoShapes    = makeChoices(static_cast<int>(LfoShape::NumShapes),     lfoShapeName);

        auto addF = [&](const char* id, const char* name,
                        const juce::NormalisableRange<float>& r, float def,
                        const char* unit = "")
        {
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                PID{ id, kVersionHint }, name, r, def,
                juce::AudioParameterFloatAttributes{}.withLabel(unit)));
        };

        // --- Master --------------------------------------------------------
        addF(ids::masterGain, "Master Gain",
             juce::NormalisableRange<float>{ -60.0f, 6.0f, 0.01f }, -6.0f, "dB");
        addF(ids::masterPan,  "Master Pan",  bipolar, 0.0f);

        // --- OSC1 ----------------------------------------------------------
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::osc1Waveform, kVersionHint }, "OSC1 Waveform",
            wavesArray, static_cast<int>(Waveform::Saw)));
        addF(ids::osc1Pitch,      "OSC1 Pitch",  semi,   0.0f, "semi");
        addF(ids::osc1Detune,     "OSC1 Detune", cents,  0.0f, "cents");
        addF(ids::osc1Pulsewidth, "OSC1 PW",     pw,     0.5f);
        addF(ids::osc1Level,      "OSC1 Level",  unit01, 0.8f);

        // --- OSC2 ----------------------------------------------------------
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::osc2Waveform, kVersionHint }, "OSC2 Waveform",
            wavesArray, static_cast<int>(Waveform::Saw)));
        addF(ids::osc2Pitch,      "OSC2 Pitch",  semi,   -12.0f, "semi");
        addF(ids::osc2Detune,     "OSC2 Detune", cents,  7.0f,   "cents");
        addF(ids::osc2Pulsewidth, "OSC2 PW",     pw,     0.5f);
        addF(ids::osc2Level,      "OSC2 Level",  unit01, 0.0f);

        // --- Sub osc -------------------------------------------------------
        addF(ids::subLevel,   "Sub Level",   unit01, 0.0f);
        const auto subWavesArray = makeChoices(static_cast<int>(SubWaveform::NumSubWaveforms), subWaveformName);
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::subWaveform, kVersionHint }, "Sub Waveform",
            subWavesArray, static_cast<int>(SubWaveform::Sine)));
        const auto subOctsArray  = makeChoices(static_cast<int>(SubOctave::NumSubOctaves), subOctaveName);
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::subOctave, kVersionHint }, "Sub Octave",
            subOctsArray, static_cast<int>(SubOctave::Minus1)));

        // --- HyperSaw shaping ---------------------------------------------
        addF(ids::hyperDetune, "Hyper Detune", unit01, 0.5f);
        addF(ids::hyperMix,    "Hyper Mix",    unit01, 0.6f);

        // --- Noise ---------------------------------------------------------
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::noiseType, kVersionHint }, "Noise Type",
            noiseArray, static_cast<int>(NoiseType::White)));
        addF(ids::noiseLevel, "Noise Level", unit01, 0.0f);

        // --- Chorus / Flanger / Phaser ------------------------------------
        const auto chorusModes = makeChoices(static_cast<int>(ChorusMode::NumChorusModes), chorusModeName);
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::chorusMode, kVersionHint }, "Chorus Mode",
            chorusModes, static_cast<int>(ChorusMode::Chorus)));
        addF(ids::chorusMix,      "Chorus Mix",      unit01, 0.0f);
        addF(ids::chorusRate,     "Chorus Rate",
             juce::NormalisableRange<float>{ 0.05f, 8.0f, 0.001f, 0.4f }, 0.6f, "Hz");
        addF(ids::chorusDepth,    "Chorus Depth",    unit01, 0.5f);
        addF(ids::chorusFeedback, "Chorus FB",       unit01, 0.2f);

        // --- Filter --------------------------------------------------------
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::filterMode, kVersionHint }, "Filter Mode",
            filterModes, static_cast<int>(FilterMode::LP12)));
        addF(ids::filterCutoff,    "Filter Cutoff",  cutoffRange, 12000.0f, "Hz");
        addF(ids::filterRes,       "Filter Reso",    unit01, 0.1f);
        addF(ids::filterDrive,     "Filter Drive",   unit01, 0.0f);
        addF(ids::filterKeytrack,  "Filter KeyTrk",  unit01, 0.0f);
        addF(ids::filterEnvAmt,    "Filter EnvAmt",  bipolar, 0.0f);
        addF(ids::filterVelAmt,    "Filter VelAmt",  unit01, 0.0f);

        // Filter env
        addF(ids::filterAttack,  "Filter A", timeRange, 0.005f, "s");
        addF(ids::filterDecay,   "Filter D", timeRange, 0.30f,  "s");
        addF(ids::filterSustain, "Filter S", unit01,    0.5f);
        addF(ids::filterRelease, "Filter R", timeRange, 0.30f,  "s");

        // Amp env
        addF(ids::ampAttack,  "Amp A", timeRange, 0.005f, "s");
        addF(ids::ampDecay,   "Amp D", timeRange, 0.20f,  "s");
        addF(ids::ampSustain, "Amp S", unit01,    0.7f);
        addF(ids::ampRelease, "Amp R", timeRange, 0.40f,  "s");

        // --- LFO1 ----------------------------------------------------------
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::lfo1Shape, kVersionHint }, "LFO1 Shape",
            lfoShapes, static_cast<int>(LfoShape::Sine)));
        addF(ids::lfo1Rate,    "LFO1 Rate",        lfoRate,    4.0f,  "Hz");
        addF(ids::lfo1FadeIn,  "LFO1 FadeIn",      timeRange,  0.0f,  "s");
        addF(ids::lfo1ToCutoff,"LFO1 → Cutoff",    octBipolar, 0.0f,  "oct");
        addF(ids::lfo1ToPitch, "LFO1 → Pitch",     semiBipolar,0.0f,  "semi");
        addF(ids::lfo1ToAmp,   "LFO1 → Amp",       unit01,     0.0f);

        // --- Unison --------------------------------------------------------
        layout.add(std::make_unique<juce::AudioParameterInt>(
            PID{ ids::unisonCount, kVersionHint }, "Unison Voices", 1, 8, 1));
        addF(ids::unisonDetune, "Unison Detune", juce::NormalisableRange<float>{ 0.0f, 50.0f, 0.1f }, 7.0f, "cents");
        addF(ids::unisonSpread, "Unison Spread", unit01, 0.5f);

        // --- Delay ---------------------------------------------------------
        addF(ids::delayMix,      "Delay Mix",       unit01,    0.0f);
        addF(ids::delayTimeL,    "Delay Time L",    delaySec,  0.25f, "s");
        addF(ids::delayTimeR,    "Delay Time R",    delaySec,  0.375f,"s");
        addF(ids::delayFeedback, "Delay Feedback",  fbRange,   0.45f);
        addF(ids::delayPingPong, "Delay PingPong",  unit01,    0.5f);
        addF(ids::delayDamp,     "Delay Damping",   dampRange, 6000.0f, "Hz");

        // --- Reverb --------------------------------------------------------
        addF(ids::reverbMix,    "Reverb Mix",     unit01, 0.0f);
        addF(ids::reverbSize,   "Reverb Size",    unit01, 0.6f);
        addF(ids::reverbDamping,"Reverb Damping", unit01, 0.4f);
        addF(ids::reverbWidth,  "Reverb Width",   unit01, 1.0f);

        // --- Tempo sync ----------------------------------------------------
        const auto syncDivs = makeChoices(static_cast<int>(SyncDivision::NumDivisions), syncDivisionName);
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::lfo1Sync, kVersionHint }, "LFO1 Sync",
            syncDivs, static_cast<int>(SyncDivision::Free)));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::delaySync, kVersionHint }, "Delay Sync",
            syncDivs, static_cast<int>(SyncDivision::Free)));

        // --- Glide ---------------------------------------------------------
        addF(ids::glideTime, "Glide",
             juce::NormalisableRange<float>{ 0.0f, 2.0f, 0.001f, 0.4f }, 0.0f, "s");

        // --- Arpeggiator ---------------------------------------------------
        layout.add(std::make_unique<juce::AudioParameterBool>(
            PID{ ids::arpOn, kVersionHint }, "Arp On", false));
        const auto arpModes = makeChoices(static_cast<int>(ArpMode::NumModes), arpModeName);
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::arpMode, kVersionHint }, "Arp Mode",
            arpModes, static_cast<int>(ArpMode::Up)));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::arpRate, kVersionHint }, "Arp Rate",
            syncDivs, static_cast<int>(SyncDivision::D1_8)));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            PID{ ids::arpOctaves, kVersionHint }, "Arp Octaves", 1, 4, 1));
        addF(ids::arpGate, "Arp Gate", unit01, 0.6f);

        // --- LFO2 ----------------------------------------------------------
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::lfo2Shape, kVersionHint }, "LFO2 Shape", lfoShapes, static_cast<int>(LfoShape::Triangle)));
        addF(ids::lfo2Rate, "LFO2 Rate", lfoRate, 2.0f, "Hz");
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::lfo2Sync, kVersionHint }, "LFO2 Sync", syncDivs, static_cast<int>(SyncDivision::Free)));
        addF(ids::lfo2FadeIn, "LFO2 FadeIn", timeRange, 0.0f, "s");

        // --- LFO3 (global) -------------------------------------------------
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::lfo3Shape, kVersionHint }, "LFO3 Shape", lfoShapes, static_cast<int>(LfoShape::Sine)));
        addF(ids::lfo3Rate, "LFO3 Rate", lfoRate, 0.5f, "Hz");
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::lfo3Sync, kVersionHint }, "LFO3 Sync", syncDivs, static_cast<int>(SyncDivision::Free)));

        // --- ENV3 (free) ---------------------------------------------------
        addF(ids::env3Attack,  "Env3 A", timeRange, 0.01f, "s");
        addF(ids::env3Decay,   "Env3 D", timeRange, 0.30f, "s");
        addF(ids::env3Sustain, "Env3 S", unit01,    0.5f);
        addF(ids::env3Release, "Env3 R", timeRange, 0.40f, "s");

        // --- Mod matrix (8 slots) -----------------------------------------
        const auto modSources = makeChoices(static_cast<int>(ModSource::NumSources), modSourceName);
        const auto modDests   = makeChoices(static_cast<int>(ModDest::NumDests),     modDestName);
        for (int i = 0; i < kNumModSlots; ++i)
        {
            layout.add(std::make_unique<juce::AudioParameterChoice>(
                PID{ ids::modSrc[(size_t) i], kVersionHint },
                "Mod " + juce::String(i + 1) + " Src", modSources, 0));
            layout.add(std::make_unique<juce::AudioParameterChoice>(
                PID{ ids::modDst[(size_t) i], kVersionHint },
                "Mod " + juce::String(i + 1) + " Dst", modDests, 0));
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                PID{ ids::modDepth[(size_t) i], kVersionHint },
                "Mod " + juce::String(i + 1) + " Depth", bipolar, 0.0f));
        }

        // --- Global --------------------------------------------------------
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID{ ids::polyMode, kVersionHint }, "Poly Mode",
            polyModes, static_cast<int>(PolyMode::Poly)));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            PID{ ids::pitchBendRange, kVersionHint }, "Pitch Bend Range", 1, 24, 2));

        return layout;
    }
}
