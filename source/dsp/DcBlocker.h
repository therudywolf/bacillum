#pragma once

namespace bacillum::dsp
{
    // 1-pole high-pass DC blocker. R ~ 0.995 → fc ~ 38 Hz @ 48 kHz.
    // y[n] = x[n] - x[n-1] + R * y[n-1]    (spec §5.1.8)
    class DcBlocker
    {
    public:
        void prepare(double /*sampleRate*/) noexcept { reset(); }

        void reset() noexcept
        {
            xPrev = 0.0f;
            yPrev = 0.0f;
        }

        [[nodiscard]] float process(float x) noexcept
        {
            const float y = x - xPrev + kR * yPrev;
            xPrev = x;
            yPrev = y;
            return y;
        }

    private:
        static constexpr float kR = 0.995f;
        float xPrev { 0.0f };
        float yPrev { 0.0f };
    };
}
