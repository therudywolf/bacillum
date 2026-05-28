#pragma once

#include <cstdint>

namespace bacillum::dsp
{
    // Cheap, RT-safe xorshift32. Seed-able per voice so unison noise sources
    // don't lock-step. No std::random_device on the audio thread.
    class WhiteNoise
    {
    public:
        explicit WhiteNoise(std::uint32_t seed = 0x9E3779B9u) noexcept
        {
            // Avoid the all-zero attractor.
            state = seed ? seed : 0x9E3779B9u;
        }

        void reset(std::uint32_t seed = 0x9E3779B9u) noexcept
        {
            state = seed ? seed : 0x9E3779B9u;
        }

        [[nodiscard]] float tick() noexcept
        {
            // xorshift32
            std::uint32_t x = state;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            state = x;
            // map [0, 2^32) → [-1, +1)
            return static_cast<float>(static_cast<std::int32_t>(x)) * (1.0f / 2147483648.0f);
        }

    private:
        std::uint32_t state { 0x9E3779B9u };
    };

    // Paul Kellet's pink noise filter (spec §5.1.7).
    // ~ -3 dB / octave from ~10 Hz up to ~Nyquist, within ±0.5 dB.
    class PinkNoise
    {
    public:
        explicit PinkNoise(std::uint32_t seed = 0xC0FFEE13u) noexcept
            : white(seed) {}

        void reset(std::uint32_t seed = 0xC0FFEE13u) noexcept
        {
            white.reset(seed);
            b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0f;
        }

        [[nodiscard]] float tick() noexcept
        {
            const float w = white.tick();
            b0 = 0.99886f * b0 + w * 0.0555179f;
            b1 = 0.99332f * b1 + w * 0.0750759f;
            b2 = 0.96900f * b2 + w * 0.1538520f;
            b3 = 0.86650f * b3 + w * 0.3104856f;
            b4 = 0.55000f * b4 + w * 0.5329522f;
            b5 = -0.7616f * b5 - w * 0.0168980f;
            const float out = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362f) * 0.11f;
            b6 = w * 0.115926f;
            return out;
        }

    private:
        WhiteNoise white;
        float b0 { 0.0f }, b1 { 0.0f }, b2 { 0.0f }, b3 { 0.0f };
        float b4 { 0.0f }, b5 { 0.0f }, b6 { 0.0f };
    };
}
