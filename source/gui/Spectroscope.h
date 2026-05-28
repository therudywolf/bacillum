#pragma once

#include "dsp/util/AudioVizBuffer.h"
#include "gui/CyberLookAndFeel.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace bacillum::gui
{
    // FFT spectrum analyser.
    //  - Hann-windowed 2048-pt FFT (juce::dsp::FFT order 11).
    //  - Magnitudes in dB (floor -80, ceiling 0).
    //  - Log frequency axis 20 Hz .. ~Nyquist.
    //  - Per-bin attack/decay smoothing for a calm display.
    //  - Peak hold (~1.5 s fall).
    //  - Filled area in cyan → red gradient based on magnitude height.
    class Spectroscope final : public juce::Component, private juce::Timer
    {
    public:
        explicit Spectroscope (const dsp::AudioVizBuffer& src) noexcept
            : viz (src),
              fft (kFftOrder),
              window (kFftSize, juce::dsp::WindowingFunction<float>::hann)
        {
            setOpaque (true);
            smoothed.fill (-80.0f);
            peaks.fill (-80.0f);
            peakFallPerFrame = 0.6f;  // dB per 33ms frame  → ~45 dB/s
            startTimerHz (30);
        }

        ~Spectroscope() override { stopTimer(); }

        void setSampleRate (double sr) noexcept { sampleRate = static_cast<float> (sr); }

        void paint (juce::Graphics& g) override
        {
            const auto r = getLocalBounds().toFloat();
            g.fillAll (Palette::voidBg());

            // Reference dB lines: -60 / -40 / -20 / 0
            g.setColour (Palette::grid().withAlpha (0.55f));
            g.setFont (juce::Font (juce::FontOptions ("Consolas", 9.0f, juce::Font::plain)));
            for (int dB : { -60, -40, -20, 0 })
            {
                const float y = mapDbToY (static_cast<float> (dB), r);
                g.setColour (Palette::grid().withAlpha (0.55f));
                g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
                g.setColour (Palette::boneDim().withAlpha (0.55f));
                g.drawText (juce::String (dB) + " dB",
                            juce::Rectangle<int> ((int) r.getX() + 4, (int) y - 12, 50, 12),
                            juce::Justification::topLeft);
            }

            // Reference frequency lines: 100, 1k, 10k
            const float minF = 20.0f;
            const float maxF = sampleRate * 0.5f;
            for (auto freq : { 100.0f, 1000.0f, 10000.0f })
            {
                if (freq > maxF) continue;
                const float x = mapFreqToX (freq, r, minF, maxF);
                g.setColour (Palette::grid().withAlpha (0.55f));
                g.drawVerticalLine ((int) x, r.getY(), r.getBottom());

                juce::String label = (freq >= 1000.0f)
                    ? juce::String ((int) (freq / 1000.0f)) + "k"
                    : juce::String ((int) freq);
                g.setColour (Palette::boneDim().withAlpha (0.55f));
                g.drawText (label,
                            juce::Rectangle<int> ((int) x - 18, (int) r.getBottom() - 14, 36, 12),
                            juce::Justification::centred);
            }

            // Frame
            g.setColour (Palette::cyanDim());
            g.drawRect (r, 1.0f);

            // ---- Compute new spectrum ----
            const bool gotData = viz.readLatest (fftData.data(), kFftSize);
            if (gotData)
            {
                window.multiplyWithWindowingTable (fftData.data(), kFftSize);

                // juce::dsp::FFT::performFrequencyOnlyForwardTransform writes
                // (kFftSize/2 + 1) magnitudes into the first half of the buffer.
                // We need 2x the size for the FFT to work in-place.
                std::fill (fftWork.begin(), fftWork.end(), 0.0f);
                std::copy_n (fftData.begin(), kFftSize, fftWork.begin());
                fft.performFrequencyOnlyForwardTransform (fftWork.data());

                // Convert to dB + per-bin envelope follower.
                constexpr float kNorm = 2.0f / (float) kFftSize;
                for (int i = 0; i < kBins; ++i)
                {
                    const float mag = fftWork[i] * kNorm;
                    const float dB  = juce::Decibels::gainToDecibels (mag, -120.0f);

                    // Asymmetric smoothing: fast attack, slow release.
                    const float attack  = 0.65f;
                    const float release = 0.10f;
                    const float coef    = (dB > smoothed[i]) ? attack : release;
                    smoothed[i] += coef * (dB - smoothed[i]);

                    // Peak hold
                    if (smoothed[i] > peaks[i]) peaks[i] = smoothed[i];
                    else                         peaks[i] -= peakFallPerFrame;
                    peaks[i] = std::max (peaks[i], -80.0f);
                }
            }

            // ---- Render filled curve ----
            juce::Path filled;
            juce::Path stroke;
            const float fBottom = r.getBottom();

            const int   pixels = (int) r.getWidth();
            float prevX = r.getX();
            float prevY = fBottom;
            filled.startNewSubPath (prevX, fBottom);
            stroke.startNewSubPath (prevX, mapDbToY (smoothed[1], r));

            for (int px = 0; px < pixels; ++px)
            {
                const float x = r.getX() + (float) px;
                // Map x → frequency → bin
                const float t = (float) px / (float) (pixels - 1);
                const float f = minF * std::pow (maxF / minF, t);
                const float binF = f * static_cast<float> (kFftSize) / sampleRate;
                const int   bin  = juce::jlimit (1, kBins - 2, (int) binF);
                const float frac = binF - (float) bin;
                const float dB   = smoothed[bin] * (1.0f - frac) + smoothed[bin + 1] * frac;

                const float y = mapDbToY (dB, r);
                stroke.lineTo (x, y);
                filled.lineTo (x, y);
                prevX = x; prevY = y;
            }
            filled.lineTo (prevX, fBottom);
            filled.closeSubPath();

            // Filled gradient
            juce::ColourGradient grad (Palette::cyan().withAlpha (0.55f), r.getX(), r.getBottom(),
                                       Palette::blood().withAlpha (0.85f), r.getX(), r.getY(),
                                       false);
            g.setGradientFill (grad);
            g.fillPath (filled);

            // Stroked top
            g.setColour (Palette::cyan());
            g.strokePath (stroke, juce::PathStrokeType (1.2f));

            // Peak-hold dots (subtle)
            g.setColour (Palette::bone().withAlpha (0.75f));
            for (int px = 0; px < pixels; px += 4)
            {
                const float t = (float) px / (float) (pixels - 1);
                const float f = minF * std::pow (maxF / minF, t);
                const float binF = f * static_cast<float> (kFftSize) / sampleRate;
                const int   bin  = juce::jlimit (1, kBins - 1, (int) binF);
                const float y    = mapDbToY (peaks[bin], r);
                g.fillRect (r.getX() + (float) px, y, 1.5f, 1.5f);
            }

            // Label
            g.setColour (Palette::boneDim());
            g.setFont (juce::Font (juce::FontOptions ("Consolas", 10.0f, juce::Font::bold)));
            g.drawText ("FFT", juce::Rectangle<int> (6, 4, 60, 14),
                        juce::Justification::topLeft);
        }

    private:
        void timerCallback() override { repaint(); }

        static float mapDbToY (float dB, const juce::Rectangle<float>& r)
        {
            constexpr float kMin = -80.0f, kMax = 0.0f;
            const float t = juce::jlimit (0.0f, 1.0f, (dB - kMin) / (kMax - kMin));
            return r.getBottom() - t * r.getHeight();
        }

        static float mapFreqToX (float f, const juce::Rectangle<float>& r,
                                 float minF, float maxF)
        {
            const float t = std::log (f / minF) / std::log (maxF / minF);
            return r.getX() + juce::jlimit (0.0f, 1.0f, t) * r.getWidth();
        }

        static constexpr int kFftOrder = 11;
        static constexpr int kFftSize  = 1 << kFftOrder;   // 2048
        static constexpr int kBins     = kFftSize / 2;

        const dsp::AudioVizBuffer& viz;
        juce::dsp::FFT fft;
        juce::dsp::WindowingFunction<float> window;

        // FFT scratch (twice the size for the JUCE in-place API).
        std::array<float, kFftSize>     fftData {};
        std::array<float, kFftSize * 2> fftWork {};

        std::array<float, kBins> smoothed {};
        std::array<float, kBins> peaks {};
        float peakFallPerFrame { 0.6f };

        float sampleRate { 48000.0f };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Spectroscope)
    };
}
