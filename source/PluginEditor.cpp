#include "PluginEditor.h"

namespace bacillum
{
    PluginEditor::PluginEditor (PluginProcessor& p)
        : juce::AudioProcessorEditor (&p),
          processor (p),
          osc1Wave   (p.getAPVTS(), params::ids::osc1Waveform,   "WAVE"),
          osc1Pitch  (p.getAPVTS(), params::ids::osc1Pitch,      "PITCH"),
          osc1Detune (p.getAPVTS(), params::ids::osc1Detune,     "DETUNE"),
          osc1PW     (p.getAPVTS(), params::ids::osc1Pulsewidth, "PW"),
          osc1Level  (p.getAPVTS(), params::ids::osc1Level,      "LEVEL"),

          osc2Wave   (p.getAPVTS(), params::ids::osc2Waveform,   "WAVE"),
          osc2Pitch  (p.getAPVTS(), params::ids::osc2Pitch,      "PITCH"),
          osc2Detune (p.getAPVTS(), params::ids::osc2Detune,     "DETUNE"),
          osc2PW     (p.getAPVTS(), params::ids::osc2Pulsewidth, "PW"),
          osc2Level  (p.getAPVTS(), params::ids::osc2Level,      "LEVEL"),

          subWaveform (p.getAPVTS(), params::ids::subWaveform, "SUB.W"),
          subOctave   (p.getAPVTS(), params::ids::subOctave,   "SUB.OCT"),
          subLevel    (p.getAPVTS(), params::ids::subLevel,    "SUB.LVL"),
          hyperDetune (p.getAPVTS(), params::ids::hyperDetune, "H.DET"),
          hyperMix    (p.getAPVTS(), params::ids::hyperMix,    "H.MIX"),

          noiseType  (p.getAPVTS(), params::ids::noiseType,  "NOISE"),
          noiseLevel (p.getAPVTS(), params::ids::noiseLevel, "N.LVL"),
          unisonCount  (p.getAPVTS(), params::ids::unisonCount,  "UNI.N"),
          unisonDetune (p.getAPVTS(), params::ids::unisonDetune, "UNI.DET"),
          unisonSpread (p.getAPVTS(), params::ids::unisonSpread, "UNI.SPR"),

          filterMode (p.getAPVTS(), params::ids::filterMode, "MODE"),
          filterCutoff   (p.getAPVTS(), params::ids::filterCutoff,   "CUTOFF"),
          filterRes      (p.getAPVTS(), params::ids::filterRes,      "RESO"),
          filterDrive    (p.getAPVTS(), params::ids::filterDrive,    "DRIVE"),
          filterKeytrack (p.getAPVTS(), params::ids::filterKeytrack, "KEY.TRK"),
          filterEnvAmt   (p.getAPVTS(), params::ids::filterEnvAmt,   "ENV.AMT"),
          filterVelAmt   (p.getAPVTS(), params::ids::filterVelAmt,   "VEL.AMT"),

          fEnvA (p.getAPVTS(), params::ids::filterAttack,  "A"),
          fEnvD (p.getAPVTS(), params::ids::filterDecay,   "D"),
          fEnvS (p.getAPVTS(), params::ids::filterSustain, "S"),
          fEnvR (p.getAPVTS(), params::ids::filterRelease, "R"),
          ampA  (p.getAPVTS(), params::ids::ampAttack,     "A"),
          ampD  (p.getAPVTS(), params::ids::ampDecay,      "D"),
          ampS  (p.getAPVTS(), params::ids::ampSustain,    "S"),
          ampR  (p.getAPVTS(), params::ids::ampRelease,    "R"),

          lfo1Shape    (p.getAPVTS(), params::ids::lfo1Shape,    "SHAPE"),
          lfo1Rate     (p.getAPVTS(), params::ids::lfo1Rate,     "RATE"),
          lfo1ToCutoff (p.getAPVTS(), params::ids::lfo1ToCutoff, "> CUT"),
          lfo1ToPitch  (p.getAPVTS(), params::ids::lfo1ToPitch,  "> PITCH"),
          lfo1ToAmp    (p.getAPVTS(), params::ids::lfo1ToAmp,    "> AMP"),
          lfo1FadeIn   (p.getAPVTS(), params::ids::lfo1FadeIn,   "FADE"),

