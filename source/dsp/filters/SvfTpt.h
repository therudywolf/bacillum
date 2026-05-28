#pragma once

#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>

namespace bacillum::dsp
{
    // Single-stage Topology-Preserving-Transform State Variable Filter.
    // Reference: Andy Simper / Cytomic
    //   https://cytomic.com/files/dsp/SvfLinearTrapezoidal.pdf
    // Spec §5.2.
    //
    // Coefficients are computed lazily — we cache g/k/a1/a2/a3 and only
    // recompute when cutoff or Q actually change. Inside the per-sample loop
    // it's just six MACs.
    class SvfTpt
    {
    public:
        enum class Mode { LP12, HP, BP, Notch, Peak, LP24 };

        void prepare(double sampleRate) noexcept
        {
            sr = static_cast<float>(sampleRate);
            invSr = 1.0f / sr;
            reset();
            setCutoff(1000.0f);
            setQ(0.707f);
        }

        void reset() noexcept
        {
            ic1eq = ic2eq = 0.0f;
            ic1eq2 = ic2eq2 = 0.0f;
        }

        void setCutoff(float hz) noexcept
        {
            // Clamp below Nyquist with margin to keep tan() well-behaved.
            const float maxHz = sr * 0.49f;
            const float clamped = juce::jlimit(20.0f, maxHz, hz);
            if (clamped != fc) { fc = clamped; coeffsDirty = true; }
        }

        void setQ(float q) noexcept
        {
            // Q=0.5 → flat, Q→inf → self-oscillation. Clamp so 1/Q doesn't blow.
            const float clamped = juce::jlimit(0.05f, 30.0f, q);
            if (clamped != q_) { q_ = clamped; coeffsDirty = true; }
        }

        // Resonance 0..1 mapped to Q ~0.5..15. Self-oscillates near 1.
        void setResonance01(float r01) noexcept
        {
            const float r = juce::jlimit(0.0f, 1.0f, r01);
            setQ(0.5f + r * r * 14.5f);
        }

        void setMode(Mode m) noexcept { mode = m; }

        [[nodiscard]] float process(float in) noexcept
        {
            if (coeffsDirty) updateCoeffs();

            float lp, bp, hp, notch;
            tick(in, ic1eq, ic2eq, lp, bp, hp, notch);

            switch (mode)
            {
                case Mode::LP12:  return lp;
                case Mode::HP:    return hp;
                case Mode::BP:    return bp;
                case Mode::Notch: return notch;
                case Mode::Peak:  return lp - hp;
                case Mode::LP24:
                {
                    float lp2, bp2, hp2, notch2;
                    tick(lp, ic1eq2, ic2eq2, lp2, bp2, hp2, notch2);
                    return lp2;
                }
            }
            return lp;
        }

    private:
        void updateCoeffs() noexcept
        {
            g  = std::tan(juce::MathConstants<float>::pi * fc * invSr);
            k  = 1.0f / q_;
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
            coeffsDirty = false;
        }

        // The canonical Cytomic SVF tick. Two integrators, four outputs.
        void tick(float in, float& s1, float& s2,
                  float& lp, float& bp, float& hp, float& notch) const noexcept
        {
            const float v3 = in - s2;
            const float v1 = a1 * s1 + a2 * v3;
            const float v2 = s2 + a2 * s1 + a3 * v3;
            s1 = 2.0f * v1 - s1;
            s2 = 2.0f * v2 - s2;

            lp    = v2;
            bp    = v1;
            hp    = in - k * v1 - v2;
            notch = in - k * v1;
        }

        float sr    { 48000.0f };
        float invSr { 1.0f / 48000.0f };
        float fc    { 1000.0f };
        float q_    { 0.707f };

        float g { 0.0f }, k { 0.0f }, a1 { 0.0f }, a2 { 0.0f }, a3 { 0.0f };
        bool  coeffsDirty { true };

        // Two SVF states for cascaded LP24.
        float ic1eq { 0.0f }, ic2eq { 0.0f };
        float ic1eq2 { 0.0f }, ic2eq2 { 0.0f };

        Mode mode { Mode::LP12 };
    };
}
