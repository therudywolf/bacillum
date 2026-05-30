#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <cmath>

namespace bacillum::dsp
{
    // Mip-mapped wavetable set, generated once and shared by all voices
    // (read-only at audio time). Eight morph frames sweep sine → triangle →
    // saw → square → pulse → organ → formant → bright. Each frame has a mip
    // pyramid: band-limited copies with fewer harmonics for higher octaves,
    // so playback never aliases regardless of pitch.
    //
    // Anti-aliasing strategy follows the classic "one band-limited table per
    // octave" approach (Välimäki & Huovilainen 2007 survey). Within a table we
    // interpolate linearly — at 2048 points per cycle that is well below the
    // audible error floor once mip selection has removed the aliasing harmonics.
    struct WavetableSet
    {
        static constexpr int kFrames    = 8;
        static constexpr int kTableSize = 2048;
        static constexpr int kNumMips   = 11;   // ~11 octaves of coverage

        // [frame][mip][sample(+1 guard for wrap-free linear interp)]
        std::array<std::array<std::array<float, kTableSize + 1>, kNumMips>, kFrames> tables {};

        // Harmonic amplitude for a frame's base spectrum (h is 1-based).
        static float harmonicAmp (int frame, int h) noexcept
        {
            const float pi = juce::MathConstants<float>::pi;
            switch (frame)
            {
                case 0: return (h == 1) ? 1.0f : 0.0f;                          // sine
                case 1: return (h % 2 == 1)                                     // triangle
                            ? ((((h - 1) / 2) % 2 == 0) ? 1.0f : -1.0f) / (float) (h * h)
                            : 0.0f;
                case 2: return 1.0f / (float) h;                                // saw
                case 3: return (h % 2 == 1) ? 1.0f / (float) h : 0.0f;          // square
                case 4: return (2.0f / (h * pi)) * std::sin (h * pi * 0.25f);   // 25% pulse
                case 5: { int p = h; bool pow2 = (p & (p - 1)) == 0;            // organ (octaves)
                          return pow2 ? 1.0f / (float) h : 0.0f; }
                case 6: { const float c = 8.0f;                                 // formant-ish
                          return std::exp (-((h - c) * (h - c)) / 18.0f) + (h == 1 ? 0.4f : 0.0f); }
                case 7: return 1.0f / std::sqrt ((float) h);                    // bright
                default: return (h == 1) ? 1.0f : 0.0f;
            }
        }

        void generate() noexcept
        {
            const float twoPi = juce::MathConstants<float>::twoPi;
            for (int f = 0; f < kFrames; ++f)
            {
                for (int m = 0; m < kNumMips; ++m)
                {
                    const int maxHarm = juce::jmax (1, (kTableSize / 2) >> m);
                    auto& tbl = tables[(size_t) f][(size_t) m];

                    float peak = 0.0f;
                    for (int s = 0; s < kTableSize; ++s)
                    {
                        float acc = 0.0f;
                        const float ph = twoPi * (float) s / (float) kTableSize;
                        for (int h = 1; h <= maxHarm; ++h)
                        {
                            const float a = harmonicAmp (f, h);
                            if (a != 0.0f)
                                acc += a * std::sin (ph * (float) h);
                        }
                        tbl[(size_t) s] = acc;
                        peak = juce::jmax (peak, std::abs (acc));
                    }
                    // Normalise to ~unity and write the wrap guard sample.
                    const float norm = (peak > 1.0e-6f) ? (1.0f / peak) : 1.0f;
                    for (int s = 0; s < kTableSize; ++s)
                        tbl[(size_t) s] *= norm;
                    tbl[(size_t) kTableSize] = tbl[0];
                }
            }
        }

        [[nodiscard]] int mipForFreq (float freqHz, float sr) const noexcept
        {
            const float nyq = sr * 0.5f;
            const float needed = (freqHz > 1.0f) ? (nyq / freqHz) : (float) (kTableSize / 2);
            const float full = (float) (kTableSize / 2);
            int level = (int) std::ceil (std::log2 (juce::jmax (1.0f, full / needed)));
            return juce::jlimit (0, kNumMips - 1, level);
        }

        static const WavetableSet& get()
        {
            static WavetableSet s;
            static const bool init = [] { s.generate(); return true; }();
            juce::ignoreUnused (init);
            return s;
        }
    };

    // Per-voice wavetable oscillator: phase accumulator + position morph.
    class WavetableOsc
    {
    public:
        void prepare (double sampleRate) noexcept
        {
            sr = static_cast<float> (sampleRate);
            invSr = 1.0f / sr;
            set = &WavetableSet::get();
            reset();
        }

        void reset() noexcept { phase = 0.0f; }

        void setFrequency (float hz) noexcept  { freq = hz; }
        void setPosition  (float p01) noexcept { pos = juce::jlimit (0.0f, 1.0f, p01); }

        [[nodiscard]] float tick() noexcept
        {
            const int mip = set->mipForFreq (freq, sr);

            const float p = phase * (float) WavetableSet::kTableSize;
            const int   i0 = (int) p;
            const float frac = p - (float) i0;

            const float fp = pos * (float) (WavetableSet::kFrames - 1);
            const int   f0 = (int) fp;
            const int   f1 = juce::jmin (f0 + 1, WavetableSet::kFrames - 1);
            const float fr = fp - (float) f0;

            const auto& tA = set->tables[(size_t) f0][(size_t) mip];
            const auto& tB = set->tables[(size_t) f1][(size_t) mip];
            const float a = tA[(size_t) i0] + frac * (tA[(size_t) (i0 + 1)] - tA[(size_t) i0]);
            const float b = tB[(size_t) i0] + frac * (tB[(size_t) (i0 + 1)] - tB[(size_t) i0]);
            const float out = a + fr * (b - a);

            phase += freq * invSr;
            if (phase >= 1.0f) phase -= 1.0f;
            return out;
        }

    private:
        const WavetableSet* set { nullptr };
        float sr    { 48000.0f };
        float invSr { 1.0f / 48000.0f };
        float freq  { 440.0f };
        float phase { 0.0f };
        float pos   { 0.0f };
    };
}
