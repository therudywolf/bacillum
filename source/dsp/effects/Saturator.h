#pragma once

#include "params/Params.h"
#include <juce_core/juce_core.h>
#include <cmath>

namespace bacillum::dsp
{
    // Per-voice waveshaper sitting between the two filters (spec §2.1).
    // Stateless for most modes; rate-reduce keeps a sample-and-hold.
    class Saturator
    {
    public:
        using Type = params::SaturatorType;

        void reset() noexcept { hold = 0.0f; holdCounter = 0; }

        void setType  (Type t)     noexcept { type = t; }
        void setAmount(float a01)  noexcept { amount = juce::jlimit(0.0f, 1.0f, a01); }

        [[nodiscard]] float process(float x) noexcept
        {
            switch (type)
            {
                case Type::Off:
                    return x;

                case Type::Tanh:
                {
                    const float k = 1.0f + amount * 9.0f;          // 1..10
                    return std::tanh(x * k) * (1.0f / std::tanh(k));
                }
                case Type::SoftClip:
                {
                    // Cubic soft clip with make-up.
                    const float pre = 1.0f + amount * 4.0f;
                    float v = juce::jlimit(-1.0f, 1.0f, x * pre);
                    v = v - (v * v * v) * (1.0f / 3.0f);
                    return v * 1.5f;
                }
                case Type::HardClip:
                {
                    const float pre = 1.0f + amount * 6.0f;
                    return juce::jlimit(-1.0f, 1.0f, x * pre);
                }
                case Type::Foldback:
                {
                    const float pre = 1.0f + amount * 5.0f;
                    float v = x * pre;
                    const float lim = 1.0f;
                    // Reflect repeatedly into [-lim, lim].
                    while (v > lim || v < -lim)
                    {
                        if (v > lim)  v = 2.0f * lim - v;
                        if (v < -lim) v = -2.0f * lim - v;
                    }
                    return v;
                }
                case Type::BitCrush:
                {
                    // amount 0 → 16 bits, amount 1 → ~3 bits.
                    const float bits = juce::jmap(amount, 0.0f, 1.0f, 16.0f, 3.0f);
                    const float steps = std::pow(2.0f, bits);
                    return std::round(x * steps) / steps;
                }
                case Type::RateReduce:
                {
                    // Sample-and-hold decimation. amount 0 → every sample,
                    // amount 1 → hold ~32 samples.
                    const int holdN = 1 + static_cast<int>(amount * 31.0f);
                    if (holdCounter <= 0)
                    {
                        hold = x;
                        holdCounter = holdN;
                    }
                    --holdCounter;
                    return hold;
                }
                default:
                    return x;
            }
        }

    private:
        Type  type   { Type::Off };
        float amount { 0.0f };
        float hold   { 0.0f };
        int   holdCounter { 0 };
    };
}
