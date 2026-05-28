#pragma once

#include "dsp/util/AudioVizBuffer.h"
#include "gui/CyberLookAndFeel.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <array>

namespace bacillum::gui
{
    // Real-time oscilloscope. Pulls the most recent ~10 ms from the shared
    // AudioVizBuffer, runs a cheap rising-zero-cross trigger search, draws
    // a smooth path with a soft glow underneath. 30 fps.
    class Oscilloscope final : public juce::Component, private juce::Timer
    {
    public:
        explicit Oscilloscope (const dsp::AudioVizBuffer& src) noexcept
            : viz (src)
        {
            setOpaque (true);
            startTimerHz (30);
        }

        ~Oscilloscope() override { stopTimer(); }

        void setSampleRate (double sr) noexcept { sampleRate = static_cast<float> (sr); }

        void paint (juce::Graphics& g) override
        {
            const auto r = getLocalBounds().toFloat();

            // Background
            g.fillAll (Palette::voidBg());

            // Grid: 8 verticals × 4 horizontals
            g.setColour (Palette::grid().withAlpha (0.45f));
            for (int i = 1; i < 8; ++i)
            {
                const float x = r.getX() + r.getWidth() * (float) i / 8.0f;
                g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
            }
            for (int i = 1; i < 4; ++i)
            {
                const float y = r.getY() + r.getHeight() * (float) i / 4.0f;
                g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
            }
            // Centre line accent
            g.setColour (Palette::cyan().withAlpha (0.18f));
            const float cy = r.getCentreY();
            g.drawHorizontalLine ((int) cy, r.getX(), r.getRight());

            // Frame
            g.setColour (Palette::cyanDim());
            g.drawRect (r, 1.0f);

            // ----- trace path -----
            if (! viz.readLatest (window.data(), kWindow))
            {
                drawIdleLabel (g);
                return;
            }

            // Trigger: from the middle of the window, scan backwards for a
            // rising zero-cross. If none found, just use offset = 0.
            const int kView    = kWindow / 2;
            const int searchEnd = kView;  // we have indices [0..kWindow-1]
            int trigger = 0;
            for (int i = kView; i > 1; --i)
            {
                if (window[i - 1] <= 0.0f && window[i] > 0.0f)
                {
                    trigger = i;
                    break;
                }
            }
            trigger = juce::jlimit (0, kWindow - kView - 1, trigger);

            const float midY  = r.getCentreY();
            const float ampPx = r.getHeight() * 0.45f;

            juce::Path trace;
            float prevY = midY - window[trigger] * ampPx;
            trace.startNewSubPath (r.getX(), prevY);
            for (int i = 1; i < kView; ++i)
            {
                const float x = r.getX() + (float) i * r.getWidth() / (float) (kView - 1);
                const float y = midY - juce::jlimit (-1.5f, 1.5f, window[trigger + i]) * ampPx;
                trace.lineTo (x, y);
            }

            // Glow underneath
            g.setColour (Palette::cyan().withAlpha (0.18f));
            g.strokePath (trace, juce::PathStrokeType (5.0f,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            // Crisp top
            g.setColour (Palette::cyan());
            g.strokePath (trace, juce::PathStrokeType (1.6f,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            // Corner label
            drawLabel (g, "SCOPE");
        }

    private:
        void timerCallback() override { repaint(); }

        void drawLabel (juce::Graphics& g, const juce::String& text) const
        {
            g.setColour (Palette::boneDim());
            g.setFont (juce::Font (juce::FontOptions ("Consolas", 10.0f, juce::Font::bold)));
            g.drawText (text, juce::Rectangle<int> (6, 4, 60, 14),
                        juce::Justification::topLeft);
        }

        void drawIdleLabel (juce::Graphics& g)
        {
            g.setColour (Palette::boneDim());
            g.setFont (juce::Font (juce::FontOptions ("Consolas", 11.0f, juce::Font::plain)));
            g.drawText ("// scope - awaiting signal",
                        getLocalBounds(),
                        juce::Justification::centred);
            drawLabel (g, "SCOPE");
        }

        static constexpr int kWindow = 2048;   // 1024 displayed + 1024 trigger headroom

        const dsp::AudioVizBuffer& viz;
        mutable std::array<float, kWindow> window {};
        float sampleRate { 48000.0f };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oscilloscope)
    };
}
