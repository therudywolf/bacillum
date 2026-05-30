#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace bacillum::params
{
    inline constexpr int kNumModSlots = 8;

    namespace ids
    {
        inline constexpr auto masterGain    = "master_gain";
        inline constexpr auto masterPan     = "master_pan";

        // OSC1
        inline constexpr auto osc1Waveform   = "osc1_waveform";
        inline constexpr auto osc1Pitch      = "osc1_pitch_semi";
        inline constexpr auto osc1Detune     = "osc1_detune_cents";
        inline constexpr auto osc1Pulsewidth = "osc1_pulsewidth";
        inline constexpr auto osc1Level      = "osc1_level";

        // OSC2
        inline constexpr auto osc2Waveform   = "osc2_waveform";
        inline constexpr auto osc2Pitch      = "osc2_pitch_semi";
        inline constexpr auto osc2Detune     = "osc2_detune_cents";
        inline constexpr auto osc2Pulsewidth = "osc2_pulsewidth";
        inline constexpr auto osc2Level      = "osc2_level";

        // Sub osc
        inline constexpr auto subLevel       = "sub_level";
        inline constexpr auto subWaveform    = "sub_waveform";
        inline constexpr auto subOctave      = "sub_octave";

        // HyperSaw shaping (applies when OSC1 or OSC2 waveform == HyperSaw)
        inline constexpr auto hyperDetune    = "hyper_detune";   // 0..1
        inline constexpr auto hyperMix       = "hyper_mix";      // 0..1

        // Wavetable scan position (applies when waveform == Wavetable)
        inline constexpr auto wavetablePos   = "wavetable_pos";  // 0..1

        // OSC interop
        inline constexpr auto oscSync        = "osc_sync";       // bool: OSC2 hard-syncs OSC1
        inline constexpr auto oscRing        = "osc_ring";       // 0..1 ring-mod level into mix
        inline constexpr auto oscFM          = "osc_fm";         // 0..1 OSC2 → OSC1 phase mod

        // Noise
        inline constexpr auto noiseType      = "noise_type";
        inline constexpr auto noiseLevel     = "noise_level";

        // Chorus / Flanger / Phaser
        inline constexpr auto chorusMode     = "chorus_mode";
        inline constexpr auto chorusMix      = "chorus_mix";
        inline constexpr auto chorusRate     = "chorus_rate";
        inline constexpr auto chorusDepth    = "chorus_depth";
        inline constexpr auto chorusFeedback = "chorus_feedback";

        // Glide / portamento
        inline constexpr auto glideTime      = "glide_time";    // seconds, 0 = off

        // Tempo sync
        inline constexpr auto lfo1Sync       = "lfo1_sync";     // SyncDivision (Free + note values)
        inline constexpr auto delaySync      = "delay_sync";    // SyncDivision

        // Arpeggiator
        inline constexpr auto arpOn          = "arp_on";
        inline constexpr auto arpMode        = "arp_mode";
        inline constexpr auto arpRate        = "arp_rate";      // SyncDivision (step length)
        inline constexpr auto arpOctaves     = "arp_octaves";   // 1..4
        inline constexpr auto arpGate        = "arp_gate";      // 0..1

        // LFO2 (per-voice, key-triggered)
        inline constexpr auto lfo2Shape      = "lfo2_shape";
        inline constexpr auto lfo2Rate       = "lfo2_rate_hz";
        inline constexpr auto lfo2Sync       = "lfo2_sync";
        inline constexpr auto lfo2FadeIn     = "lfo2_fade_in";

        // LFO3 (global, free-running)
        inline constexpr auto lfo3Shape      = "lfo3_shape";
        inline constexpr auto lfo3Rate       = "lfo3_rate_hz";
        inline constexpr auto lfo3Sync       = "lfo3_sync";

        // ENV3 (free assignable, ADSR)
        inline constexpr auto env3Attack     = "env3_attack";
        inline constexpr auto env3Decay      = "env3_decay";
        inline constexpr auto env3Sustain    = "env3_sustain";
        inline constexpr auto env3Release    = "env3_release";

        // Mod matrix: kNumModSlots × (source, dest, depth)
        inline constexpr std::array<const char*, kNumModSlots> modSrc {
            "mod1_src","mod2_src","mod3_src","mod4_src","mod5_src","mod6_src","mod7_src","mod8_src" };
        inline constexpr std::array<const char*, kNumModSlots> modDst {
            "mod1_dst","mod2_dst","mod3_dst","mod4_dst","mod5_dst","mod6_dst","mod7_dst","mod8_dst" };
        inline constexpr std::array<const char*, kNumModSlots> modDepth {
            "mod1_dep","mod2_dep","mod3_dep","mod4_dep","mod5_dep","mod6_dep","mod7_dep","mod8_dep" };

        // Filter 1
        inline constexpr auto filterMode      = "filter_mode";
        inline constexpr auto filterCutoff    = "filter_cutoff";
        inline constexpr auto filterRes       = "filter_resonance";
        inline constexpr auto filterDrive     = "filter_drive";
        inline constexpr auto filterKeytrack  = "filter_keytrack";
        inline constexpr auto filterEnvAmt    = "filter_env_amount";
        inline constexpr auto filterVelAmt    = "filter_vel_amount";

        // Filter 2 + routing + saturator
        inline constexpr auto filterRouting   = "filter_routing";
        inline constexpr auto filter2Mode     = "filter2_mode";
        inline constexpr auto filter2Cutoff   = "filter2_cutoff";
        inline constexpr auto filter2Res      = "filter2_resonance";
        inline constexpr auto satType         = "saturator_type";
        inline constexpr auto satAmount       = "saturator_amount";

        // Filter env
        inline constexpr auto filterAttack   = "filter_attack";
        inline constexpr auto filterDecay    = "filter_decay";
        inline constexpr auto filterSustain  = "filter_sustain";
        inline constexpr auto filterRelease  = "filter_release";

        // Amp env
        inline constexpr auto ampAttack      = "amp_attack";
        inline constexpr auto ampDecay       = "amp_decay";
        inline constexpr auto ampSustain     = "amp_sustain";
        inline constexpr auto ampRelease     = "amp_release";

        // LFO1 (key-triggered, per-voice)
        inline constexpr auto lfo1Shape      = "lfo1_shape";
        inline constexpr auto lfo1Rate       = "lfo1_rate_hz";
        inline constexpr auto lfo1FadeIn     = "lfo1_fade_in";
        inline constexpr auto lfo1ToCutoff   = "lfo1_to_cutoff_oct";    // ±5 oct
        inline constexpr auto lfo1ToPitch    = "lfo1_to_pitch_semi";    // ±12 semi
        inline constexpr auto lfo1ToAmp      = "lfo1_to_amp";           // 0..1 tremolo

        // Unison
        inline constexpr auto unisonCount    = "unison_count";          // 1..8
        inline constexpr auto unisonDetune   = "unison_detune";         // cents
        inline constexpr auto unisonSpread   = "unison_spread";         // 0..1

        // Delay
        inline constexpr auto delayMix       = "delay_mix";             // 0..1
        inline constexpr auto delayTimeL     = "delay_time_l";          // sec
        inline constexpr auto delayTimeR     = "delay_time_r";          // sec
        inline constexpr auto delayFeedback  = "delay_feedback";        // 0..0.95
        inline constexpr auto delayPingPong  = "delay_ping_pong";       // 0..1 cross
        inline constexpr auto delayDamp      = "delay_damping_hz";      // Hz

        // Reverb (juce::Reverb wrap)
        inline constexpr auto reverbMix      = "reverb_mix";            // 0..1
        inline constexpr auto reverbSize     = "reverb_size";           // 0..1
        inline constexpr auto reverbDamping  = "reverb_damping";        // 0..1
        inline constexpr auto reverbWidth    = "reverb_width";          // 0..1

        // 3-band EQ (front of FX chain)
        inline constexpr auto eqLowFreq      = "eq_low_freq";
        inline constexpr auto eqLowGain      = "eq_low_gain";
        inline constexpr auto eqMidFreq      = "eq_mid_freq";
        inline constexpr auto eqMidGain      = "eq_mid_gain";
        inline constexpr auto eqMidQ         = "eq_mid_q";
        inline constexpr auto eqHighFreq     = "eq_high_freq";
        inline constexpr auto eqHighGain     = "eq_high_gain";

        // Compressor (end of FX chain)
        inline constexpr auto compThresh     = "comp_threshold";
        inline constexpr auto compRatio      = "comp_ratio";
        inline constexpr auto compAttack     = "comp_attack";
        inline constexpr auto compRelease    = "comp_release";
        inline constexpr auto compMakeup     = "comp_makeup";

        // Global
        inline constexpr auto polyMode       = "poly_mode";
        inline constexpr auto pitchBendRange = "pitch_bend_range";
    }

    enum class Waveform : int
    {
        Sine = 0, Triangle, Saw, Square, HyperSaw, Wavetable,
        NumWaveforms
    };

    enum class SubWaveform : int
    {
        Sine = 0, Triangle, Square,
        NumSubWaveforms
    };

    enum class SubOctave : int
    {
        Minus1 = 0, Minus2,
        NumSubOctaves
    };

    enum class NoiseType : int
    {
        White = 0, Pink,
        NumNoiseTypes
    };

    enum class ChorusMode : int
    {
        Chorus = 0, Flanger, Phaser,
        NumChorusModes
    };

    enum class FilterMode : int
    {
        // SVF (Cytomic TPT) modes …
        LP12 = 0, LP24, HP12, BP12, Notch, Peak,
        // … then Moog ladder (Huovilainen) models.
        Ladder24, Ladder12,
        NumModes
    };

    // Dual-filter topology (spec §1.1).
    enum class FilterRouting : int
    {
        Single = 0,   // F1 only
        Serial,       // F1 → saturator → F2
        Parallel,     // F1(mix) + F2(mix)
        Split,        // F1(osc1+sub) + F2(osc2+noise)
        NumRoutings
    };

    enum class SaturatorType : int
    {
        Off = 0, Tanh, SoftClip, HardClip, Foldback, BitCrush, RateReduce,
        NumTypes
    };

    // Note divisions for tempo-synced LFO / delay / arpeggiator.
    // Index 0 = Free (use the Hz/seconds knob instead).
    enum class SyncDivision : int
    {
        Free = 0,
        D1_1, D1_2, D1_4Dot, D1_4, D1_4Trip,
        D1_8Dot, D1_8, D1_8Trip,
        D1_16, D1_16Trip, D1_32,
        NumDivisions
    };

    // Returns the length of one cycle/step in beats (quarter notes).
    // 0 means "free running" (caller should use the manual rate).
    [[nodiscard]] inline float syncBeats (SyncDivision d) noexcept
    {
        switch (d)
        {
            case SyncDivision::D1_1:      return 4.0f;
            case SyncDivision::D1_2:      return 2.0f;
            case SyncDivision::D1_4Dot:   return 1.5f;
            case SyncDivision::D1_4:      return 1.0f;
            case SyncDivision::D1_4Trip:  return 2.0f / 3.0f;
            case SyncDivision::D1_8Dot:   return 0.75f;
            case SyncDivision::D1_8:      return 0.5f;
            case SyncDivision::D1_8Trip:  return 1.0f / 3.0f;
            case SyncDivision::D1_16:     return 0.25f;
            case SyncDivision::D1_16Trip: return 1.0f / 6.0f;
            case SyncDivision::D1_32:     return 0.125f;
            case SyncDivision::Free:
            default:                      return 0.0f;
        }
    }

    enum class ArpMode : int
    {
        Up = 0, Down, UpDown, Random, AsPlayed,
        NumModes
    };

    // Modulation-matrix sources. Order must match the source-value array
    // assembled in Voice and the name table in Params.cpp.
    enum class ModSource : int
    {
        None = 0,
        Env1, Env2, Env3,         // filter env, amp env, free env
        Lfo1, Lfo2, Lfo3,         // 2 per-voice + 1 global
        Velocity, Note,
        ModWheel, PitchBend, Aftertouch,
        Random,                   // per-note random, fixed for the note's life
        Constant,                 // always 1.0 (offset / bias)
        NumSources
    };

    // Modulation-matrix destinations.
    enum class ModDest : int
    {
        None = 0,
        Cutoff, Resonance, Drive,
        Pitch, Osc2Pitch,
        Osc1PW, Osc2PW,
        Osc1Level, Osc2Level, SubLevel, NoiseLevel,
        Pan, Amp,
        Lfo1Rate, Lfo2Rate,
        Cutoff2, Reso2,
        NumDests
    };

    enum class PolyMode : int
    {
        Poly = 0, Mono, Legato,
        NumModes
    };

    enum class LfoShape : int
    {
        Sine = 0, Triangle, SawUp, SawDown, Square, PWM, SampleHold, SmoothRandom,
        NumShapes
    };

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
