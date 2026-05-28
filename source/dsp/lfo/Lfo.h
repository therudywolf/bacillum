#pragma once

#include "dsp/oscillators/Noise.h"
#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>

namespace bacillum::dsp
{
    // Low-frequency oscillator: 8 shapes, optional fade-in, key-triggerable.
    // Spec §2.1 / §2.2. Sine/Tri/Saw/Square are PolyBLEP-free (sub-audio rate).
    class Lfo
    {
    public:
        enum class Shape : int
        {
            Sine = 0, Triangle, SawUp, SawDown, Square, PWM, SampleHold, SmoothRandom,
            NumShapes
        };

        void prepare(double sampleRate) noexcept
        {
            sr = static_cast<float>(sampleRate);
            invSr = 1.0f / sr;
            reset();
        }

        void reset() noexcept
        {
            phase = phaseOffset;
            shVal = nextShVal = 0.0f;
            currRand = nextRand = 0.0f;
            cyclesSinceTrigger = 0.0f;
            fadeEnvelope = 0.0f;
            // Seed both noise streams differently.
            rng.reset(0xA5A5A5A5u);
            // Prime first S&H value so we don't start on 0.
            nextShVal = rng.tick();
        }

        void setRateHz   (float hz)         noexcept { rate = std::max(0.0001f, hz); }
        void setShape    (Shape s)          noexcept { shape = s; }
        void setPulseWidth(float pw01)      noexcept { pw = juce::jlimit(0.05f, 0.95f, pw01); }
        void setPhaseOffset01(float p)      noexcept { phaseOffset = p - std::floor(p); }
        void setFadeInSec(float s)          noexcept { fadeInSec = std::max(0.0f, s); }

        // Key trigger: reset phase to user-defined offset; restart fade-in.
        void retrigger() noexcept
        {
            phase = phaseOffset;
            cyclesSinceTrigger = 0.0f;
            fadeEnvelope = 0.0f;
            // Re-seed S&H so each new note can pick a different value.
            nextShVal = rng.tick();
        }

        // [-1, +1] LFO sample.
        [[nodiscard]] float tick() noexcept
        {
            const float dt = rate * invSr;

            float v = 0.0f;
            switch (shape)
            {
                case Shape::Sine:
                    v = std::sin(phase * juce::MathConstants<float>::twoPi);
                    break;
                case Shape::Triangle:
                    v = (phase < 0.5f) ? (4.0f * phase - 1.0f)
                                       : (3.0f - 4.0f * phase);
                    break;
                case Shape::SawUp:
                    v = 2.0f * phase - 1.0f;
                    break;
                case Shape::SawDown:
                    v = 1.0f - 2.0f * phase;
                    break;
                case Shape::Square:
                    v = (phase < 0.5f) ? 1.0f : -1.0f;
                    break;
                case Shape::PWM:
                    v = (phase < pw) ? 1.0f : -1.0f;
                    break;
                case Shape::SampleHold:
                    v = shVal;
                    break;
                case Shape::SmoothRandom:
                {
                    // Linear interp between currRand and nextRand over one cycle.
                    v = currRand + (nextRand - currRand) * phase;
                    break;
                }
                default: break;
            }

            // Phase advance + wrap. Generate new S&H / random target on cycle.
            phase += dt;
            if (phase >= 1.0f)
            {
                phase -= 1.0f;
                shVal     = nextShVal;
                nextShVal = rng.tick();
                currRand  = nextRand;
                nextRand  = rng.tick();
                cyclesSinceTrigger += 1.0f;
            }

            // Fade-in attenuator (key-triggered LFOs only; for free-running
            // it stays at 1.0 because setFadeInSec(0) keeps fadeEnvelope=1).
            if (fadeInSec > 0.0f && fadeEnvelope < 1.0f)
            {
                fadeEnvelope += invSr / fadeInSec;
                if (fadeEnvelope > 1.0f) fadeEnvelope = 1.0f;
                v *= fadeEnvelope;
            }

            return v;
        }

    private:
        float sr    { 48000.0f };
        float invSr { 1.0f / 48000.0f };
        float rate  { 1.0f };
        float phase { 0.0f };
        float phaseOffset { 0.0f };
        float pw    { 0.5f };

        // Sample & hold + smooth-random state
        WhiteNoise rng { 0xA5A5A5A5u };
        float shVal     { 0.0f };
        float nextShVal { 0.0f };
        float currRand  { 0.0f };
        float nextRand  { 0.0f };

        // Fade-in
        float fadeInSec    { 0.0f };
        float fadeEnvelope { 1.0f };   // 1 = full; 0 = silent
        float cyclesSinceTrigger { 0.0f };

        Shape shape { Shape::Sine };
    };
}