          chorusMode     (p.getAPVTS(), params::ids::chorusMode,     "TYPE"),
          chorusMix      (p.getAPVTS(), params::ids::chorusMix,      "MIX"),
          chorusRate     (p.getAPVTS(), params::ids::chorusRate,     "RATE"),
          chorusDepth    (p.getAPVTS(), params::ids::chorusDepth,    "DEPTH"),
          chorusFeedback (p.getAPVTS(), params::ids::chorusFeedback, "FB"),

          delayMix      (p.getAPVTS(), params::ids::delayMix,      "MIX"),
          delayTimeL    (p.getAPVTS(), params::ids::delayTimeL,    "T.L"),
          delayTimeR    (p.getAPVTS(), params::ids::delayTimeR,    "T.R"),
          delayFB       (p.getAPVTS(), params::ids::delayFeedback, "FB"),
          delayPingPong (p.getAPVTS(), params::ids::delayPingPong, "PING"),

          reverbMix     (p.getAPVTS(), params::ids::reverbMix,     "MIX"),
          reverbSize    (p.getAPVTS(), params::ids::reverbSize,    "SIZE"),
          reverbDamping (p.getAPVTS(), params::ids::reverbDamping, "DAMP"),
          reverbWidth   (p.getAPVTS(), params::ids::reverbWidth,   "WIDTH"),

          masterGain (p.getAPVTS(), params::ids::masterGain, "GAIN"),
          masterPan  (p.getAPVTS(), params::ids::masterPan,  "PAN"),
          polyMode   (p.getAPVTS(), params::ids::polyMode,   "MODE"),

          keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard),
          scope    (p.getVizBuffer()),
          analyzer (p.getVizBuffer())
    {
        juce::LookAndFeel::setDefaultLookAndFeel (&cyberLaf);
        setLookAndFeel (&cyberLaf);

        // Hot/destructive knobs get blood-red fills.
        for (auto* r : { &filterRes, &filterDrive, &delayFB, &masterGain, &chorusFeedback })
            r->slider.setColour (juce::Slider::rotarySliderFillColourId, gui::Palette::blood());

        // Visualisers sync sample rate.
        scope.setSampleRate    (p.getCurrentSampleRate());
        analyzer.setSampleRate (p.getCurrentSampleRate());

        setSize (1200, 980);
        setResizable (true, true);
        setResizeLimits (960, 720, 2200, 1400);

        for (auto* r : { &osc1Pitch, &osc1Detune, &osc1PW, &osc1Level,
                         &osc2Pitch, &osc2Detune, &osc2PW, &osc2Level,
                         &subLevel, &hyperDetune, &hyperMix,
                         &noiseLevel, &unisonCount, &unisonDetune, &unisonSpread,
                         &filterCutoff, &filterRes, &filterDrive,
                         &filterKeytrack, &filterEnvAmt, &filterVelAmt,
                         &fEnvA, &fEnvD, &fEnvS, &fEnvR,
                         &ampA,  &ampD,  &ampS,  &ampR,
                         &lfo1Rate, &lfo1ToCutoff, &lfo1ToPitch, &lfo1ToAmp, &lfo1FadeIn,
                         &chorusMix, &chorusRate, &chorusDepth, &chorusFeedback,
                         &delayMix, &delayTimeL, &delayTimeR, &delayFB, &delayPingPong,
                         &reverbMix, &reverbSize, &reverbDamping, &reverbWidth,
                         &masterGain, &masterPan })
            addAndDisplay (*r);

        for (auto* c : { &osc1Wave, &osc2Wave,
                         &subWaveform, &subOctave,
                         &noiseType, &filterMode, &lfo1Shape,
                         &chorusMode, &polyMode })
            addAndDisplay (*c);

        keyboard.setKeyPressBaseOctave (4);
        keyboard.setLowestVisibleKey (24);
        keyboard.setAvailableRange (0, 127);
        keyboard.setWantsKeyboardFocus (true);
        addAndMakeVisible (keyboard);

        addAndMakeVisible (scope);
        addAndMakeVisible (analyzer);

        setWantsKeyboardFocus (true);
        juce::MessageManager::callAsync ([this]()
        {
            if (isShowing()) keyboard.grabKeyboardFocus();
        });

        startTimerHz (2);
    }

    PluginEditor::~PluginEditor()
    {
        stopTimer();
        setLookAndFeel (nullptr);
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void PluginEditor::timerCallback()
    {
        caretOn = ! caretOn;
        repaint (0, 0, getWidth(), 56);
    }

    bool PluginEditor::keyPressed (const juce::KeyPress& key)    { return keyboard.keyPressed (key); }
    bool PluginEditor::keyStateChanged (bool isKeyDown)          { return keyboard.keyStateChanged (isKeyDown); }

    void PluginEditor::addAndDisplay (LabeledRotary& r) { addAndMakeVisible (r.slider); addAndMakeVisible (r.label); }
    void PluginEditor::addAndDisplay (LabeledCombo&  c) { addAndMakeVisible (c.combo);  addAndMakeVisible (c.label); }

    void PluginEditor::drawGrid (juce::Graphics& g, juce::Rectangle<int> area)
    {
        g.setColour (gui::Palette::grid().withAlpha (0.35f));
        const int step = 40;
        for (int x = area.getX() + step; x < area.getRight(); x += step)
            g.drawVerticalLine (x, (float) area.getY(), (float) area.getBottom());
        for (int y = area.getY() + step; y < area.getBottom(); y += step)
            g.drawHorizontalLine (y, (float) area.getX(), (float) area.getRight());
    }

    void PluginEditor::drawHeader (juce::Graphics& g, juce::Rectangle<int> area)
    {
        g.setColour (gui::Palette::voidBg());
        g.fillRect (area);
        g.setColour (gui::Palette::cyan());
        g.fillRect (area.getX(), area.getBottom() - 1, area.getWidth(), 1);

        const auto fontH = 16.0f;
        const juce::Font promptFont (juce::FontOptions ("Consolas", fontH, juce::Font::bold));
        g.setFont (promptFont);

        g.setColour (gui::Palette::blood());
        g.drawText ("root@bacillum",
                    juce::Rectangle<int> (area.getX() + 14, area.getY(), 220, area.getHeight()),
                    juce::Justification::centredLeft);
        g.setColour (gui::Palette::boneDim());
        g.drawText (":~$",
                    juce::Rectangle<int> (area.getX() + 14 + 165, area.getY(), 30, area.getHeight()),
                    juce::Justification::centredLeft);
        g.setColour (gui::Palette::cyan());
        const juce::String cmd = "./synth -mode poly -voices 16 -engine VA";
        g.drawText (cmd,
                    juce::Rectangle<int> (area.getX() + 14 + 200, area.getY(), 600, area.getHeight()),
                    juce::Justification::centredLeft);
        if (caretOn)
        {
            const int caretX = area.getX() + 14 + 200 + promptFont.getStringWidth (cmd) + 6;
            g.setColour (gui::Palette::cyan());
            g.fillRect (caretX, area.getY() + 14, 8, (int) fontH - 2);
        }

        const juce::Font brandFont (juce::FontOptions ("Consolas", 17.0f, juce::Font::bold));
        g.setFont (brandFont);
        g.setColour (gui::Palette::blood());
        const juce::String brand = "// BACILLUM";
        const int brandW = brandFont.getStringWidth (brand);
        g.drawText (brand,
                    juce::Rectangle<int> (area.getRight() - brandW - 18, area.getY() + 4,
                                          brandW, area.getHeight() / 2),
                    juce::Justification::centredRight);
        g.setFont (juce::Font (juce::FontOptions ("Consolas", 10.0f, juce::Font::plain)));
        g.setColour (gui::Palette::boneDim());
        const juce::String hint = "PC KEYS: A W S E D F T G Y H U J K   Z X = OCTAVE";
        g.drawText (hint,
                    juce::Rectangle<int> (area.getRight() - 420,
                                          area.getY() + area.getHeight() / 2 - 2,
                                          400, area.getHeight() / 2),
                    juce::Justification::centredRight);
    }

    void PluginEditor::drawSection (juce::Graphics& g, const Section& s)
    {
        auto r = s.bounds;

        g.setColour (gui::Palette::inkBg());
        g.fillRect (r);
        g.setColour (gui::Palette::cyanDim());
        g.drawRect (r, 1);
        g.setColour (gui::Palette::cyan());
        g.fillRect (r.getX(), r.getY(), 3, r.getHeight());

        auto titleArea = r.removeFromTop (22).withTrimmedLeft (8);
        g.setColour (gui::Palette::bone());
        g.setFont (juce::Font (juce::FontOptions ("Consolas", 11.0f, juce::Font::bold)));
        g.drawText (s.title, titleArea, juce::Justification::centredLeft);

        const int cx = r.getRight() - 6;
        const int cy = r.getY() - 13;
        g.setColour (gui::Palette::cyan());
        g.drawHorizontalLine (cy, (float) cx - 3, (float) cx + 3);
        g.drawVerticalLine   (cx, (float) cy - 3, (float) cy + 3);
    }

    void PluginEditor::paint (juce::Graphics& g)
    {
        g.fillAll (gui::Palette::voidBg());

        drawGrid (g, getLocalBounds().withTrimmedTop (56).withTrimmedBottom (110));

        drawHeader (g, getLocalBounds().removeFromTop (44));
        g.setColour (gui::Palette::blood());
        g.fillRect (0, 44, getWidth(), 2);
        g.setColour (gui::Palette::cyan().withAlpha (0.4f));
        g.fillRect (0, 46, getWidth(), 1);

        for (auto& s : sections)
            if (! s.bounds.isEmpty())
                drawSection (g, s);
    }

    static void layoutKnob (juce::Rectangle<int> area,
                            juce::Component& slider, juce::Label& label)
    {
        label.setBounds (area.removeFromBottom (14));
        slider.setBounds (area.reduced (2));
    }

    static void layoutCombo (juce::Rectangle<int> area,
                             juce::Component& combo, juce::Label& label)
    {
        label.setBounds (area.removeFromTop (12));
        combo.setBounds (area.removeFromTop (22).reduced (4, 0));
    }

    void PluginEditor::resized()
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop (44 + 3);

        // Keyboard at bottom.
        const int kbH = 100;
        keyboard.setBounds (bounds.removeFromBottom (kbH).reduced (8, 4));

        auto body = bounds.reduced (8);

        // Reserve bottom strip for visualisers.
        const int vizH = 170;
        auto vizStrip = body.removeFromBottom (vizH);
        body.removeFromBottom (6);   // gap

        // 4 cols × 4 rows
        const int cols = 4, rows = 4, gap = 6;
        const int colW = (body.getWidth()  - (cols - 1) * gap) / cols;
        const int rowH = (body.getHeight() - (rows - 1) * gap) / rows;

        auto sectionRect = [&](int c, int r)
        {
            return juce::Rectangle<int>(
                body.getX() + c * (colW + gap),
                body.getY() + r * (rowH + gap),
                colW, rowH);
        };

        // Row 0 ─ source
        sections[0]  = { "// OSC.1",            sectionRect (0, 0) };
        sections[1]  = { "// OSC.2",            sectionRect (1, 0) };
        sections[2]  = { "// SUB + HYPER",      sectionRect (2, 0) };
        sections[3]  = { "// NOISE + UNISON",   sectionRect (3, 0) };
        // Row 1 ─ filter chain
        sections[4]  = { "// FILTER",           sectionRect (0, 1) };
        sections[5]  = { "// FILT.ENV",         sectionRect (1, 1) };
        sections[6]  = { "// FILT.MOD",         sectionRect (2, 1) };
        sections[7]  = { "// LFO.1",            sectionRect (3, 1) };
        // Row 2 ─ amp + FX
        sections[8]  = { "// AMP.ENV",          sectionRect (0, 2) };
        sections[9]  = { "// CHORUS",           sectionRect (1, 2) };
        sections[10] = { "// DELAY",            sectionRect (2, 2) };
        sections[11] = { "// REVERB",           sectionRect (3, 2) };
        // Row 3 ─ master + perf
        sections[12] = { "// MASTER",           sectionRect (0, 3) };
        sections[13] = { "// PERF",             sectionRect (1, 3) };

        // Full-width visualiser strip beneath the param grid:
        //   left half  -> oscilloscope
        //   right half -> spectrum analyser
        const int half = (vizStrip.getWidth() - gap) / 2;
        scope.setBounds    (vizStrip.removeFromLeft (half));
        vizStrip.removeFromLeft (gap);
        analyzer.setBounds (vizStrip);

        auto inset = [](juce::Rectangle<int> r)
        {
            return r.withTrimmedTop (24).reduced (8);
        };

        // OSC1
        {
            auto c = inset (sections[0].bounds);
            layoutCombo (c.removeFromTop (38), osc1Wave.combo, osc1Wave.label);
            const int kw = c.getWidth() / 4;
            layoutKnob (c.removeFromLeft (kw), osc1Pitch.slider,  osc1Pitch.label);
            layoutKnob (c.removeFromLeft (kw), osc1Detune.slider, osc1Detune.label);
            layoutKnob (c.removeFromLeft (kw), osc1PW.slider,     osc1PW.label);
            layoutKnob (c.removeFromLeft (kw), osc1Level.slider,  osc1Level.label);
        }
        // OSC2
        {
            auto c = inset (sections[1].bounds);
            layoutCombo (c.removeFromTop (38), osc2Wave.combo, osc2Wave.label);
            const int kw = c.getWidth() / 4;
            layoutKnob (c.removeFromLeft (kw), osc2Pitch.slider,  osc2Pitch.label);
            layoutKnob (c.removeFromLeft (kw), osc2Detune.slider, osc2Detune.label);
            layoutKnob (c.removeFromLeft (kw), osc2PW.slider,     osc2PW.label);
            layoutKnob (c.removeFromLeft (kw), osc2Level.slider,  osc2Level.label);
        }
        // SUB + HYPER
        {
            auto c = inset (sections[2].bounds);
            auto top = c.removeFromTop (38);
            const int comboW = top.getWidth() / 2;
            layoutCombo (top.removeFromLeft (comboW), subWaveform.combo, subWaveform.label);
            layoutCombo (top,                         subOctave.combo,   subOctave.label);
            const int kw = c.getWidth() / 3;
            layoutKnob (c.removeFromLeft (kw), subLevel.slider,    subLevel.label);
            layoutKnob (c.removeFromLeft (kw), hyperDetune.slider, hyperDetune.label);
            layoutKnob (c.removeFromLeft (kw), hyperMix.slider,    hyperMix.label);
        }
        // NOISE + UNISON
        {
            auto c = inset (sections[3].bounds);
            layoutCombo (c.removeFromTop (38), noiseType.combo, noiseType.label);
            const int kw = c.getWidth() / 4;
            layoutKnob (c.removeFromLeft (kw), noiseLevel.slider,   noiseLevel.label);
            layoutKnob (c.removeFromLeft (kw), unisonCount.slider,  unisonCount.label);
            layoutKnob (c.removeFromLeft (kw), unisonDetune.slider, unisonDetune.label);
            layoutKnob (c.removeFromLeft (kw), unisonSpread.slider, unisonSpread.label);
        }
        // FILTER
        {
            auto c = inset (sections[4].bounds);
            layoutCombo (c.removeFromTop (38), filterMode.combo, filterMode.label);
            const int kw = c.getWidth() / 3;
            layoutKnob (c.removeFromLeft (kw), filterCutoff.slider, filterCutoff.label);
            layoutKnob (c.removeFromLeft (kw), filterRes.slider,    filterRes.label);
            layoutKnob (c.removeFromLeft (kw), filterDrive.slider,  filterDrive.label);
        }
        // FILTER ENV
        {
            auto c = inset (sections[5].bounds);
            const int kw = c.getWidth() / 4;
            layoutKnob (c.removeFromLeft (kw), fEnvA.slider, fEnvA.label);
            layoutKnob (c.removeFromLeft (kw), fEnvD.slider, fEnvD.label);
            layoutKnob (c.removeFromLeft (kw), fEnvS.slider, fEnvS.label);
            layoutKnob (c.removeFromLeft (kw), fEnvR.slider, fEnvR.label);
        }
        // FILTER MOD
        {
            auto c = inset (sections[6].bounds);
            const int kw = c.getWidth() / 3;
            layoutKnob (c.removeFromLeft (kw), filterKeytrack.slider, filterKeytrack.label);
            layoutKnob (c.removeFromLeft (kw), filterEnvAmt.slider,   filterEnvAmt.label);
            layoutKnob (c.removeFromLeft (kw), filterVelAmt.slider,   filterVelAmt.label);
        }
        // LFO1
        {
            auto c = inset (sections[7].bounds);
            layoutCombo (c.removeFromTop (38), lfo1Shape.combo, lfo1Shape.label);
            const int kw = c.getWidth() / 5;
            layoutKnob (c.removeFromLeft (kw), lfo1Rate.slider,     lfo1Rate.label);
            layoutKnob (c.removeFromLeft (kw), lfo1ToCutoff.slider, lfo1ToCutoff.label);
            layoutKnob (c.removeFromLeft (kw), lfo1ToPitch.slider,  lfo1ToPitch.label);
            layoutKnob (c.removeFromLeft (kw), lfo1ToAmp.slider,    lfo1ToAmp.label);
            layoutKnob (c.removeFromLeft (kw), lfo1FadeIn.slider,   lfo1FadeIn.label);
        }
        // AMP ENV
        {
            auto c = inset (sections[8].bounds);
            const int kw = c.getWidth() / 4;
            layoutKnob (c.removeFromLeft (kw), ampA.slider, ampA.label);
            layoutKnob (c.removeFromLeft (kw), ampD.slider, ampD.label);
            layoutKnob (c.removeFromLeft (kw), ampS.slider, ampS.label);
            layoutKnob (c.removeFromLeft (kw), ampR.slider, ampR.label);
        }
        // CHORUS
        {
            auto c = inset (sections[9].bounds);
            layoutCombo (c.removeFromTop (38), chorusMode.combo, chorusMode.label);
            const int kw = c.getWidth() / 4;
            layoutKnob (c.removeFromLeft (kw), chorusMix.slider,      chorusMix.label);
            layoutKnob (c.removeFromLeft (kw), chorusRate.slider,     chorusRate.label);
            layoutKnob (c.removeFromLeft (kw), chorusDepth.slider,    chorusDepth.label);
            layoutKnob (c.removeFromLeft (kw), chorusFeedback.slider, chorusFeedback.label);
        }
        // DELAY
        {
            auto c = inset (sections[10].bounds);
            const int kw = c.getWidth() / 5;
            layoutKnob (c.removeFromLeft (kw), delayMix.slider,      delayMix.label);
            layoutKnob (c.removeFromLeft (kw), delayTimeL.slider,    delayTimeL.label);
            layoutKnob (c.removeFromLeft (kw), delayTimeR.slider,    delayTimeR.label);
            layoutKnob (c.removeFromLeft (kw), delayFB.slider,       delayFB.label);
            layoutKnob (c.removeFromLeft (kw), delayPingPong.slider, delayPingPong.label);
        }
        // REVERB
        {
            auto c = inset (sections[11].bounds);
            const int kw = c.getWidth() / 4;
            layoutKnob (c.removeFromLeft (kw), reverbMix.slider,     reverbMix.label);
            layoutKnob (c.removeFromLeft (kw), reverbSize.slider,    reverbSize.label);
            layoutKnob (c.removeFromLeft (kw), reverbDamping.slider, reverbDamping.label);
            layoutKnob (c.removeFromLeft (kw), reverbWidth.slider,   reverbWidth.label);
        }
        // MASTER
        {
            auto c = inset (sections[12].bounds);
            const int kw = c.getWidth() / 2;
            layoutKnob (c.removeFromLeft (kw), masterGain.slider, masterGain.label);
            layoutKnob (c.removeFromLeft (kw), masterPan.slider,  masterPan.label);
        }
        // PERF
        {
            auto c = inset (sections[13].bounds);
            layoutCombo (c.removeFromTop (38), polyMode.combo, polyMode.label);
        }
    }
}
