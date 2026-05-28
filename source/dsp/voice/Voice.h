#pragma once

#include "dsp/oscillators/Oscillator.h"
#include "dsp/oscillators/HyperSaw.h"
#include "dsp/oscillators/Noise.h"
#include "dsp/filters/SvfTpt.h"
#include "dsp/envelopes/Adsr.h"
#include "dsp/lfo/Lfo.h"
#include "dsp/DcBlocker.h"
#include "params/Params.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace bacillum::dsp
{
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

        // Filter
        params::FilterMode filterMode { params::FilterMode::LP12 };
        float filterCutoff    { 12000.0f };
        float filterRes01     { 0.1f };
        float filterDrive01   { 0.0f };
        float filterKeytrack  { 0.0f };
        float filterEnvAmount { 0.0f };
        float filterVelAmount { 0.0f };

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

        // Modulation from host
        float modWheel01       { 0.0f };  // 0..1 — scales LFO1 depth (fixed routing)
        float pitchBendSemis   { 0.0f };

        // Performance / voicing
        float pan              { 0.0f };  // -1..+1 master pan offset applied per-voice
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
        void applyFilterMode(params::FilterMode m) noexcept;

        // DSP graph
        Oscillator osc1, osc2, subOsc;
        HyperSaw   hyper1, hyper2;   // engaged when waveform == HyperSaw
        WhiteNoise whiteNoise;
        PinkNoise  pinkNoise;
        SvfTpt     filter;
        AdsrLinear amp;
        AdsrLinear fEnv;
        Lfo        lfo1;
        DcBlocker  dcBlock;

        // Source levels (cached from params)
        float osc1Level    { 0.8f };
        float osc2Level    { 0.0f };
        float subLevel     { 0.0f };
        float noiseLevel   { 0.0f };
        params::Waveform   osc1Wave { params::Waveform::Saw };
        params::Waveform   osc2Wave { params::Waveform::Saw };
        params::SubWaveform subWave { params::SubWaveform::Sine };
        int                subOctOffset { -12 };  // semitones from base
        params::NoiseType  noiseType { params::NoiseType::White };

        // Filter modulation
        float filterCutoffBase { 12000.0f };
        float filterKeytrack   { 0.0f };
        float filterEnvAmount  { 0.0f };
        float filterVelAmount  { 0.0f };
        float filterDrive01    { 0.0f };

        // LFO1 mod depths (already scaled by modwheel before reaching here? no — we read modWheel here)
        float lfo1ToCutoffOct { 0.0f };
        float lfo1ToPitchSemi { 0.0f };
        float lfo1ToAmp01     { 0.0f };
        float modWheel01      { 0.0f };

        // Pitch
        int   midiNote        { -1 };
        float velocity        { 0.0f };
        float baseHz          { 440.0f };
        float pitchBendSemis  { 0.0f };
        float osc1OffsetSemis { 0.0f };
        float osc2OffsetSemis { 0.0f };

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
