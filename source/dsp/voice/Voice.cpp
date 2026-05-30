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
        wt1.prepare(sr);
        wt2.prepare(sr);
        filter.prepare(sr);
        ladder.prepare(sr);
        filter2.prepare(sr);
        ladder2.prepare(sr);
        amp.prepare(sr);
        fEnv.prepare(sr);
        env3.prepare(sr);
        lfo1.prepare(sr);
        lfo2.prepare(sr);
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
        wt1.reset();
        wt2.reset();
        whiteNoise.reset(0x9E3779B9u);
        pinkNoise.reset(0xC0FFEE13u);
        filter.reset();
        ladder.reset();
        filter2.reset();
        ladder2.reset();
        saturator.reset();
        amp.reset();
        fEnv.reset();
        env3.reset();
        lfo1.reset();
        lfo2.reset();
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
        osc2ModSemis = 0.0f;
        randomValue = 0.0f;
        matrixAmp = 1.0f;
        controlUpdateCounter = 0;
        startStamp = 0;
    }

    void Voice::applyFilterMode(params::FilterMode m, SvfTpt& f, MoogLadder& l, bool& useL) noexcept
    {
        switch (m)
        {
            case params::FilterMode::LP12:  useL = false; f.setMode(SvfTpt::Mode::LP12);  break;
            case params::FilterMode::LP24:  useL = false; f.setMode(SvfTpt::Mode::LP24);  break;
            case params::FilterMode::HP12:  useL = false; f.setMode(SvfTpt::Mode::HP);    break;
            case params::FilterMode::BP12:  useL = false; f.setMode(SvfTpt::Mode::BP);    break;
            case params::FilterMode::Notch: useL = false; f.setMode(SvfTpt::Mode::Notch); break;
            case params::FilterMode::Peak:  useL = false; f.setMode(SvfTpt::Mode::Peak);  break;
            case params::FilterMode::Ladder24: useL = true; l.setFourPole(true);  break;
            case params::FilterMode::Ladder12: useL = true; l.setFourPole(false); break;
            default:                        useL = false; f.setMode(SvfTpt::Mode::LP12);  break;
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
        env3.noteOn();
        lfo1.retrigger();
        lfo2.retrigger();

        const std::uint32_t seed = 0x9E3779B9u
                                 ^ (static_cast<std::uint32_t>(n) * 2654435761u)
                                 ^ (static_cast<std::uint32_t>(velocity * 1000.0f) * 0xCA8B2A4Du);
        whiteNoise.reset(seed);
        pinkNoise.reset(seed ^ 0xDEADBEEFu);

        // Per-note random source: deterministic hash of the seed → [-1, +1].
        randomValue = static_cast<float>((seed >> 9) & 0xFFFFu) * (1.0f / 32768.0f) - 1.0f;
    }

    void Voice::noteOff(float) noexcept
    {
        amp.noteOff();
        fEnv.noteOff();
        env3.noteOff();
    }

    void Voice::killFast() noexcept
    {
        amp.killFast();
        fEnv.killFast();
        env3.killFast();
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

        // Wavetable scan (applies to whichever OSC is in Wavetable mode).
        wt1.setPosition(p.wavetablePos);
        wt2.setPosition(p.wavetablePos);

        // OSC interop.
        oscSync = p.oscSync;
        oscRing = p.oscRing;
        oscFM   = p.oscFM;

        // For non-HyperSaw OSC, configure the classic VA oscillator.
        if (osc1Wave != params::Waveform::HyperSaw)
            osc1.setWaveform(osc1Wave);
        if (osc2Wave != params::Waveform::HyperSaw)
            osc2.setWaveform(osc2Wave);

        osc1PWBase = p.osc1Pulsewidth;
        osc2PWBase = p.osc2Pulsewidth;
        osc1.setPulsewidth(osc1PWBase);
        osc2.setPulsewidth(osc2PWBase);

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

        // Source levels — base values; the matrix modulates around them.
        osc1LevelBase = p.osc1Level;   osc1Level  = osc1LevelBase;
        osc2LevelBase = p.osc2Level;   osc2Level  = osc2LevelBase;
        subLevelBase  = p.subLevel;    subLevel   = subLevelBase;
        noiseLevelBase= p.noiseLevel;  noiseLevel = noiseLevelBase;
        noiseType  = p.noiseType;

        osc1OffsetSemis = p.osc1PitchSemi + p.osc1DetuneCents / 100.0f;
        osc2OffsetSemis = p.osc2PitchSemi + p.osc2DetuneCents / 100.0f;
        pitchBendSemis  = p.pitchBendSemis;
        recomputeOscFrequencies();

        applyFilterMode(p.filterMode, filter, ladder, useLadder);
        filterCutoffBase  = p.filterCutoff;
        filterRes01Base   = p.filterRes01;
        filter.setResonance01(filterRes01Base);
        ladder.setResonance01(filterRes01Base);
        filterDrive01Base = p.filterDrive01;
        filterDrive01     = filterDrive01Base;
        filterKeytrack    = p.filterKeytrack;
        filterEnvAmount   = p.filterEnvAmount;
        filterVelAmount   = p.filterVelAmount;

        // Filter 2 + routing + saturator.
        routing = p.filterRouting;
        applyFilterMode(p.filter2Mode, filter2, ladder2, useLadder2);
        filter2CutoffBase = p.filter2Cutoff;
        filter2Res01Base  = p.filter2Res01;
        filter2.setResonance01(filter2Res01Base);
        ladder2.setResonance01(filter2Res01Base);
        saturator.setType  (p.satType);
        saturator.setAmount(p.satAmount);

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

        env3.setAttack (p.env3A);
        env3.setDecay  (p.env3D);
        env3.setSustain(p.env3S);
        env3.setRelease(p.env3R);

        lfo1.setShape(p.lfo1Shape);
        lfo1RateBase = p.lfo1RateHz;
        lfo1.setRateHz(lfo1RateBase);
        lfo1.setFadeInSec(p.lfo1FadeInSec);
        lfo1ToCutoffOct = p.lfo1ToCutoffOct;
        lfo1ToPitchSemi = p.lfo1ToPitchSemi;
        lfo1ToAmp01     = p.lfo1ToAmp01;

        lfo2.setShape(p.lfo2Shape);
        lfo2RateBase = p.lfo2RateHz;
        lfo2.setRateHz(lfo2RateBase);
        lfo2.setFadeInSec(p.lfo2FadeInSec);

        // Matrix + source scalars.
        modSlots       = p.modSlots;
        modWheel01     = p.modWheel01;
        pitchBendNorm  = p.pitchBendNorm;
        aftertouch01   = p.aftertouch01;
        lfo3Value      = p.lfo3Value;

        // Equal-power per-voice pan: base = master + unison (matrix adds at control rate).
        panBase = p.pan;
        const float pan = juce::jlimit(-1.0f, 1.0f, panBase + unisonPan);
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
        wt1.setFrequency(f1);

        const float ratio2 = std::pow(2.0f, (osc2OffsetSemis + osc2ModSemis + unisonSemis + bend) * (1.0f / 12.0f));
        const float f2 = juce::jlimit(0.01f, maxHz, baseHz * ratio2);
        osc2.setFrequency(f2);
        hyper2.setFrequency(f2);
        wt2.setFrequency(f2);

        const float subRatio = std::pow(2.0f, (bend + (float) subOctOffset + unisonSemis) * (1.0f / 12.0f));
        subOsc.setFrequency(juce::jlimit(0.01f, maxHz, baseHz * subRatio));
    }

    void Voice::renderAdd(float* outL, float* outR, int startSample, int numSamples) noexcept
    {
        if (! amp.isActive())
            return;

        const float velGain = velocity * velocity;
        const float keytrackSemis = filterKeytrack * static_cast<float>(midiNote - 60);

        // Dedicated LFO1 routings; the mod wheel ADDS vibrato (Virus/Nord style).
        constexpr float kModWheelVibratoSemis = 0.5f;
        const float depthCutoffOct = lfo1ToCutoffOct;
        const float depthPitchSemi = lfo1ToPitchSemi + modWheel01 * kModWheelVibratoSemis;
        const float depthAmp       = lfo1ToAmp01;

        float lfo1Value = 0.0f;
        float lfo2Value = 0.0f;

        // Filter helpers (read members at call time, so control-rate changes apply).
        auto driveFn = [this](float x) -> float
        {
            if (filterDrive01 > 0.001f)
            {
                const float pre  = 1.0f + filterDrive01 * 6.0f;
                const float comp = 1.0f / (1.0f + filterDrive01 * 1.5f);
                return std::tanh(x * pre) * comp;
            }
            return x;
        };
        auto f1 = [this](float x) { return useLadder  ? ladder.process(x)  : filter.process(x);  };
        auto f2 = [this](float x) { return useLadder2 ? ladder2.process(x) : filter2.process(x); };

        for (int n = 0; n < numSamples; ++n)
        {
            const int i = startSample + n;

            lfo1Value = lfo1.tick();
            lfo2Value = lfo2.tick();

            // ---- Source mix with OSC interop ----------------------------
            // OSC2 (modulator) ticks first so it can hard-sync / FM OSC1.
            const bool osc2Classic = (osc2Wave != params::Waveform::HyperSaw
                                      && osc2Wave != params::Waveform::Wavetable);
            const float o2raw = (osc2Wave == params::Waveform::HyperSaw)  ? hyper2.tick()
                              : (osc2Wave == params::Waveform::Wavetable) ? wt2.tick()
                              : osc2.tick();

            const bool osc1Classic = (osc1Wave != params::Waveform::HyperSaw
                                      && osc1Wave != params::Waveform::Wavetable);

            // Hard sync: OSC2 wrap resets OSC1 phase (classic carriers only).
            if (oscSync && osc1Classic && osc2Classic && osc2.justWrapped())
                osc1.setPhase(0.0f);

            // FM / phase-mod: OSC2 → OSC1 (classic carrier only).
            if (osc1Classic)
                osc1.setPhaseMod(oscFM > 0.0001f ? o2raw * oscFM * 0.5f : 0.0f);

            const float o1raw = (osc1Wave == params::Waveform::HyperSaw)  ? hyper1.tick()
                              : (osc1Wave == params::Waveform::Wavetable) ? wt1.tick()
                              : osc1.tick();

            const float o1 = o1raw * osc1Level;
            const float o2 = o2raw * osc2Level;
            const float os = subOsc.tick() * subLevel;
            const float on = (noiseType == params::NoiseType::White
                                ? whiteNoise.tick()
                                : pinkNoise.tick()) * noiseLevel;
            const float ringS = (oscRing > 0.0001f) ? (o1raw * o2raw * oscRing) : 0.0f;

            const float grpA = o1 + os;          // OSC1 + sub   → filter 1 in Split
            const float grpB = o2 + on + ringS;  // OSC2 + noise + ring → filter 2 in Split

            const float fe = fEnv.tick();
            const float e3 = env3.tick();

            // ---- Control-rate block: glide + mod matrix + all dests -----
            if (controlUpdateCounter == 0)
            {
                if (glideCoef < 0.999f)
                {
                    currentNote += (targetNote - currentNote) * glideCoef;
                    if (std::abs(targetNote - currentNote) < 0.001f)
                        currentNote = targetNote;
                }

                // Assemble mod sources (index by ModSource enum order).
                constexpr int kNumSrc = static_cast<int>(params::ModSource::NumSources);
                float sv[kNumSrc] = { 0.0f };
                sv[(int) params::ModSource::Env1]       = fe;
                sv[(int) params::ModSource::Env2]       = amp.getLevel();
                sv[(int) params::ModSource::Env3]       = e3;
                sv[(int) params::ModSource::Lfo1]       = lfo1Value;
                sv[(int) params::ModSource::Lfo2]       = lfo2Value;
                sv[(int) params::ModSource::Lfo3]       = lfo3Value;
                sv[(int) params::ModSource::Velocity]   = velocity;
                sv[(int) params::ModSource::Note]       = juce::jlimit(-1.0f, 1.0f,
                                                              static_cast<float>(midiNote - 60) * (1.0f / 24.0f));
                sv[(int) params::ModSource::ModWheel]   = modWheel01;
                sv[(int) params::ModSource::PitchBend]  = pitchBendNorm;
                sv[(int) params::ModSource::Aftertouch] = aftertouch01;
                sv[(int) params::ModSource::Random]     = randomValue;
                sv[(int) params::ModSource::Constant]   = 1.0f;

                // Accumulate per destination.
                float dCut = 0, dRes = 0, dDrive = 0, dPitch = 0, dOsc2 = 0;
                float dPw1 = 0, dPw2 = 0, dL1 = 0, dL2 = 0, dSub = 0, dNz = 0;
                float dPan = 0, dAmp = 0, dR1 = 0, dR2 = 0, dCut2 = 0, dRes2 = 0;

                for (const auto& slot : modSlots)
                {
                    if (slot.source == params::ModSource::None
                        || slot.dest == params::ModDest::None
                        || slot.depth == 0.0f)
                        continue;

                    const float v = params::applyModCurve (slot.curve, sv[(int) slot.source]) * slot.depth;
                    switch (slot.dest)
                    {
                        case params::ModDest::Cutoff:     dCut   += v; break;
                        case params::ModDest::Resonance:  dRes   += v; break;
                        case params::ModDest::Drive:      dDrive += v; break;
                        case params::ModDest::Pitch:      dPitch += v; break;
                        case params::ModDest::Osc2Pitch:  dOsc2  += v; break;
                        case params::ModDest::Osc1PW:     dPw1   += v; break;
                        case params::ModDest::Osc2PW:     dPw2   += v; break;
                        case params::ModDest::Osc1Level:  dL1    += v; break;
                        case params::ModDest::Osc2Level:  dL2    += v; break;
                        case params::ModDest::SubLevel:   dSub   += v; break;
                        case params::ModDest::NoiseLevel: dNz    += v; break;
                        case params::ModDest::Pan:        dPan   += v; break;
                        case params::ModDest::Amp:        dAmp   += v; break;
                        case params::ModDest::Lfo1Rate:   dR1    += v; break;
                        case params::ModDest::Lfo2Rate:   dR2    += v; break;
                        case params::ModDest::Cutoff2:    dCut2  += v; break;
                        case params::ModDest::Reso2:      dRes2  += v; break;
                        default: break;
                    }
                }

                // Pitch: LFO1 vibrato + matrix (1 unit = 12 semitones).
                pitchModSemis = lfo1Value * depthPitchSemi + dPitch * 12.0f;
                osc2ModSemis  = dOsc2 * 12.0f;
                recomputeOscFrequencies();

                // Cutoff (octaves): env + vel + keytrack + LFO1 + matrix (1 unit = 5 oct).
                const float envOct = filterEnvAmount * 5.0f * fe;
                const float velOct = filterVelAmount * 3.0f * velocity;
                const float ktOct  = keytrackSemis * (1.0f / 12.0f);
                const float lfoOct = lfo1Value * depthCutoffOct;
                const float modOctCommon = envOct + velOct + ktOct + lfoOct;
                const float cutoff = filterCutoffBase * std::pow(2.0f, modOctCommon + dCut * 5.0f);
                filter.setCutoff(cutoff);
                ladder.setCutoff(cutoff);

                // Filter 2 follows the same env/keytrack/LFO offset around its own base.
                const float cutoff2 = filter2CutoffBase * std::pow(2.0f, modOctCommon + dCut2 * 5.0f);
                filter2.setCutoff(cutoff2);
                ladder2.setCutoff(cutoff2);
                const float res2Eff = juce::jlimit(0.0f, 1.0f, filter2Res01Base + dRes2);
                filter2.setResonance01(res2Eff);
                ladder2.setResonance01(res2Eff);

                // Resonance / drive (filter 1).
                const float resEff = juce::jlimit(0.0f, 1.0f, filterRes01Base + dRes);
                filter.setResonance01(resEff);
                ladder.setResonance01(resEff);
                filterDrive01 = juce::jlimit(0.0f, 1.0f, filterDrive01Base + dDrive);

                // Source levels.
                osc1Level  = juce::jlimit(0.0f, 1.5f, osc1LevelBase  + dL1);
                osc2Level  = juce::jlimit(0.0f, 1.5f, osc2LevelBase  + dL2);
                subLevel   = juce::jlimit(0.0f, 1.5f, subLevelBase   + dSub);
                noiseLevel = juce::jlimit(0.0f, 1.5f, noiseLevelBase + dNz);

                // Pulse width.
                osc1.setPulsewidth(juce::jlimit(0.05f, 0.95f, osc1PWBase + dPw1 * 0.45f));
                osc2.setPulsewidth(juce::jlimit(0.05f, 0.95f, osc2PWBase + dPw2 * 0.45f));

                // Pan (equal-power).
                const float panEff = juce::jlimit(-1.0f, 1.0f, panBase + unisonPan + dPan);
                const float a = (panEff + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
                panLeft  = std::cos(a);
                panRight = std::sin(a);

                // LFO rate scaling (±2 octaves) and amp matrix gain.
                lfo1.setRateHz(lfo1RateBase * std::pow(2.0f, dR1 * 2.0f));
                lfo2.setRateHz(lfo2RateBase * std::pow(2.0f, dR2 * 2.0f));
                matrixAmp = juce::jlimit(0.0f, 2.0f, 1.0f + dAmp);
            }
            if (++controlUpdateCounter >= kControlUpdateInterval)
                controlUpdateCounter = 0;

            // ---- Filter routing + saturator -----------------------------
            float filtered;
            switch (routing)
            {
                case params::FilterRouting::Serial:
                    filtered = f2(saturator.process(f1(driveFn(grpA + grpB))));
                    break;
                case params::FilterRouting::Parallel:
                {
                    const float s = driveFn(grpA + grpB);
                    filtered = saturator.process(0.5f * (f1(s) + f2(s)));
                    break;
                }
                case params::FilterRouting::Split:
                    filtered = saturator.process(f1(driveFn(grpA)) + f2(driveFn(grpB)));
                    break;
                case params::FilterRouting::Single:
                default:
                    filtered = saturator.process(f1(driveFn(grpA + grpB)));
                    break;
            }

            // ---- VCA: amp env × velocity × LFO1 tremolo × matrix amp ----
            const float env = amp.tick();
            const float tremolo = (depthAmp > 0.0f)
                ? juce::jlimit(0.0f, 2.0f, 1.0f - depthAmp + depthAmp * (lfo1Value * 0.5f + 0.5f))
                : 1.0f;

            const float voiceOut = dcBlock.process(filtered * env * velGain * tremolo * matrixAmp);

            outL[i] += voiceOut * panLeft;
            outR[i] += voiceOut * panRight;
        }
    }
}
