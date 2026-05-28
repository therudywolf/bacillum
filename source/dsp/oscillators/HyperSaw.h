#pragma once

#include "Polyblep.h"
#include <juce_core/juce_core.h>
#include <cmath>
#include <array>

namespace bacillum::dsp
{
    // 7-voice "supersaw" / hypersaw oscillator. Detune & mix curves are
    // derived from Adam Szabo's reverse-engineering of the Roland JP-8000
    // SuperSaw ("How to Emulate the Super Saw", KVR / 2009).
    //
    // Two control parameters:
    //   - detune  ∈ [0..1]  : amount of pitch spread between the 7 saws
    //   - mix     ∈ [0..1]  : centre saw amount vs. side saws (timbre control)
    //
    // Output is normalised so that detune=0 sounds the same loudness as detune=1.
    class HyperSaw
    {
    public:
        void prepare (double sampleRate) noexcept
        {
            sr = static_cast<float> (sampleRate);
            invSr = 1.0f / sr;
            reset();
        }

        void reset() noexcept
        {
            // Stagger initial phases so we don't start mono-coherent.
            for (int i = 0; i < kN; ++i)
                phase[i] = static_cast<float> (i) / static_cast<float> (kN);
        }

        void setFrequency (float hz)    noexcept { f0 = hz; }
        void setDetune    (float d01)   noexcept { detune = juce::jlimit (0.0f, 1.0f, d01); }
        void setMix       (float m01)   noexcept { mix    = juce::jlimit (0.0f, 1.0f, m01); }

        [[nodiscard]] float tick() noexcept
        {
            // Szabo curves (eq. 4 & 5 of the paper).
            //   detuneCurve(x) = 10028.7*x^11 - 50221.6*x^10 + 104822*x^9 - ... (polyfit)
            // For simplicity and similar sound we use the more compact exponential
            // approximation that ships in most implementations.
            const float spread = detuneCurve (detune);

            // Side / centre mix levels.
            const float centreLvl = -0.55366f * mix + 0.99785f;        // ≈ paper
            const float sideLvl   = -0.73764f * mix * mix + 1.2841f * mix + 0.044372f;

            float out = 0.0f;
            for (int i = 0; i < kN; ++i)
            {
                const float fi = f0 * std::pow (2.0f, kCents[i] * spread / 1200.0f);
                const float dt = juce::jlimit (0.0f, 0.49f, fi * invSr);

                // PolyBLEP saw
                const float naive = 2.0f * phase[i] - 1.0f;
                const float v     = naive - polyBlep (phase[i], dt);

                phase[i] += dt;
                if (phase[i] >= 1.0f) phase[i] -= 1.0f;

                out += v * (i == kCentreIdx ? centreLvl : sideLvl);
            }

            // 7-saw bank normalisation: empirical 0.27 keeps peak around full saw.
            return out * 0.27f;
        }

    private:
        // Szabo's polynomial detune curve, simplified to a smooth scaler.
        static float detuneCurve (float d) noexcept
        {
            // Smooth quartic that goes from 0 to ~1 with a knee shape similar
            // to fig. 6 of the paper.
            return d * (1.0f + d * (0.18f + d * 0.4f));
        }

        static constexpr int kN = 7;
        static constexpr int kCentreIdx = 3;

        // Roughly symmetric detune in cents. Side voices are far enough to
        // beat audibly, centre is in tune.
        static constexpr std::array<float, kN> kCents
            { -32.0f, -22.0f, -11.0f, 0.0f, 11.0f, 22.0f, 32.0f };

        std::array<float, kN> phase {};
        float sr     { 48000.0f };
        float invSr  { 1.0f / 48000.0f };
        float f0     { 440.0f };
        float detune { 0.5f };
        float mix    { 0.6f };
    };
}
