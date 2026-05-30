#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace bacillum::dsp
{
    // Jon Dattorro plate reverb ("Effect Design Part 1: Reverberator and Other
    // Filters", JAES 1997). Input bandwidth LP → 4 cascaded input-diffusion
    // allpasses → a figure-8 tank (allpass + delay + damping per half, with
    // cross-coupled decay) → Dattorro's multi-tap stereo output network.
    //
    // Delay/allpass lengths are the paper's canonical values (sampled at
    // 29761 Hz) scaled to the running sample rate. Allpasses are static in this
    // revision (no tank excursion modulation yet — a later "shimmer" refinement);
    // the diffusion + damped figure-8 already give a lush, decorrelated plate.
    //   https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf
    class DattorroReverb
    {
    public:
        void prepare (double sampleRate)
        {
            sr = static_cast<float> (sampleRate);
            const float k = sr / 29761.0f;
            auto L = [k] (int n) { return juce::jmax (1, (int) std::round (n * k)); };

            preDelay.init (L (4096));
            inDiff[0].init (L (142), 0.75f);
            inDiff[1].init (L (107), 0.75f);
            inDiff[2].init (L (379), 0.625f);
            inDiff[3].init (L (277), 0.625f);

            apL1.init (L (672),  0.70f);
            delL1.init (L (4453));
            apL2.init (L (1800), 0.50f);
            delL2.init (L (3720));

            apR1.init (L (908),  0.70f);
            delR1.init (L (4217));
            apR2.init (L (2656), 0.50f);
            delR2.init (L (3163));

            scale = k;
            reset();
            setSize (0.6f); setDamping (0.4f); setWidth (1.0f); setMix (0.0f);
        }

        void reset() noexcept
        {
            preDelay.reset();
            for (auto& a : inDiff) a.reset();
            apL1.reset(); delL1.reset(); apL2.reset(); delL2.reset();
            apR1.reset(); delR1.reset(); apR2.reset(); delR2.reset();
            lpL = lpR = inLp = lastL = lastR = 0.0f;
        }

        void setMix     (float m01)  noexcept { mix = juce::jlimit (0.0f, 1.0f, m01); }
        void setSize    (float s01)  noexcept { decay = juce::jlimit (0.0f, 1.0f, s01) * 0.55f + 0.40f; } // 0.40..0.95
        void setDamping (float d01)  noexcept { dampA = 1.0f - juce::jlimit (0.0f, 1.0f, d01) * 0.9f; }   // 1=bright..0.1=dark
        void setWidth   (float w01)  noexcept { width = juce::jlimit (0.0f, 1.0f, w01); }

        void process (float* inOutL, float* inOutR, int numSamples) noexcept
        {
            if (mix < 0.0005f) return;
            const float dry = 1.0f - mix;

            for (int n = 0; n < numSamples; ++n)
            {
                const float dL = inOutL[n];
                const float dR = inOutR[n];

                // Input: pre-delay + bandwidth low-pass + 4 diffusers.
                float x = preDelay.process (0.5f * (dL + dR));
                inLp += kInputBw * (x - inLp);
                x = inLp;
                for (auto& a : inDiff) x = a.process (x);

                // Figure-8 tank (cross-coupled).
                float l = x + decay * lastR;
                l = apL1.process (l);
                l = delL1.process (l);
                lpL += dampA * (l - lpL);
                l = lpL * decay;
                l = apL2.process (l);
                lastL = delL2.process (l);

                float r = x + decay * lastL;
                r = apR1.process (r);
                r = delR1.process (r);
                lpR += dampA * (r - lpR);
                r = lpR * decay;
                r = apR2.process (r);
                lastR = delR2.process (r);

                // Dattorro output tap network (indices scaled to sr).
                const float yl = 0.6f * ( delR1.at (t (266))  + delR1.at (t (2974))
                                        - apR2.at (t (1913))  + delR2.at (t (1996))
                                        - delL1.at (t (1990)) - apL2.at (t (187))
                                        - delL2.at (t (1066)) );
                const float yr = 0.6f * ( delL1.at (t (353))  + delL1.at (t (3627))
                                        - apL2.at (t (1228))  + delL2.at (t (2673))
                                        - delR1.at (t (2111)) - apR2.at (t (335))
                                        - delR2.at (t (121)) );

                // Stereo width via mid/side.
                const float mid  = 0.5f * (yl + yr);
                const float side = 0.5f * (yl - yr) * width;
                const float wetL = mid + side;
                const float wetR = mid - side;

                inOutL[n] = dry * dL + mix * wetL;
                inOutR[n] = dry * dR + mix * wetR;
            }
        }

    private:
        [[nodiscard]] int t (int idx29761) const noexcept
        {
            return juce::jmax (1, (int) std::round (idx29761 * scale));
        }

        // Pure delay line (ring buffer).
        struct Delay
        {
            std::vector<float> buf;
            int size { 1 }, idx { 0 };
            void init (int n) { size = juce::jmax (1, n); buf.assign ((size_t) size, 0.0f); idx = 0; }
            void reset() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; }
            float process (float x) noexcept
            {
                const float out = buf[(size_t) idx];
                buf[(size_t) idx] = x;
                if (++idx >= size) idx = 0;
                return out;
            }
            [[nodiscard]] float at (int d) const noexcept
            {
                int i = idx - juce::jlimit (0, size - 1, d);
                if (i < 0) i += size;
                return buf[(size_t) i];
            }
        };

        // Schroeder allpass: w[n] = x + g·w[n-N];  y = -g·w[n] + w[n-N].
        struct AllPass
        {
            std::vector<float> buf;
            int size { 1 }, idx { 0 };
            float g { 0.5f };
            void init (int n, float gain) { size = juce::jmax (1, n); buf.assign ((size_t) size, 0.0f); idx = 0; g = gain; }
            void reset() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; }
            float process (float x) noexcept
            {
                const float wN = buf[(size_t) idx];
                const float w  = x + g * wN;
                const float y  = -g * w + wN;
                buf[(size_t) idx] = w;
                if (++idx >= size) idx = 0;
                return y;
            }
            [[nodiscard]] float at (int d) const noexcept
            {
                int i = idx - juce::jlimit (0, size - 1, d);
                if (i < 0) i += size;
                return buf[(size_t) i];
            }
        };

        static constexpr float kInputBw = 0.72f;   // input bandwidth LP coefficient

        float sr { 48000.0f };
        float scale { 1.0f };
        float mix { 0.0f }, decay { 0.7f }, dampA { 0.5f }, width { 1.0f };

        Delay   preDelay;
        AllPass inDiff[4];
        AllPass apL1, apL2, apR1, apR2;
        Delay   delL1, delL2, delR1, delR2;
        float   lpL { 0 }, lpR { 0 }, inLp { 0 }, lastL { 0 }, lastR { 0 };
    };
}
