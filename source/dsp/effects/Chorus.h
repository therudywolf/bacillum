#pragma once

#include "params/Params.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

namespace bacillum::dsp
{
    // Unified Chorus / Flanger / Phaser. Spec §5.5.3.
    //
    // Chorus / Flanger: stereo modulated Lagrange3-interpolated delay line.
    //   - Chorus:  base 22 ms, depth 1.5 ms, feedback small
    //   - Flanger: base 3  ms, depth 2.0 ms, feedback bigger, narrower sweep
    //
    // Phaser: 6-stage all-pass cascade per channel, all-pass coefficient
    // swept by the LFO. Light feedback.
    //
    // LFO is a quadrature pair (sin / cos) so L and R get 90° phase offset,
    // which is what gives chorus its "wide" feel.
    class ChorusFx
    {
    public:
        using Mode = params::ChorusMode;

        void prepare (double sampleRate, int maxBlockSize)
        {
            sr = static_cast<float> (sampleRate);
            juce::dsp::ProcessSpec spec;
            spec.sampleRate       = sampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (juce::jmax (32, maxBlockSize));
            spec.numChannels      = 1;

            for (auto* line : { &lineL, &lineR })
            {
                line->reset();
                line->setMaximumDelayInSamples (static_cast<int> (sr * 0.05) + 4);  // 50 ms
                line->prepare (spec);
            }

            for (auto& ap : apL) ap = 0.0f;
            for (auto& ap : apR) ap = 0.0f;

            lfoPhase = 0.0f;
            lpL = lpR = 0.0f;
        }

        void reset() noexcept
        {
            lineL.reset();
            lineR.reset();
            for (auto& ap : apL) ap = 0.0f;
            for (auto& ap : apR) ap = 0.0f;
            lfoPhase = 0.0f;
            lpL = lpR = 0.0f;
        }

        void setMode     (Mode m)        noexcept { mode = m; }
        void setMix      (float m01)     noexcept { mix       = juce::jlimit (0.0f, 1.0f,  m01); }
        void setRate     (float hz)      noexcept { rateHz    = juce::jmax  (0.01f, hz); }
        void setDepth    (float d01)     noexcept { depth     = juce::jlimit (0.0f, 1.0f,  d01); }
        void setFeedback (float fb01)    noexcept { feedback  = juce::jlimit (0.0f, 0.9f,  fb01); }

        void process (float* inOutL, float* inOutR, int numSamples) noexcept
        {
            if (mix < 0.0005f) return;   // bypass when fully dry

            const float lfoInc = juce::MathConstants<float>::twoPi * rateHz / sr;

            // Mode-specific defaults.
            float baseMs, depthMs;
            switch (mode)
            {
                case Mode::Flanger: baseMs = 3.0f;  depthMs = depth * 2.5f;  break;
                case Mode::Phaser:  baseMs = 0.0f;  depthMs = 0.0f;          break;  // unused
                case Mode::Chorus:
                default:            baseMs = 18.0f; depthMs = depth * 8.0f;  break;
            }

            const float baseSamples  = baseMs  * sr * 0.001f;
            const float depthSamples = depthMs * sr * 0.001f;

            const float dry = 1.0f - mix;
            const float wet = mix;

            for (int n = 0; n < numSamples; ++n)
            {
                const float lfoL = std::sin (lfoPhase);
                const float lfoR = std::sin (lfoPhase + juce::MathConstants<float>::halfPi);
                lfoPhase += lfoInc;
                if (lfoPhase >= juce::MathConstants<float>::twoPi)
                    lfoPhase -= juce::MathConstants<float>::twoPi;

                const float inL = inOutL[n];
                const float inR = inOutR[n];

                float wetL, wetR;
                if (mode == Mode::Phaser)
                {
                    // All-pass coefficient swept by LFO. Centre ~700 Hz, ±LFO octaves.
                    const float baseFreq = 700.0f;
                    const float oct      = depth * 2.0f;  // ±2 octaves
                    const float fL = baseFreq * std::pow (2.0f, oct * lfoL);
                    const float fR = baseFreq * std::pow (2.0f, oct * lfoR);
                    const float aL = (1.0f - std::tan (juce::MathConstants<float>::pi * fL / sr))
                                   / (1.0f + std::tan (juce::MathConstants<float>::pi * fL / sr));
                    const float aR = (1.0f - std::tan (juce::MathConstants<float>::pi * fR / sr))
                                   / (1.0f + std::tan (juce::MathConstants<float>::pi * fR / sr));

                    float xL = inL + feedback * fbZL;
                    float xR = inR + feedback * fbZR;

                    for (auto& z : apL)
                    {
                        const float yk = -aL * xL + z;
                        z  = xL + aL * yk;
                        xL = yk;
                    }
                    for (auto& z : apR)
                    {
                        const float yk = -aR * xR + z;
                        z  = xR + aR * yk;
                        xR = yk;
                    }
                    fbZL = xL;
                    fbZR = xR;

                    wetL = xL;
                    wetR = xR;
                }
                else
                {
                    const float dL = baseSamples + depthSamples * (0.5f + 0.5f * lfoL);
                    const float dR = baseSamples + depthSamples * (0.5f + 0.5f * lfoR);

                    lineL.setDelay (dL);
                    lineR.setDelay (dR);

                    wetL = lineL.popSample (0);
                    wetR = lineR.popSample (0);

                    // Feedback path with gentle damping.
                    lpL += 0.4f * (wetL - lpL);
                    lpR += 0.4f * (wetR - lpR);

                    lineL.pushSample (0, inL + feedback * lpL);
                    lineR.pushSample (0, inR + feedback * lpR);
                }

                inOutL[n] = dry * inL + wet * wetL;
                inOutR[n] = dry * inR + wet * wetR;
            }
        }

    private:
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> lineL, lineR;

        // 6-stage all-pass cascade for phaser.
        std::array<float, 6> apL {};
        std::array<float, 6> apR {};
        float fbZL { 0.0f }, fbZR { 0.0f };

        float lpL { 0.0f }, lpR { 0.0f };  // feedback damping LP for chorus/flanger

        float sr        { 48000.0f };
        float lfoPhase  { 0.0f };
        float rateHz    { 0.6f };
        float depth     { 0.5f };
        float feedback  { 0.2f };
        float mix       { 0.0f };
        Mode  mode      { Mode::Chorus };
    };
}
