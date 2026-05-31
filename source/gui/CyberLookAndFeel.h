#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

namespace bacillum::gui
{
    // Wolf-cyberpunk LookAndFeel.
    // Palette taken from rudywolf.ru / forestserver.ru and the user's desktop:
    //   void:    near-black background (#0A0B0F)
    //   ink:     darker rectangles for section bodies (#11141A)
    //   bone:    primary text (#E8E8EF)
    //   cyan:    primary accent — knob fills, borders, active states (#4DD9F2)
    //   blood:   secondary accent — destructive/heat params, sustain peaks (#E63946)
    //   grid:    subtle separator (#1F2630)
    //
    // No rounded corners, no gradients — sharp angular geometry, thin neon-ish
    // strokes, monospace labels with terminal-style prefixes.
    struct Palette
    {
        static inline juce::Colour voidBg () noexcept { return juce::Colour::fromRGB (10, 11, 15); }
        static inline juce::Colour inkBg  () noexcept { return juce::Colour::fromRGB (17, 20, 26); }
        static inline juce::Colour grid   () noexcept { return juce::Colour::fromRGB (31, 38, 48); }
        static inline juce::Colour bone   () noexcept { return juce::Colour::fromRGB (232, 232, 239); }
        static inline juce::Colour boneDim() noexcept { return juce::Colour::fromRGBA (232, 232, 239, 180); }
        static inline juce::Colour cyan   () noexcept { return juce::Colour::fromRGB (77, 217, 242); }
        static inline juce::Colour cyanDim() noexcept { return juce::Colour::fromRGBA (77, 217, 242, 120); }
        static inline juce::Colour blood  () noexcept { return juce::Colour::fromRGB (230,  57,  70); }
        static inline juce::Colour bloodDim()noexcept { return juce::Colour::fromRGBA (230,  57,  70, 160); }
    };

    class CyberLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        CyberLookAndFeel()
        {
            setDefaultSansSerifTypefaceName ("Consolas");  // Win fallback chain handles missing

            setColour (juce::ResizableWindow::backgroundColourId,    Palette::voidBg());
            setColour (juce::DocumentWindow::backgroundColourId,     Palette::voidBg());

            setColour (juce::PopupMenu::backgroundColourId,          Palette::inkBg());
            setColour (juce::PopupMenu::textColourId,                Palette::bone());
            setColour (juce::PopupMenu::highlightedBackgroundColourId,Palette::cyan().withAlpha (0.18f));
            setColour (juce::PopupMenu::highlightedTextColourId,     Palette::cyan());

            setColour (juce::ComboBox::backgroundColourId,           Palette::voidBg());
            setColour (juce::ComboBox::outlineColourId,              Palette::cyanDim());
            setColour (juce::ComboBox::textColourId,                 Palette::bone());
            setColour (juce::ComboBox::arrowColourId,                Palette::cyan());
            setColour (juce::ComboBox::buttonColourId,               Palette::voidBg());

            setColour (juce::Label::textColourId,                    Palette::boneDim());

            setColour (juce::Slider::backgroundColourId,             juce::Colours::transparentBlack);
            setColour (juce::Slider::trackColourId,                  Palette::grid());
            setColour (juce::Slider::rotarySliderFillColourId,       Palette::cyan());
            setColour (juce::Slider::rotarySliderOutlineColourId,    Palette::grid());
            setColour (juce::Slider::thumbColourId,                  Palette::cyan());
            setColour (juce::Slider::textBoxTextColourId,            Palette::bone());
            setColour (juce::Slider::textBoxBackgroundColourId,      juce::Colours::transparentBlack);
            setColour (juce::Slider::textBoxOutlineColourId,         juce::Colours::transparentBlack);
            setColour (juce::Slider::textBoxHighlightColourId,       Palette::cyan().withAlpha (0.3f));

            setColour (juce::CaretComponent::caretColourId,          Palette::cyan());

            setColour (juce::TextButton::buttonColourId,             Palette::voidBg());
            setColour (juce::TextButton::buttonOnColourId,           Palette::cyan().withAlpha (0.2f));
            setColour (juce::TextButton::textColourOnId,             Palette::cyan());
            setColour (juce::TextButton::textColourOffId,            Palette::bone());

