#pragma once

#include "PluginProcessor.h"
#include "gui/CyberLookAndFeel.h"
#include "gui/Oscilloscope.h"
#include "gui/Spectroscope.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace bacillum
{
    class PluginEditor final : public juce::AudioProcessorEditor,
                               private juce::Timer
    {
    public:
        explicit PluginEditor (PluginProcessor& p);
        ~PluginEditor() override;

        void paint (juce::Graphics& g) override;
        void resized() override;
        bool keyPressed (const juce::KeyPress& key) override;
        bool keyStateChanged (bool isKeyDown) override;

    private:
        void timerCallback() override;

        using APVTS = juce::AudioProcessorValueTreeState;
        using SAtt  = APVTS::SliderAttachment;
        using CAtt  = APVTS::ComboBoxAttachment;

        struct LabeledRotary
        {
            juce::Slider slider;
            juce::Label  label;
            std::unique_ptr<SAtt> attach;

            LabeledRotary (APVTS& s, const juce::String& paramID, const juce::String& text)
            {
                slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
                slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 14);
                label.setText (text, juce::dontSendNotification);
                label.setJustificationType (juce::Justification::centred);
                label.setFont (juce::Font (juce::FontOptions ("Consolas", 11.0f, juce::Font::plain)));
                attach = std::make_unique<SAtt>(s, paramID, slider);
            }
        };

        struct LabeledCombo
        {
            juce::ComboBox combo;
            juce::Label    label;
            std::unique_ptr<CAtt> attach;

            LabeledCombo (APVTS& s, const juce::String& paramID, const juce::String& text)
            {
                label.setText (text, juce::dontSendNotification);
                label.setJustificationType (juce::Justification::centred);
                label.setFont (juce::Font (juce::FontOptions ("Consolas", 11.0f, juce::Font::plain)));
                attach = std::make_unique<CAtt>(s, paramID, combo);
            }
        };

        void addAndDisplay (LabeledRotary& r);
        void addAndDisplay (LabeledCombo&  c);

        struct Section
        {
            juce::String title;
            juce::Rectangle<int> bounds;
        };
        void drawSection (juce::Graphics& g, const Section& s);
        void drawHeader  (juce::Graphics& g, juce::Rectangle<int> area);
        void drawGrid    (juce::Graphics& g, juce::Rectangle<int> area);

        gui::CyberLookAndFeel cyberLaf;
        PluginProcessor& processor;

        // === OSC ============================================================
        LabeledCombo  osc1Wave;
        LabeledRotary osc1Pitch, osc1Detune, osc1PW, osc1Level;
        LabeledCombo  osc2Wave;
        LabeledRotary osc2Pitch, osc2Detune, osc2PW, osc2Level;

        // === Sub + HyperSaw ================================================
        LabeledCombo  subWaveform, subOctave;
        LabeledRotary subLevel, hyperDetune, hyperMix;

        // === Noise + Unison ================================================
        LabeledCombo  noiseType;
        LabeledRotary noiseLevel, unisonCount, unisonDetune, unisonSpread;

        // === Filter ========================================================
        LabeledCombo  filterMode;
        LabeledRotary filterCutoff, filterRes, filterDrive;
        LabeledRotary filterKeytrack, filterEnvAmt, filterVelAmt;

        // === Envs ==========================================================
        LabeledRotary fEnvA, fEnvD, fEnvS, fEnvR;
        LabeledRotary ampA, ampD, ampS, ampR;

        // === LFO1 ==========================================================
        LabeledCombo  lfo1Shape;
        LabeledRotary lfo1Rate, lfo1ToCutoff, lfo1ToPitch, lfo1ToAmp, lfo1FadeIn;

        // === Chorus ========================================================
        LabeledCombo  chorusMode;
        LabeledRotary chorusMix, chorusRate, chorusDepth, chorusFeedback;

        // === Delay / Reverb ================================================
        LabeledRotary delayMix, delayTimeL, delayTimeR, delayFB, delayPingPong;
        LabeledRotary reverbMix, reverbSize, reverbDamping, reverbWidth;

        // === Master ========================================================
        LabeledRotary masterGain, masterPan;
        LabeledCombo  polyMode;

        // === On-screen + visualisers =======================================
        juce::MidiKeyboardComponent keyboard;
        gui::Oscilloscope scope;
        gui::Spectroscope analyzer;

        std::array<Section, 14> sections {};
        bool caretOn { true };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
    };
}
