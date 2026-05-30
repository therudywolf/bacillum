#pragma once

#include "dsp/oscillators/Oscillator.h"
#include "dsp/oscillators/HyperSaw.h"
#include "dsp/oscillators/Noise.h"
#include "dsp/filters/SvfTpt.h"
#include "dsp/filters/MoogLadder.h"
#include "dsp/effects/Saturator.h"
#include "dsp/envelopes/Adsr.h"
#include "dsp/lfo/Lfo.h"
#include "dsp/DcBlocker.h"
#include "params/Params.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace bacillum::dsp
{
    // One modulation-matrix routing.
    struct ModSlot
    {
        params::ModSource source { params::ModSource::None };
        params::ModDest   dest   { params::ModDest::None };
        float             depth  { 0.0f };   // -1..+1
    };

    // Per-block snapshot of audio-relevant parameters.
    struct VoiceParams
    {
        // OSC1
        params::Waveform osc1Waveform { params::Waveform::Saw };
        float osc1PitchSemi   { 0.0f };
        float osc1DetuneCents { 0.0f };
        float osc1Pulsewidth  { 0.5f };
        float osc1Level       { 0.8f };

        // OSC2
        params::Waveform osc2Waveform { params::Waveform::Saw };
        float osc2PitchSemi   { -12.0f };
        float osc2DetuneCents { 7.0f };
        float osc2Pulsewidth  { 0.5f };
        float osc2Level       { 0.0f };

        // Sub
        float subLevel    { 0.0f };
        params::SubWaveform subWaveform { params::SubWaveform::Sine };
        params::SubOctave   subOctave   { params::SubOctave::Minus1 };

        // HyperSaw shaping (shared for any OSC running in HyperSaw mode)
        float hyperDetune { 0.5f };
        float hyperMix    { 0.6f };

        // Noise
        params::NoiseType noiseType { params::NoiseType::White };
        float noiseLevel  { 0.0f };

        // Filter 1
        params::FilterMode filterMode { params::FilterMode::LP12 };
        float filterCutoff    { 12000.0f };
        float filterRes01     { 0.1f };
        float filterDrive01   { 0.0f };
        float filterKeytrack  { 0.0f };
        float filterEnvAmount { 0.0f };
        float filterVelAmount { 0.0f };

        // Filter 2 + routing + saturator
        params::FilterRouting filterRouting { params::FilterRouting::Single };
        params::FilterMode    filter2Mode   { params::FilterMode::LP12 };
        float filter2Cutoff { 8000.0f };
        float filter2Res01  { 0.1f };
        params::SaturatorType satType   { params::SaturatorType::Off };
        float satAmount { 0.5f };

        // Filter envelope
        float fEnvA { 0.005f }, fEnvD { 0.30f }, fEnvS { 0.5f }, fEnvR { 0.30f };

        // Amp envelope
        float ampA { 0.005f }, ampD { 0.20f }, ampS { 0.7f }, ampR { 0.40f };

        // LFO1 (key-triggered, per-voice)
        Lfo::Shape lfo1Shape { Lfo::Shape::Sine };
        float lfo1RateHz      { 4.0f };
        float lfo1FadeInSec   { 0.0f };
        float lfo1ToCutoffOct { 0.0f };   // depth in octaves on cutoff
        float lfo1ToPitchSemi { 0.0f };   // depth in semitones on pitch
        float lfo1ToAmp01     { 0.0f };   // 0..1 amount on amp (tremolo)

        // LFO2 (key-triggered, per-voice) — routed only via the mod matrix
        Lfo::Shape lfo2Shape { Lfo::Shape::Triangle };
        float lfo2RateHz      { 2.0f };
        float lfo2FadeInSec   { 0.0f };

        // ENV3 (free) — routed only via the mod matrix
        float env3A { 0.01f }, env3D { 0.30f }, env3S { 0.5f }, env3R { 0.40f };

        // Mod matrix
        std::array<ModSlot, params::kNumModSlots> modSlots {};

        // Modulation sources from host / global engine
        float modWheel01       { 0.0f };  // 0..1
        float pitchBendSemis   { 0.0f };  // already scaled by bend range (fixed routing)
        float pitchBendNorm    { 0.0f };  // -1..+1 (matrix source)
        float aftertouch01     { 0.0f };  // 0..1 (matrix source)
        float lfo3Value        { 0.0f };  // -1..+1, global LFO (matrix source)

        // Performance / voicing
        float pan              { 0.0f };  // -1..+1 master pan offset applied per-voice
        float glideTime        { 0.0f };  // seconds; 0 = instant pitch
    };

    class Voice
    {
    public:
        void prepare(double sampleRate) noexcept;
        void reset() noexcept;

        void noteOn(int midiNote, float velocity01) noexcept;
        void noteOff(float velRelease01) noexcept;
        void killFast() noexcept;
        void setNote(int midiNote) noexcept;

        // Seed the glide origin: the next noteOn will glide from this pitch
        // (in fractional MIDI-note units). Call before noteOn/applyParams.
        void glideFrom(float fromNoteFloat) noexcept;

        // Unison contributors: caller sets per-voice detune cents and pan
        // before noteOn. Two voices in a unison group will have opposite
        // signs on these so the pair spreads symmetrically.
        void setUnisonOffsets(float centsOffset, float panOffset) noexcept;

        void applyParams(const VoiceParams& p) noexcept;
        void renderAdd(float* outL, float* outR, int startSample, int numSamples) noexcept;

        [[nodiscard]] bool isPlaying()   const noexcept { return amp.isActive(); }
        [[nodiscard]] bool isInRelease() const noexcept { return amp.isInRelease(); }
        [[nodiscard]] float envLevel()   const noexcept { return amp.getLevel(); }
        [[nodiscard]] int  getMidiNote() const noexcept { return midiNote; }
        [[nodiscard]] juce::int64 getStartStamp() const noexcept { return startStamp; }

        void setStartStamp(juce::int64 s) noexcept { startStamp = s; }

    private:
        void recomputeOscFrequencies() noexcept;
        static void applyFilterMode(params::FilterMode m, SvfTpt& f, MoogLadder& l, bool& useL) noexcept;

        // DSP graph
        Oscillator osc1, osc2, subOsc;
        HyperSaw   hyper1, hyper2;   // engaged when waveform == HyperSaw
        WhiteNoise whiteNoise;
        PinkNoise  pinkNoise;
        SvfTpt     filter;          // Cytomic SVF (filter 1)
        MoogLadder ladder;          // Huovilainen ladder (filter 1)
        bool       useLadder { false };
        SvfTpt     filter2;         // filter 2
        MoogLadder ladder2;
        bool       useLadder2 { false };
        Saturator  saturator;       // between filters
        params::FilterRouting routing { params::FilterRouting::Single };
        AdsrLinear amp;
        AdsrLinear fEnv;
        AdsrLinear env3;
        Lfo        lfo1;
        Lfo        lfo2;
        DcBlocker  dcBlock;

        // Effective per-sample source levels (base ± matrix modulation).
        float osc1Level    { 0.8f };
        float osc2Level    { 0.0f };
        float subLevel     { 0.0f };
        float noiseLevel   { 0.0f };
        params::Waveform   osc1Wave { params::Waveform::Saw };
        params::Waveform   osc2Wave { params::Waveform::Saw };
        params::SubWaveform subWave { params::SubWaveform::Sine };
        int                subOctOffset { -12 };  // semitones from base
        params::NoiseType  noiseType { params::NoiseType::White };

        // Base values (from params); matrix modulates around these.
        float osc1LevelBase  { 0.8f }, osc2LevelBase { 0.0f };
        float subLevelBase   { 0.0f }, noiseLevelBase { 0.0f };
        float osc1PWBase     { 0.5f }, osc2PWBase { 0.5f };
        float filterRes01Base{ 0.1f }, filterDrive01Base { 0.0f };
        float panBase        { 0.0f };
        float lfo1RateBase   { 4.0f }, lfo2RateBase { 2.0f };

        // Filter modulation
        float filterCutoffBase { 12000.0f };
        float filterKeytrack   { 0.0f };
        float filterEnvAmount  { 0.0f };
        float filterVelAmount  { 0.0f };
        float filterDrive01    { 0.0f };
        float filter2CutoffBase{ 8000.0f };
        float filter2Res01Base { 0.1f };

        // LFO1 mod depths (dedicated routings; matrix adds on top)
        float lfo1ToCutoffOct { 0.0f };
        float lfo1ToPitchSemi { 0.0f };
        float lfo1ToAmp01     { 0.0f };
        float modWheel01      { 0.0f };

        // Matrix state
        std::array<ModSlot, params::kNumModSlots> modSlots {};
        float pitchBendNorm   { 0.0f };
        float aftertouch01    { 0.0f };
        float lfo3Value       { 0.0f };
        float randomValue     { 0.0f };   // per-note random, set on noteOn
        float matrixAmp       { 1.0f };   // amp multiplier from matrix (control-rate)

        // Pitch
        int   midiNote        { -1 };
        float velocity        { 0.0f };
        float baseHz          { 440.0f };
        float pitchBendSemis  { 0.0f };
        float osc1OffsetSemis { 0.0f };
        float osc2OffsetSemis { 0.0f };

        // Glide: currentNote chases targetNote at control rate. baseHz is
        // derived from currentNote so glide and pitch-bend compose cleanly.
        float currentNote     { 60.0f };
        float targetNote      { 60.0f };
        float glideCoef       { 1.0f };   // per control-step; 1 = instant
        float pitchModSemis   { 0.0f };   // LFO1+matrix → pitch (both oscs)
        float osc2ModSemis    { 0.0f };   // matrix → OSC2 pitch only

        // Unison
        float unisonCents { 0.0f };
        float unisonPan   { 0.0f };

        // Per-voice pan (equal-power computed each block from master + unison)
        float panLeft  { 0.7071f };
        float panRight { 0.7071f };

        float sampleRate { 48000.0f };

        static constexpr int kControlUpdateInterval = 16;
        int controlUpdateCounter { 0 };

        juce::int64 startStamp { 0 };
    };
}
