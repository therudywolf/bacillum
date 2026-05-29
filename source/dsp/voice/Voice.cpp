#include "dsp/voice/Voice.h"

#include <cmath>

namespace bacillum::dsp
{
    // Fractional MIDI note → frequency (A4 = note 69 = 440 Hz).
    static inline float noteToHz (float note) noexcept
    {
        return 440.0f * std::pow (2.0f, (note - 69.0f) * (1.0f / 12.0f));
    }

    void Voice::prepare(double sr) noexcept
    {
        sampleRate = static_cast<float>(sr);
        osc1.prepare(sr);
        osc2.prepare(sr);
        subOsc.prepare(sr);
        subOsc.setWaveform(params::Waveform::Sine);
        hyper1.prepare(sr);
        hyper2.prepare(sr);
        filter.prepare(sr);
        ladder.prepare(sr);
        amp.prepare(sr);
        fEnv.prepare(sr);
        lfo1.prepare(sr);
        dcBlock.prepare(sr);
        reset();
    }

    void Voice::reset() noexcept
    {
        osc1.reset();
        osc2.reset();
        subOsc.reset();
        hyper1.reset();
        hyper2.reset();
        whiteNoise.reset(0x9E3779B9u);
        pinkNoise.reset(0xC0FFEE13u);
        filter.reset();
        ladder.reset();
        amp.reset();
        fEnv.reset();
        lfo1.reset();
        dcBlock.reset();
        midiNote = -1;
        velocity = 0.0f;
        baseHz = 440.0f;
        pitchBendSemis = 0.0f;
        osc1OffsetSemis = osc2OffsetSemis = 0.0f;
        unisonCents = unisonPan = 0.0f;
        panLeft = panRight = juce::MathConstants<float>::sqrt2 * 0.5f;
        currentNote = targetNote = 60.0f;
        glideCoef = 1.0f;
        pitchModSemis = 0.0f;
        controlUpdateCounter = 0;
        startStamp = 0;
    }

    void Voice::applyFilterMode(params::FilterMode m) noexcept
    {
        switch (m)
        {
            case params::FilterMode::LP12:  useLadder = false; filter.setMode(SvfTpt::Mode::LP12);  break;
            case params::FilterMode::LP24:  useLadder = false; filter.setMode(SvfTpt::Mode::LP24);  break;
            case params::FilterMode::HP12:  useLadder = false; filter.setMode(SvfTpt::Mode::HP);    break;
            case params::FilterMode::BP12:  useLadder = false; filter.setMode(SvfTpt::Mode::BP);    break;
            case params::FilterMode::Notch: useLadder = false; filter.setMode(SvfTpt::Mode::Notch); break;
            case params::FilterMode::Peak:  useLadder = false; filter.setMode(SvfTpt::Mode::Peak);  break;
            case params::FilterMode::Ladder24: useLadder = true; ladder.setFourPole(true);  break;
            case params::FilterMode::Ladder12: useLadder = true; ladder.setFourPole(false); break;
            default:                        useLadder = false; filter.setMode(SvfTpt::Mode::LP12);  break;
        }
    }

    void Voice::setUnisonOffsets(float centsOffset, float panOffset) noexcept
    {
        unisonCents = centsOffset;
        unisonPan   = juce::jlimit(-1.0f, 1.0f, panOffset);
        recomputeOscFrequencies();
    }

    void Voice::glideFrom(float fromNoteFloat) noexcept
    {
        currentNote = fromNoteFloat;
    }

    void Voice::noteOn(int n, float vel) noexcept
    {
        midiNote = n;
        velocity = juce::jlimit(0.0f, 1.0f, vel);
        targetNote = static_cast<float>(n);
        if (glideCoef >= 0.999f)        // glide off → jump to pitch
            currentNote = targetNote;
        recomputeOscFrequencies();
        amp.noteOn();
        fEnv.noteOn();
        lfo1.retrigger();

        const std::uint32_t seed = 0x9E3779B9u
                                 ^ (static_cast<std::uint32_t>(n) * 2654435761u)
                                 ^ (static_cast<std::uint32_t>(velocity * 1000.0f) * 0xCA8B2A4Du);
        whiteNoise.reset(seed);
        pinkNoise.reset(seed ^ 0xDEADBEEFu);
    }

    void Voice::noteOff(float) noexcept
    {
        amp.noteOff();
        fEnv.noteOff();
    }

    void Voice::killFast() noexcept
    {
        amp.killFast();
        fEnv.killFast();
    }

    void Voice::setNote(int n) noexcept
    {
        midiNote = n;
        targetNote = static_cast<float>(n);
        if (glideCoef >= 0.999f)
            currentNote = targetNote;
        recomputeOscFrequencies();
    }

