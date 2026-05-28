#pragma once

#include "Polyblep.h"
#include "params/Params.h"
#include <juce_core/juce_core.h>
#include <cmath>

namespace bacillum::dsp
{
    // Single-mode classic VA oscillator: sine / triangle / saw / square(PWM).
    // Float phase in [0, 1); PolyBLEP for saw and square discontinuities.
    // The triangle is built via a leaky integrator of the square so it shares
    // the same anti-aliasing treatment (spec §5.1.4).
    class Oscillator
    {
    public:
        using Waveform = params::Waveform;

        void prepare(double sampleRate) noexcept
        {
            sr = static_cast<float>(sampleRate);
            invSr = 1.0f / sr;
            reset();
        }

        void reset() noexcept
        {
            phase = 0.0f;
            triState = 0.0f;
        }

        void setWaveform(Waveform w) noexcept   { waveform = w; }
        void setFrequency(float hz) noexcept    { freq = hz; }
        void setPulsewidth(float pw01) noexcept { pulsewidth = juce::jlimit(0.05f, 0.95f, pw01); }

        // Hard reset of phase (for hard sync, voice retrigger if requested).
        void setPhase(float p) noexcept
        {
            phase = p - std::floor(p);
            triState = 0.0f;
        }

        [[nodiscard]] float tick() noexcept
        {
            const float dt = juce::jlimit(0.0f, 0.49f, freq * invSr);

            float out = 0.0f;
            switch (waveform)
            {
                case Waveform::Sine:
                {
                    out = std::sin(phase * juce::MathConstants<float>::twoPi);
                    break;
                }
                case Waveform::Saw:
                {
                    const float naive = 2.0f * phase - 1.0f;
                    out = naive - polyBlep(phase, dt);
                    break;
                }
                case Waveform::Square:
                {
                    const float pw = pulsewidth;
                    const float naive = (phase < pw) ? 1.0f : -1.0f;
                    float t2 = phase - pw;
                    if (t2 < 0.0f) t2 += 1.0f;
                    out = naive + polyBlep(phase, dt) - polyBlep(t2, dt);
                    break;
                }
                case Waveform::Triangle:
                {
                    // Square first, then a leaky integrator (spec §5.1.4).
                    const float pw = 0.5f;
                    const float naive = (phase < pw) ? 1.0f : -1.0f;
                    float t2 = phase - pw;
                    if (t2 < 0.0f) t2 += 1.0f;
                    const float sq = naive + polyBlep(phase, dt) - polyBlep(t2, dt);

                    triState = 0.999f * triState + dt * sq;
                    out = triState * 4.0f;
                    break;
                }
                default:
                    break;
            }

            phase += dt;
            if (phase >= 1.0f) phase -= 1.0f;
            return out;
        }

    private:
        Waveform waveform { Waveform::Saw };
        float    sr        { 48000.0f };
        float    invSr     { 1.0f / 48000.0f };
        float    freq      { 440.0f };
        float    phase     { 0.0f };
        float    pulsewidth{ 0.5f };
        float    triState  { 0.0f };
    };
}