            setColour (juce::MidiKeyboardComponent::whiteNoteColourId,        Palette::bone());
            setColour (juce::MidiKeyboardComponent::blackNoteColourId,        juce::Colour::fromRGB (5, 6, 9));
            setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, Palette::grid());
            setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Palette::cyan().withAlpha (0.35f));
            setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,   Palette::cyan().withAlpha (0.7f));
            setColour (juce::MidiKeyboardComponent::textLabelColourId,        Palette::boneDim());
            setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, Palette::inkBg());
            setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId,Palette::cyan());
            setColour (juce::MidiKeyboardComponent::shadowColourId,           juce::Colours::transparentBlack);
        }

        // === Fonts =========================================================
        juce::Font getLabelFont (juce::Label& l) override
        {
            return juce::Font (juce::FontOptions ("Consolas",
                                                  l.getFont().getHeight(),
                                                  l.getFont().getStyleFlags()));
        }

        juce::Font getComboBoxFont (juce::ComboBox&) override
        {
            return juce::Font (juce::FontOptions ("Consolas", 13.0f, juce::Font::plain));
        }

        juce::Font getPopupMenuFont() override
        {
            return juce::Font (juce::FontOptions ("Consolas", 13.0f, juce::Font::plain));
        }

        juce::Font getSliderPopupFont (juce::Slider&) override
        {
            return juce::Font (juce::FontOptions ("Consolas", 12.0f, juce::Font::plain));
        }

        juce::Font getTextButtonFont (juce::TextButton&, int /*buttonHeight*/) override
        {
            return juce::Font (juce::FontOptions ("Consolas", 12.0f, juce::Font::plain));
        }

        // === Rotary slider — concentric cyber knob ==========================
        void drawRotarySlider (juce::Graphics& g,
                               int x, int y, int width, int height,
                               float sliderPos, float rotaryStart, float rotaryEnd,
                               juce::Slider& slider) override
        {
            const float diameter = static_cast<float> (juce::jmin (width, height)) * 0.86f;
            const float radius   = diameter * 0.5f;
            const float cx = static_cast<float> (x) + width  * 0.5f;
            const float cy = static_cast<float> (y) + height * 0.5f;

            const float angle = rotaryStart + sliderPos * (rotaryEnd - rotaryStart);

            // Detect bipolar range so we draw the fill arc from centre.
            const bool isBipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;

            const juce::Colour fillCol    = slider.findColour (juce::Slider::rotarySliderFillColourId);
            const juce::Colour outlineCol = slider.findColour (juce::Slider::rotarySliderOutlineColourId);

            // --- outer outline circle (dashed effect by two thin strokes) ---
            {
                juce::Path ring;
                ring.addCentredArc (cx, cy, radius, radius, 0.0f,
                                    rotaryStart, rotaryEnd, true);
                g.setColour (outlineCol);
                g.strokePath (ring, juce::PathStrokeType (1.4f));
            }

            // --- value arc ---
            {
                juce::Path arc;
                if (isBipolar)
                {
                    const float zero = rotaryStart + 0.5f * (rotaryEnd - rotaryStart);
                    arc.addCentredArc (cx, cy, radius, radius, 0.0f,
                                       zero, angle, true);
                }
                else
                {
                    arc.addCentredArc (cx, cy, radius, radius, 0.0f,
                                       rotaryStart, angle, true);
                }
                g.setColour (fillCol);
                g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                              juce::PathStrokeType::rounded));

                // Subtle glow underneath.
                g.setColour (fillCol.withAlpha (0.18f));
                g.strokePath (arc, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                              juce::PathStrokeType::rounded));
            }

            // --- inner disk: dark, slightly inset ---
            const float innerR = radius * 0.66f;
            g.setColour (Palette::inkBg());
            g.fillEllipse (cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
            g.setColour (outlineCol);
            g.drawEllipse (cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, 1.0f);

            // --- pointer line ---
            {
                juce::Path needle;
                const float tipLen = innerR * 0.95f;
                const float baseLen = innerR * 0.25f;
                needle.startNewSubPath (0.0f, -baseLen);
                needle.lineTo          (0.0f, -tipLen);
                g.setColour (fillCol);
                g.strokePath (needle,
                              juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded),
                              juce::AffineTransform::rotation (angle).translated (cx, cy));
            }

            // --- live modulation ring (set via slider properties by the editor) ---
            if (slider.getProperties().getWithDefault ("hasMod", false))
            {
                const float modNorm  = juce::jlimit (0.0f, 1.0f,
                                          (float) slider.getProperties().getWithDefault ("modNorm", (double) sliderPos));
                const float modAngle = rotaryStart + modNorm * (rotaryEnd - rotaryStart);

                // Translucent blood-red arc from the base value to the modulated value…
                juce::Path modArc;
                modArc.addCentredArc (cx, cy, radius, radius, 0.0f, angle, modAngle, true);
                g.setColour (Palette::blood().withAlpha (0.55f));
                g.strokePath (modArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                                  juce::PathStrokeType::rounded));
                // …plus a bright tick at the modulated position.
                juce::Path tick;
                tick.startNewSubPath (0.0f, -radius - 1.0f);
                tick.lineTo          (0.0f, -radius + 4.0f);
                g.setColour (Palette::blood());
                g.strokePath (tick, juce::PathStrokeType (2.0f),
                              juce::AffineTransform::rotation (modAngle).translated (cx, cy));
            }
        }

        // === ComboBox — angular dropdown ====================================
        void drawComboBox (juce::Graphics& g, int width, int height,
                           bool /*isButtonDown*/, int /*buttonX*/, int /*buttonY*/,
                           int /*buttonW*/, int /*buttonH*/,
                           juce::ComboBox& box) override
        {
            const juce::Rectangle<int> r (0, 0, width, height);

            g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
            g.fillRect (r);

            g.setColour (box.findColour (juce::ComboBox::outlineColourId));
            g.drawRect (r, 1);

            // Right-side arrow: down chevron made from two strokes (ASCII v).
            const float ax = static_cast<float> (width)  - 14.0f;
            const float ay = static_cast<float> (height) * 0.5f;
            juce::Path arrow;
            arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
            arrow.lineTo          (ax,        ay + 3.0f);
            arrow.lineTo          (ax + 4.0f, ay - 2.0f);
            g.setColour (Palette::cyan());
            g.strokePath (arrow, juce::PathStrokeType (1.6f));
        }

        void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
        {
            label.setBounds (8, 0, box.getWidth() - 24, box.getHeight());
            label.setFont (getComboBoxFont (box));
            label.setColour (juce::Label::textColourId, box.findColour (juce::ComboBox::textColourId));
        }

        // === Popup menu — sharp items =======================================
        void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
        {
            g.fillAll (Palette::inkBg());
            g.setColour (Palette::cyanDim());
            g.drawRect (0, 0, width, height, 1);
        }
    };
}
