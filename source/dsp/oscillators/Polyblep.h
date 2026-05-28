#pragma once

#include <cmath>

namespace bacillum::dsp
{
    // Anti-aliasing residual for trivial saw/square oscillators.
    // t : current phase in [0, 1).  dt : phase increment per sample = f / sr.
    // Reference: Välimäki & Huovilainen, "Antialiasing Oscillators in
    // Subtractive Synthesis", IEEE SPM 2007. Spec §5.1.1.
    [[nodiscard]] inline float polyBlep(float t, float dt) noexcept
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }
}
