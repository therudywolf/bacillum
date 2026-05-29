#pragma once

#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>

namespace bacillum::dsp
{
    // Huovilainen non-linear Moog ladder (DAFx-04, "Non-Linear Digital
    // Implementation of the Moog Ladder Filter"). This is the widely-used
    // "thermal" formulation: each of the four one-pole sections is driven
    // through a tanh saturator, with a polynomial tuning correction so the
    // cutoff stays accurate as resonance rises.
    //
    // 2x oversampling is built in (the spec mandates it: tanh + 4x feedback
    // is a strong alias source). We upsample with zero-order hold and average
    // the two sub-steps on the way down — cheap, and enough to keep the
    // self-oscillation clean for a VA voice.
    //
    // Reference: Huovilainen 2004 — https://www.dafx.de/paper-archive/2004/P_061.PDF
    //            (and Pirkle, "Designing Software Synthesizer Plugins", ch. 7).
    class MoogLadder
    {
    public:
        void prepare (double sampleRate) noexcept
        {
            sr = static_cast<float> (sampleRate);
            reset();
            setCutoff (1000.0f);
            setResonance01 (0.1f);
        }

        void reset() noexcept
        {
            for (auto& s : stage)     s = 0.0f;
            for (auto& s : stageTanh) s = 0.0f;
            for (auto& d : delay)     d = 0.0f;
        }

        void setFourPole (bool four) noexcept { fourPole = four; }

        void setCutoff (float hz) noexcept
        {
            const float clamped = juce::jlimit (20.0f, sr * 0.49f, hz);
            if (clamped == fc) return;
            fc = clamped;
            updateTuning();
        }

        // 0..1 → resonance up to self-oscillation (~k = 4).
        void setResonance01 (float r01) noexcept
        {
            res = juce::jlimit (0.0f, 1.0f, r01);
            updateTuning();
        }

        [[nodiscard]] float process (float in) noexcept
        {
            // 2x oversample: run the kernel twice on the same input (ZOH),
            // average the chosen tap.
            float acc = 0.0f;
            for (int os = 0; os < 2; ++os)
                acc += kernel (in);
            return acc * 0.5f;
        }

    private:
        void updateTuning() noexcept
        {
            // Normalised cutoff at the *oversampled* rate (so /2).
            const float fcNorm = (fc / sr) * 0.5f;
            const float f  = fcNorm;
            const float f2 = f * f;

            // Huovilainen tuning polynomials.
            const float fcr = 1.8730f * f2 * f + 0.4955f * f2 - 0.6490f * f + 0.9988f;
            const float acr = -3.9364f * f2 + 1.8409f * f + 0.9968f;

            tune    = (1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * f * fcr)) / kThermal;
            resQuad = 4.0f * res * acr;
        }

        [[nodiscard]] float kernel (float input) noexcept
        {
            // Resonant feedback from the last stage.
            float x = input - resQuad * delay[5];

            // Stage 0
            stage[0] = delay[0] + tune * (std::tanh (x * kThermal) - stageTanh[0]);
            delay[0] = stage[0];

            // Stages 1..3
            for (int k = 1; k < 4; ++k)
            {
                x = stage[k - 1];
                stageTanh[k - 1] = std::tanh (x * kThermal);
                stage[k] = delay[k]
                         + tune * (stageTanh[k - 1]
                                   - (k != 3 ? stageTanh[k]
                                             : std::tanh (delay[k] * kThermal)));
                delay[k] = stage[k];
            }

            // Half-sample delay for phase compensation (used in feedback).
            delay[5] = (stage[3] + delay[4]) * 0.5f;
            delay[4] = stage[3];

            // LP24 = 4-pole output; LP12 ≈ 2-pole tap.
            return fourPole ? delay[5] : stage[1];
        }

        static constexpr float kThermal = 0.000025f;

        float sr       { 48000.0f };
        float fc       { 1000.0f };
        float res      { 0.1f };
        float tune     { 0.0f };
        float resQuad  { 0.0f };
        bool  fourPole { true };

        float stage[4]     { 0, 0, 0, 0 };
        float stageTanh[3] { 0, 0, 0 };
        float delay[6]     { 0, 0, 0, 0, 0, 0 };
    };
}
