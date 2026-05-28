#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace bacillum::params
{
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

        // Noise
        inline constexpr auto noiseType      = "noise_type";
        inline constexpr auto noiseLevel     = "noise_level";

        // Chorus / Flanger / Phaser
        inline constexpr auto chorusMode     = "chorus_mode";
        inline constexpr auto chorusMix      = "chorus_mix";
        inline constexpr auto chorusRate     = "chorus_rate";
        inline constexpr auto chorusDepth    = "chorus_depth";
        inline constexpr auto chorusFeedback = "chorus_feedback";

        // Filter
        inline constexpr auto filterMode      = "filter_mode";
        inline constexpr auto filterCutoff    = "filter_cutoff";
        inline constexpr auto filterRes       = "filter_resonance";
        inline constexpr auto filterDrive     = "filter_drive";
        inline constexpr auto filterKeytrack  = "filter_keytrack";
        inline constexpr auto filterEnvAmt    = "filter_env_amount";
        inline constexpr auto filterVelAmt    = "filter_vel_amount";

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

        // Global
        inline constexpr auto polyMode       = "poly_mode";
        inline constexpr auto pitchBendRange = "pitch_bend_range";
    }

    enum class Waveform : int
    {
        Sine = 0, Triangle, Saw, Square, HyperSaw,
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
        LP12 = 0, LP24, HP12, BP12, Notch, Peak,
        NumModes
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
