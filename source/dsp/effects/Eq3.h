#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

namespace bacillum::dsp
{
    // 3-band parametric EQ: low shelf + peaking + high shelf.
    // RBJ Audio EQ Cookbook biquads, Transposed-Direct-Form-II, stereo.
    // Coefficients live on the stack (no heap) so it is safe to recompute
    // every block on the audio thread — unlike juce::dsp::IIR::Coefficients,
    // which allocates.
    //   https://www.w3.org/TR/audio-eq-cookbook/
    class Eq3
    {
    public:
        void prepare (double sampleRate) noexcept { sr = static_cast<float>(sampleRate); reset(); }

        void reset() noexcept
        {
            for (auto& s : low)  s = {};
            for (auto& s : peak) s = {};
            for (auto& s : high) s = {};
        }

        void setParams (float lowFreq, float lowGainDb,
                        float midFreq, float midGainDb, float midQ,
                        float highFreq, float highGainDb) noexcept
        {
            makeLowShelf  (lowCoef,  lowFreq,  lowGainDb);
            makePeak      (peakCoef, midFreq,  midGainDb, midQ);
            makeHighShelf (highCoef, highFreq, highGainDb);
        }

        void process (float* L, float* R, int n) noexcept
        {
            for (int i = 0; i < n; ++i)
            {
                L[i] = highCoef.run(peakCoef.run(lowCoef.run(L[i], low[0]), peak[0]), high[0]);
                R[i] = highCoef.run(peakCoef.run(lowCoef.run(R[i], low[1]), peak[1]), high[1]);
            }
        }

    private:
        struct Coef
        {
            float b0 { 1 }, b1 { 0 }, b2 { 0 }, a1 { 0 }, a2 { 0 };
            struct State { float z1 { 0 }, z2 { 0 }; };
            [[nodiscard]] float run (float x, State& s) const noexcept
            {
                const float y = b0 * x + s.z1;
                s.z1 = b1 * x - a1 * y + s.z2;
                s.z2 = b2 * x - a2 * y;
                return y;
            }
        };

        void normalise (Coef& c, float b0, float b1, float b2, float a0, float a1, float a2) noexcept
        {
            const float inv = 1.0f / a0;
            c.b0 = b0 * inv; c.b1 = b1 * inv; c.b2 = b2 * inv;
            c.a1 = a1 * inv; c.a2 = a2 * inv;
        }

        void makePeak (Coef& c, float f, float gainDb, float Q) noexcept
        {
            const float A  = std::pow (10.0f, gainDb / 40.0f);
            const float w0 = juce::MathConstants<float>::twoPi * juce::jlimit (10.0f, sr * 0.49f, f) / sr;
            const float cw = std::cos (w0), sw = std::sin (w0);
            const float alpha = sw / (2.0f * juce::jmax (0.05f, Q));
            normalise (c,
                       1.0f + alpha * A, -2.0f * cw, 1.0f - alpha * A,
                       1.0f + alpha / A, -2.0f * cw, 1.0f - alpha / A);
        }

        void makeLowShelf (Coef& c, float f, float gainDb) noexcept
        {
            const float A  = std::pow (10.0f, gainDb / 40.0f);
            const float w0 = juce::MathConstants<float>::twoPi * juce::jlimit (10.0f, sr * 0.49f, f) / sr;
            const float cw = std::cos (w0), sw = std::sin (w0);
            const float beta = std::sqrt (A) * sw * juce::MathConstants<float>::sqrt2;  // S=1
            normalise (c,
                       A * ((A + 1) - (A - 1) * cw + beta),
                       2 * A * ((A - 1) - (A + 1) * cw),
                       A * ((A + 1) - (A - 1) * cw - beta),
                       (A + 1) + (A - 1) * cw + beta,
                       -2 * ((A - 1) + (A + 1) * cw),
                       (A + 1) + (A - 1) * cw - beta);
        }

        void makeHighShelf (Coef& c, float f, float gainDb) noexcept
        {
            const float A  = std::pow (10.0f, gainDb / 40.0f);
            const float w0 = juce::MathConstants<float>::twoPi * juce::jlimit (10.0f, sr * 0.49f, f) / sr;
            const float cw = std::cos (w0), sw = std::sin (w0);
            const float beta = std::sqrt (A) * sw * juce::MathConstants<float>::sqrt2;  // S=1
            normalise (c,
                       A * ((A + 1) + (A - 1) * cw + beta),
                       -2 * A * ((A - 1) + (A + 1) * cw),
                       A * ((A + 1) + (A - 1) * cw - beta),
                       (A + 1) - (A - 1) * cw + beta,
                       2 * ((A - 1) - (A + 1) * cw),
                       (A + 1) - (A - 1) * cw - beta);
        }

        float sr { 48000.0f };
        Coef  lowCoef, peakCoef, highCoef;
        Coef::State low[2], peak[2], high[2];
    };
}
