#pragma once

#include "dsp/voice/Voice.h"
#include "params/Params.h"

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>

namespace bacillum::dsp
{
    // Custom voice manager. NOT juce::Synthesiser — we want sub-block MIDI
    // dispatch, our own stealing priority and full control of the per-voice
    // graph. Spec §2.3, §5.6.
    class VoiceManager
    {
    public:
        static constexpr int kMaxVoices = 16;

        void prepare(double sampleRate) noexcept;
        void reset() noexcept;

        void setParams(const VoiceParams& p) noexcept;
        void setPolyMode(params::PolyMode mode) noexcept { polyMode = mode; }
        void setUnison(int count, float detuneCents, float spread01) noexcept;

        void noteOn(int midiNote, float velocity01) noexcept;
        void noteOff(int midiNote, float velRelease01) noexcept;
        void sustainPedal(bool down) noexcept;
        void allNotesOff(bool fast) noexcept;
        void panic() noexcept;

        void render(float* outL, float* outR, int startSample, int numSamples) noexcept;

    private:
        Voice* findVoiceForNoteOn(int midiNote, bool allowSameNoteRetrigger) noexcept;
        void   triggerOneVoice(int midiNote, float vel, float centsOffset, float panOffset) noexcept;

        std::array<Voice, kMaxVoices> voices;

        VoiceParams       params;
        params::PolyMode  polyMode { params::PolyMode::Poly };

        // Unison
        int   unisonCount   { 1 };       // 1..8
        float unisonDetune  { 7.0f };    // cents
        float unisonSpread  { 0.5f };    // 0..1

        bool sustainDown { false };
        std::array<bool, 128> sustainedNote { {} };

        juce::int64 ageCounter { 0 };
    };
}
