#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>

namespace bacillum::dsp
{
    // Stereo delay with per-channel times, cross-feedback (ping-pong),
    // damping low-pass on the feedback path.
    //
    // Capacity is pre-allocated in prepare() — no allocation in process().
    class StereoDelay
    {
    public:
        void prepare(double sampleRate, double maxDelaySeconds = 4.0)
        {
            sr = static_cast<float>(sampleRate);
            const int maxLen = juce::nextPowerOfTwo (static_cast<int>(sr * maxDelaySeconds) + 8);

            juce::dsp::ProcessSpec spec;
            spec.sampleRate       = sampleRate;
            spec.maximumBlockSize = 4096;
            spec.numChannels      = 1;

            delayL.reset();
            delayR.reset();
            delayL.setMaximumDelayInSamples(maxLen);
            delayR.setMaximumDelayInSamples(maxLen);
            delayL.prepare(spec);
            delayR.prepare(spec);

            dampL = 0.0f;
            dampR = 0.0f;

            setTimes(0.25f, 0.25f);
            setFeedback(0.4f);
            setMix(0.0f);
            setDampingCutoff(8000.0f);
        }

        void reset() noexcept
        {
            delayL.reset();
            delayR.reset();
            dampL = dampR = 0.0f;
        }

        void setTimes(float secondsL, float secondsR) noexcept
        {
            const float maxSec = (delayL.getMaximumDelayInSamples() - 4) * (1.0f / sr);
            timeL = juce::jlimit(0.001f, maxSec, secondsL);
            timeR = juce::jlimit(0.001f, maxSec, secondsR);
            // SmoothedValue inside DelayLine handles fractional ramps.
            delayL.setDelay(timeL * sr);
            delayR.setDelay(timeR * sr);
        }

        void setFeedback(float fb01) noexcept
        {
            // Clamp safely below unity to avoid runaway. 0.95 is plenty wild.
            feedback = juce::jlimit(0.0f, 0.95f, fb01);
        }

        void setCrossFeedback(float ping01) noexcept
        {
            cross = juce::jlimit(0.0f, 1.0f, ping01);
        }

        void setMix(float mix01) noexcept
        {
            mix = juce::jlimit(0.0f, 1.0f, mix01);
        }

        void setDampingCutoff(float hz) noexcept
        {
            const float fc = juce::jlimit(200.0f, sr * 0.49f, hz);
            // 1-pole LP coefficient: y = y + a*(x - y); a = 1 - exp(-2*pi*fc/sr).
            dampingCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * fc / sr);
        }

        // Process in place. inL/inR are mixed with wet according to mix01.
        void process(float* inOutL, float* inOutR, int numSamples) noexcept
        {
            const float dry = 1.0f - mix;
            const float wet = mix;

            for (int n = 0; n < numSamples; ++n)
            {
                const float dryL = inOutL[n];
                const float dryR = inOutR[n];

                const float wetL = delayL.popSample(0);
                const float wetR = delayR.popSample(0);

                // Damping LP on feedback path.
                dampL += dampingCoeff * (wetL - dampL);
                dampR += dampingCoeff * (wetR - dampR);

                // Ping-pong: cross-feed from opposite tap.
                const float fbL = dampL * (1.0f - cross) + dampR * cross;
                const float fbR = dampR * (1.0f - cross) + dampL * cross;

                delayL.pushSample(0, dryL + fbL * feedback);
                delayR.pushSample(0, dryR + fbR * feedback);

                inOutL[n] = dryL * dry + wetL * wet;
                inOutR[n] = dryR * dry + wetR * wet;
            }
        }

    private:
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayL;
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayR;

        float sr            { 48000.0f };
        float timeL         { 0.25f };
        float timeR         { 0.25f };
        float feedback      { 0.4f };
        float cross         { 0.5f };
        float mix           { 0.0f };
        float dampingCoeff  { 0.5f };
        float dampL         { 0.0f };
        float dampR         { 0.0f };
    };
}