    void Voice::applyParams(const VoiceParams& p) noexcept
    {
        osc1Wave = p.osc1Waveform;
        osc2Wave = p.osc2Waveform;

        // HyperSaw shaping (applies to whichever OSC is in HyperSaw mode).
        hyper1.setDetune(p.hyperDetune);
        hyper1.setMix   (p.hyperMix);
        hyper2.setDetune(p.hyperDetune);
        hyper2.setMix   (p.hyperMix);

        // For non-HyperSaw OSC, configure the classic VA oscillator.
        if (osc1Wave != params::Waveform::HyperSaw)
            osc1.setWaveform(osc1Wave);
        osc1.setPulsewidth(p.osc1Pulsewidth);

        if (osc2Wave != params::Waveform::HyperSaw)
            osc2.setWaveform(osc2Wave);
        osc2.setPulsewidth(p.osc2Pulsewidth);

        // Sub osc waveform & octave.
        subWave = p.subWaveform;
        switch (p.subWaveform)
        {
            case params::SubWaveform::Sine:     subOsc.setWaveform(params::Waveform::Sine);     break;
            case params::SubWaveform::Triangle: subOsc.setWaveform(params::Waveform::Triangle); break;
            case params::SubWaveform::Square:   subOsc.setWaveform(params::Waveform::Square);   break;
            default: break;
        }
        subOctOffset = (p.subOctave == params::SubOctave::Minus2) ? -24 : -12;

        osc1Level  = p.osc1Level;
        osc2Level  = p.osc2Level;
        subLevel   = p.subLevel;
        noiseLevel = p.noiseLevel;
        noiseType  = p.noiseType;

        osc1OffsetSemis = p.osc1PitchSemi + p.osc1DetuneCents / 100.0f;
        osc2OffsetSemis = p.osc2PitchSemi + p.osc2DetuneCents / 100.0f;
        pitchBendSemis  = p.pitchBendSemis;
        recomputeOscFrequencies();

        applyFilterMode(p.filterMode);
        filterCutoffBase = p.filterCutoff;
        filter.setResonance01(p.filterRes01);
        ladder.setResonance01(p.filterRes01);
        filterDrive01    = p.filterDrive01;
        filterKeytrack   = p.filterKeytrack;
        filterEnvAmount  = p.filterEnvAmount;
        filterVelAmount  = p.filterVelAmount;

        // Glide coefficient per control step (one-pole approach).
        if (p.glideTime <= 0.001f)
        {
            glideCoef = 1.0f;
        }
        else
        {
            const float controlRate = sampleRate / static_cast<float>(kControlUpdateInterval);
            glideCoef = 1.0f - std::exp(-2.2f / (p.glideTime * controlRate));
            glideCoef = juce::jlimit(0.0001f, 1.0f, glideCoef);
        }

        amp.setAttack (p.ampA);
        amp.setDecay  (p.ampD);
        amp.setSustain(p.ampS);
        amp.setRelease(p.ampR);

        fEnv.setAttack (p.fEnvA);
        fEnv.setDecay  (p.fEnvD);
        fEnv.setSustain(p.fEnvS);
        fEnv.setRelease(p.fEnvR);

        lfo1.setShape(p.lfo1Shape);
        lfo1.setRateHz(p.lfo1RateHz);
        lfo1.setFadeInSec(p.lfo1FadeInSec);
        lfo1ToCutoffOct = p.lfo1ToCutoffOct;
        lfo1ToPitchSemi = p.lfo1ToPitchSemi;
        lfo1ToAmp01     = p.lfo1ToAmp01;
        modWheel01      = p.modWheel01;

        // Equal-power per-voice pan: pan = master + unison.
        const float pan = juce::jlimit(-1.0f, 1.0f, p.pan + unisonPan);
        const float a = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        panLeft  = std::cos(a);
        panRight = std::sin(a);
    }

    void Voice::recomputeOscFrequencies() noexcept
    {
        baseHz = noteToHz(currentNote);

        const float bend = pitchBendSemis + pitchModSemis;   // pitch wheel + LFO→pitch
        const float unisonSemis = unisonCents / 100.0f;
        const float maxHz = sampleRate * 0.49f;

        const float ratio1 = std::pow(2.0f, (osc1OffsetSemis + unisonSemis + bend) * (1.0f / 12.0f));
        const float f1 = juce::jlimit(0.01f, maxHz, baseHz * ratio1);
        osc1.setFrequency(f1);
        hyper1.setFrequency(f1);

        const float ratio2 = std::pow(2.0f, (osc2OffsetSemis + unisonSemis + bend) * (1.0f / 12.0f));
        const float f2 = juce::jlimit(0.01f, maxHz, baseHz * ratio2);
        osc2.setFrequency(f2);
        hyper2.setFrequency(f2);

        const float subRatio = std::pow(2.0f, (bend + (float) subOctOffset + unisonSemis) * (1.0f / 12.0f));
        subOsc.setFrequency(juce::jlimit(0.01f, maxHz, baseHz * subRatio));
    }

