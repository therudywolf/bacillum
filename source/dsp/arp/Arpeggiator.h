#pragma once

#include "dsp/oscillators/Noise.h"
#include "params/Params.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <algorithm>

namespace bacillum::dsp
{
    // MIDI-domain arpeggiator. It sits between incoming MIDI and the voice
    // manager: held notes are collected, and a tempo-locked clock emits
    // note-on/off into a MidiBuffer that the normal render path consumes.
    // This keeps the synth engine unaware of the arp (clean separation), and
    // keeps everything sample-accurate because we add events at sample offsets.
    class Arpeggiator
    {
    public:
        using Mode = params::ArpMode;

        void prepare (double sampleRate) noexcept
        {
            sr = static_cast<float> (sampleRate);
            reset();
        }

        void reset() noexcept
        {
            heldCount = 0;
            patternLen = 0;
            stepIndex = 0;
            stepCountdown = 0;
            gateCountdown = 0;
            gateActive = false;
            activeNote = -1;
            dirty = true;
            rng.reset (0x51ED2700u);
        }

        void setEnabled (bool e) noexcept { enabled = e; }
        [[nodiscard]] bool isEnabled() const noexcept { return enabled; }

        void setParams (Mode m, float stepBeats, int oct, float gate01, float bpm) noexcept
        {
            if (m != mode || oct != octaves) dirty = true;
            mode      = m;
            octaves   = juce::jlimit (1, 4, oct);
            gate      = juce::jlimit (0.05f, 1.0f, gate01);
            beats     = juce::jmax (0.0001f, stepBeats);
            currentBpm = juce::jmax (1.0f, bpm);
        }

        // --- Held-note bookkeeping (called from MIDI handling) -------------
        void noteOn (int note, float vel) noexcept
        {
            for (int i = 0; i < heldCount; ++i)
                if (held[(size_t) i].note == note) { held[(size_t) i].vel = vel; return; }

            if (heldCount < kMaxHeld)
                held[(size_t) heldCount++] = { note, vel };
            dirty = true;
        }

        void noteOff (int note) noexcept
        {
            for (int i = 0; i < heldCount; ++i)
            {
                if (held[(size_t) i].note == note)
                {
                    for (int j = i; j < heldCount - 1; ++j)
                        held[(size_t) j] = held[(size_t) (j + 1)];
                    --heldCount;
                    dirty = true;
                    return;
                }
            }
        }

        void allNotesOff() noexcept
        {
            heldCount = 0;
            dirty = true;
        }

        // --- Block generation ---------------------------------------------
        void process (juce::MidiBuffer& midi, int numSamples) noexcept
        {
            // Disabled: flush any sounding arp note exactly once.
            if (! enabled)
            {
                if (activeNote >= 0)
                {
                    midi.addEvent (juce::MidiMessage::noteOff (1, activeNote), 0);
                    activeNote = -1;
                    gateActive = false;
                }
                return;
            }

            if (heldCount == 0)
            {
                if (activeNote >= 0)
                {
                    midi.addEvent (juce::MidiMessage::noteOff (1, activeNote), 0);
                    activeNote = -1;
                    gateActive = false;
                }
                stepCountdown = 0;   // next held note fires immediately
                return;
            }

            if (dirty) rebuildPattern();
            if (patternLen == 0) return;

            const int stepSamples = juce::jmax (1,
                (int) (beats * (60.0f / currentBpm) * sr));

            for (int i = 0; i < numSamples; ++i)
            {
                if (stepCountdown <= 0)
                {
                    // End previous note.
                    if (activeNote >= 0)
                    {
                        midi.addEvent (juce::MidiMessage::noteOff (1, activeNote), i);
                        activeNote = -1;
                        gateActive = false;
                    }

                    // Pick step.
                    int idx;
                    if (mode == Mode::Random)
                        idx = randomIndex();
                    else
                    {
                        idx = stepIndex;
                        stepIndex = (stepIndex + 1) % patternLen;
                    }

                    const auto& s = pattern[(size_t) idx];
                    midi.addEvent (juce::MidiMessage::noteOn (1, s.note, s.vel), i);
                    activeNote   = s.note;
                    gateActive   = true;
                    gateCountdown = juce::jmax (1, (int) (stepSamples * gate));

                    stepCountdown += stepSamples;
                }

                if (gateActive)
                {
                    if (gateCountdown <= 0)
                    {
                        if (activeNote >= 0)
                            midi.addEvent (juce::MidiMessage::noteOff (1, activeNote), i);
                        activeNote = -1;
                        gateActive = false;
                    }
                    else
                    {
                        --gateCountdown;
                    }
                }

                --stepCountdown;
            }
        }

    private:
        struct Held { int note; float vel; };
        struct Step { int note; float vel; };

        int randomIndex() noexcept
        {
            // rng.tick() is [-1,1); fold to [0,patternLen).
            const float u = rng.tick() * 0.5f + 0.5f;
            return juce::jlimit (0, patternLen - 1, (int) (u * (float) patternLen));
        }

        void rebuildPattern() noexcept
        {
            dirty = false;
            patternLen = 0;
            if (heldCount == 0) return;

            // Base ordering of the held notes.
            std::array<Held, kMaxHeld> base = held;
            int baseLen = heldCount;

            if (mode == Mode::Up || mode == Mode::UpDown || mode == Mode::Random)
                std::sort (base.begin(), base.begin() + baseLen,
                           [](const Held& a, const Held& b) { return a.note < b.note; });
            else if (mode == Mode::Down)
                std::sort (base.begin(), base.begin() + baseLen,
                           [](const Held& a, const Held& b) { return a.note > b.note; });
            // AsPlayed: keep insertion order.

            // Expand octaves (ascending octave stacking).
            auto push = [&](int note, float vel)
            {
                if (patternLen < kMaxPattern)
                    pattern[(size_t) patternLen++] = { juce::jlimit (0, 127, note), vel };
            };

            for (int o = 0; o < octaves; ++o)
                for (int i = 0; i < baseLen; ++i)
                    push (base[(size_t) i].note + 12 * o, base[(size_t) i].vel);

            // UpDown: append the descending middle (exclude endpoints) to bounce.
            if (mode == Mode::UpDown && patternLen > 2)
            {
                const int top = patternLen;
                for (int i = top - 2; i >= 1; --i)
                    push (pattern[(size_t) i].note, pattern[(size_t) i].vel);
            }

            if (stepIndex >= patternLen) stepIndex = 0;
        }

        static constexpr int kMaxHeld    = 16;
        static constexpr int kMaxPattern = 128;

        std::array<Held, kMaxHeld>     held {};
        std::array<Step, kMaxPattern>  pattern {};
        int heldCount  { 0 };
        int patternLen { 0 };
        int stepIndex  { 0 };

        int  stepCountdown { 0 };
        int  gateCountdown { 0 };
        bool gateActive    { false };
        int  activeNote    { -1 };
        bool dirty         { true };

        Mode  mode    { Mode::Up };
        int   octaves { 1 };
        float gate    { 0.6f };
        float beats   { 0.5f };
        float currentBpm { 120.0f };
        float sr      { 48000.0f };

        WhiteNoise rng { 0x51ED2700u };

        bool enabled { false };
    };
}