    void Voice::renderAdd(float* outL, float* outR, int startSample, int numSamples) noexcept
    {
        if (! amp.isActive())
            return;

        const float velGain = velocity * velocity;
        const float keytrackSemis = filterKeytrack * static_cast<float>(midiNote - 60);

        // Cache pan into locals (no atomic per-sample).
        const float pL = panLeft;
        const float pR = panRight;

        // Effective LFO depth scaled by mod wheel (fixed routing per spec).
        const float modWheelScale = modWheel01;  // 0..1 attenuator on LFO1
        const float depthCutoffOct = lfo1ToCutoffOct * modWheelScale;
        const float depthPitchSemi = lfo1ToPitchSemi * modWheelScale;
        const float depthAmp       = lfo1ToAmp01    * modWheelScale;

        // Pitch modulation is applied by recomputing OSC frequency periodically.
        // Keep a smoothed LFO sample for cutoff between updates.
        float lfo1Value = 0.0f;

        for (int n = 0; n < numSamples; ++n)
        {
            const int i = startSample + n;

            // Tick LFO once per sample so all rates are usable, but only
            // *apply* its value to frequency/cutoff every kControlUpdateInterval.
            lfo1Value = lfo1.tick();

            // ---- Source mix ---------------------------------------------
            const float o1 = (osc1Wave == params::Waveform::HyperSaw
                                ? hyper1.tick() : osc1.tick()) * osc1Level;
            const float o2 = (osc2Wave == params::Waveform::HyperSaw
                                ? hyper2.tick() : osc2.tick()) * osc2Level;
            const float os = subOsc.tick() * subLevel;
            const float on = (noiseType == params::NoiseType::White
                                ? whiteNoise.tick()
                                : pinkNoise.tick()) * noiseLevel;
            float src = o1 + o2 + os + on;

            // ---- Drive ---------------------------------------------------
            if (filterDrive01 > 0.001f)
            {
                const float pre  = 1.0f + filterDrive01 * 6.0f;
                const float comp = 1.0f / (1.0f + filterDrive01 * 1.5f);
                src = std::tanh(src * pre) * comp;
            }

            // ---- Control-rate updates (glide + cutoff + LFO→pitch) ------
            const float fe = fEnv.tick();
            if (controlUpdateCounter == 0)
            {
                // Glide: advance currentNote toward targetNote.
                if (glideCoef < 0.999f)
                {
                    currentNote += (targetNote - currentNote) * glideCoef;
                    if (std::abs(targetNote - currentNote) < 0.001f)
                        currentNote = targetNote;
                }

                // LFO1 → pitch, folded into the frequency recompute.
                pitchModSemis = lfo1Value * depthPitchSemi;
                recomputeOscFrequencies();

                // Cutoff modulation (octaves).
                const float envOct = filterEnvAmount * 5.0f * fe;
                const float velOct = filterVelAmount * 3.0f * velocity;
                const float ktOct  = keytrackSemis * (1.0f / 12.0f);
                const float lfoOct = lfo1Value * depthCutoffOct;
                const float cutoff = filterCutoffBase * std::pow(2.0f, envOct + velOct + ktOct + lfoOct);
                filter.setCutoff(cutoff);
                ladder.setCutoff(cutoff);
            }
            if (++controlUpdateCounter >= kControlUpdateInterval)
                controlUpdateCounter = 0;

            // ---- Filter (SVF or Moog ladder) ----------------------------
            const float filtered = useLadder ? ladder.process(src) : filter.process(src);

            // ---- VCA + tremolo ------------------------------------------
            const float env = amp.tick();
            // Tremolo: LFO -1..+1 → amplitude scale (1 - depth + depth * (lfo*0.5 + 0.5))
            const float tremolo = (depthAmp > 0.0f)
                ? juce::jlimit(0.0f, 2.0f, 1.0f - depthAmp + depthAmp * (lfo1Value * 0.5f + 0.5f))
                : 1.0f;

            const float voiceOut = dcBlock.process(filtered * env * velGain * tremolo);

            outL[i] += voiceOut * pL;
            outR[i] += voiceOut * pR;
        }
    }
}
