#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace bacillum
{
    PluginProcessor::PluginProcessor()
        : juce::AudioProcessor (BusesProperties()
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          apvts (*this, nullptr, "PARAMS", params::createLayout())
    {
        pMasterGain     = apvts.getRawParameterValue (params::ids::masterGain);
        pMasterPan      = apvts.getRawParameterValue (params::ids::masterPan);

        pOsc1Waveform   = apvts.getRawParameterValue (params::ids::osc1Waveform);
        pOsc1Pitch      = apvts.getRawParameterValue (params::ids::osc1Pitch);
        pOsc1Detune     = apvts.getRawParameterValue (params::ids::osc1Detune);
        pOsc1Pulsewidth = apvts.getRawParameterValue (params::ids::osc1Pulsewidth);
        pOsc1Level      = apvts.getRawParameterValue (params::ids::osc1Level);

        pOsc2Waveform   = apvts.getRawParameterValue (params::ids::osc2Waveform);
        pOsc2Pitch      = apvts.getRawParameterValue (params::ids::osc2Pitch);
        pOsc2Detune     = apvts.getRawParameterValue (params::ids::osc2Detune);
        pOsc2Pulsewidth = apvts.getRawParameterValue (params::ids::osc2Pulsewidth);
        pOsc2Level      = apvts.getRawParameterValue (params::ids::osc2Level);

        pSubLevel       = apvts.getRawParameterValue (params::ids::subLevel);
        pSubWaveform    = apvts.getRawParameterValue (params::ids::subWaveform);
        pSubOctave      = apvts.getRawParameterValue (params::ids::subOctave);
        pHyperDetune    = apvts.getRawParameterValue (params::ids::hyperDetune);
        pHyperMix       = apvts.getRawParameterValue (params::ids::hyperMix);
        pWavetablePos   = apvts.getRawParameterValue (params::ids::wavetablePos);
        pOscSync        = apvts.getRawParameterValue (params::ids::oscSync);
        pOscRing        = apvts.getRawParameterValue (params::ids::oscRing);
        pOscFM          = apvts.getRawParameterValue (params::ids::oscFM);
        pNoiseType      = apvts.getRawParameterValue (params::ids::noiseType);
        pNoiseLevel     = apvts.getRawParameterValue (params::ids::noiseLevel);

        pChorusMode     = apvts.getRawParameterValue (params::ids::chorusMode);
        pChorusMix      = apvts.getRawParameterValue (params::ids::chorusMix);
        pChorusRate     = apvts.getRawParameterValue (params::ids::chorusRate);
        pChorusDepth    = apvts.getRawParameterValue (params::ids::chorusDepth);
        pChorusFeedback = apvts.getRawParameterValue (params::ids::chorusFeedback);

        pGlideTime      = apvts.getRawParameterValue (params::ids::glideTime);
        pLfo1Sync       = apvts.getRawParameterValue (params::ids::lfo1Sync);
        pDelaySync      = apvts.getRawParameterValue (params::ids::delaySync);

        pArpOn          = apvts.getRawParameterValue (params::ids::arpOn);
        pArpMode        = apvts.getRawParameterValue (params::ids::arpMode);
        pArpRate        = apvts.getRawParameterValue (params::ids::arpRate);
        pArpOctaves     = apvts.getRawParameterValue (params::ids::arpOctaves);
        pArpGate        = apvts.getRawParameterValue (params::ids::arpGate);

        pLfo2Shape      = apvts.getRawParameterValue (params::ids::lfo2Shape);
        pLfo2Rate       = apvts.getRawParameterValue (params::ids::lfo2Rate);
        pLfo2Sync       = apvts.getRawParameterValue (params::ids::lfo2Sync);
        pLfo2FadeIn     = apvts.getRawParameterValue (params::ids::lfo2FadeIn);

        pLfo3Shape      = apvts.getRawParameterValue (params::ids::lfo3Shape);
        pLfo3Rate       = apvts.getRawParameterValue (params::ids::lfo3Rate);
        pLfo3Sync       = apvts.getRawParameterValue (params::ids::lfo3Sync);

        pEnv3Attack     = apvts.getRawParameterValue (params::ids::env3Attack);
        pEnv3Decay      = apvts.getRawParameterValue (params::ids::env3Decay);
        pEnv3Sustain    = apvts.getRawParameterValue (params::ids::env3Sustain);
        pEnv3Release    = apvts.getRawParameterValue (params::ids::env3Release);

        for (int i = 0; i < params::kNumModSlots; ++i)
        {
            pModSrc[i]   = apvts.getRawParameterValue (params::ids::modSrc[(size_t) i]);
            pModDst[i]   = apvts.getRawParameterValue (params::ids::modDst[(size_t) i]);
            pModDepth[i] = apvts.getRawParameterValue (params::ids::modDepth[(size_t) i]);
            pModCurve[i] = apvts.getRawParameterValue (params::ids::modCurve[(size_t) i]);
        }

        pFilterMode     = apvts.getRawParameterValue (params::ids::filterMode);
        pFilterCutoff   = apvts.getRawParameterValue (params::ids::filterCutoff);
        pFilterRes      = apvts.getRawParameterValue (params::ids::filterRes);
        pFilterDrive    = apvts.getRawParameterValue (params::ids::filterDrive);
        pFilterKeytrack = apvts.getRawParameterValue (params::ids::filterKeytrack);
        pFilterEnvAmt   = apvts.getRawParameterValue (params::ids::filterEnvAmt);
        pFilterVelAmt   = apvts.getRawParameterValue (params::ids::filterVelAmt);

        pFilterRouting  = apvts.getRawParameterValue (params::ids::filterRouting);
        pFilter2Mode    = apvts.getRawParameterValue (params::ids::filter2Mode);
        pFilter2Cutoff  = apvts.getRawParameterValue (params::ids::filter2Cutoff);
        pFilter2Res     = apvts.getRawParameterValue (params::ids::filter2Res);
        pSatType        = apvts.getRawParameterValue (params::ids::satType);
        pSatAmount      = apvts.getRawParameterValue (params::ids::satAmount);

        pFilterAttack   = apvts.getRawParameterValue (params::ids::filterAttack);
        pFilterDecay    = apvts.getRawParameterValue (params::ids::filterDecay);
        pFilterSustain  = apvts.getRawParameterValue (params::ids::filterSustain);
        pFilterRelease  = apvts.getRawParameterValue (params::ids::filterRelease);

        pAmpAttack      = apvts.getRawParameterValue (params::ids::ampAttack);
        pAmpDecay       = apvts.getRawParameterValue (params::ids::ampDecay);
        pAmpSustain     = apvts.getRawParameterValue (params::ids::ampSustain);
        pAmpRelease     = apvts.getRawParameterValue (params::ids::ampRelease);

        pLfo1Shape      = apvts.getRawParameterValue (params::ids::lfo1Shape);
        pLfo1Rate       = apvts.getRawParameterValue (params::ids::lfo1Rate);
        pLfo1FadeIn     = apvts.getRawParameterValue (params::ids::lfo1FadeIn);
        pLfo1ToCutoff   = apvts.getRawParameterValue (params::ids::lfo1ToCutoff);
        pLfo1ToPitch    = apvts.getRawParameterValue (params::ids::lfo1ToPitch);
        pLfo1ToAmp      = apvts.getRawParameterValue (params::ids::lfo1ToAmp);

        pUnisonCount    = apvts.getRawParameterValue (params::ids::unisonCount);
        pUnisonDetune   = apvts.getRawParameterValue (params::ids::unisonDetune);
        pUnisonSpread   = apvts.getRawParameterValue (params::ids::unisonSpread);

        pDelayMix       = apvts.getRawParameterValue (params::ids::delayMix);
        pDelayTimeL     = apvts.getRawParameterValue (params::ids::delayTimeL);
        pDelayTimeR     = apvts.getRawParameterValue (params::ids::delayTimeR);
        pDelayFeedback  = apvts.getRawParameterValue (params::ids::delayFeedback);
        pDelayPingPong  = apvts.getRawParameterValue (params::ids::delayPingPong);
        pDelayDamp      = apvts.getRawParameterValue (params::ids::delayDamp);

        pReverbMix      = apvts.getRawParameterValue (params::ids::reverbMix);
        pReverbSize     = apvts.getRawParameterValue (params::ids::reverbSize);
        pReverbDamping  = apvts.getRawParameterValue (params::ids::reverbDamping);
        pReverbWidth    = apvts.getRawParameterValue (params::ids::reverbWidth);

        pEqLowFreq      = apvts.getRawParameterValue (params::ids::eqLowFreq);
        pEqLowGain      = apvts.getRawParameterValue (params::ids::eqLowGain);
        pEqMidFreq      = apvts.getRawParameterValue (params::ids::eqMidFreq);
        pEqMidGain      = apvts.getRawParameterValue (params::ids::eqMidGain);
        pEqMidQ         = apvts.getRawParameterValue (params::ids::eqMidQ);
        pEqHighFreq     = apvts.getRawParameterValue (params::ids::eqHighFreq);
        pEqHighGain     = apvts.getRawParameterValue (params::ids::eqHighGain);

        pCompThresh     = apvts.getRawParameterValue (params::ids::compThresh);
        pCompRatio      = apvts.getRawParameterValue (params::ids::compRatio);
        pCompAttack     = apvts.getRawParameterValue (params::ids::compAttack);
        pCompRelease    = apvts.getRawParameterValue (params::ids::compRelease);
        pCompMakeup     = apvts.getRawParameterValue (params::ids::compMakeup);

        pPolyMode       = apvts.getRawParameterValue (params::ids::polyMode);
        pPitchBendRange = apvts.getRawParameterValue (params::ids::pitchBendRange);
    }

    bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
    {
        const auto& out = layouts.getMainOutputChannelSet();
        return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    }

    void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = static_cast<float>(sampleRate);
        voiceManager.prepare (sampleRate);
        arp.prepare (sampleRate);
        lfo3Global.prepare (sampleRate);
        eq.prepare (sampleRate);
        chorus.prepare (sampleRate, samplesPerBlock);
        delay.prepare (sampleRate, 4.0);
        reverb.prepare (sampleRate);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (juce::jmax (32, samplesPerBlock));
        spec.numChannels      = 2;
        comp.prepare (spec);
        limiter.prepare (spec);
        limiter.setThreshold (-0.3f);   // brick-wall safety
        limiter.setRelease (100.0f);

        vizBuffer.reset();
        workMidi.ensureSize (2048);   // pre-grow so processBlock never allocates

        masterGain.reset (sampleRate, 0.02);
        masterPanL.reset (sampleRate, 0.02);
        masterPanR.reset (sampleRate, 0.02);
        masterGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pMasterGain->load(), -60.0f));
        masterPanL.setCurrentAndTargetValue (juce::MathConstants<float>::sqrt2 * 0.5f);
        masterPanR.setCurrentAndTargetValue (juce::MathConstants<float>::sqrt2 * 0.5f);

        // Apply current unison once so voices spawn with the right config.
        applyUnisonSetting();
    }

    void PluginProcessor::applyUnisonSetting() noexcept
    {
        const int   count   = static_cast<int>(pUnisonCount->load());
        const float detune  = pUnisonDetune->load();
        const float spread  = pUnisonSpread->load();
        if (count != lastUnisonCount || detune != lastUnisonDetune || spread != lastUnisonSpread)
        {
            voiceManager.setUnison (count, detune, spread);
            lastUnisonCount  = count;
            lastUnisonDetune = detune;
            lastUnisonSpread = spread;
        }
    }

    void PluginProcessor::snapshotVoiceParams (dsp::VoiceParams& out) const noexcept
    {
        // OSC1
        out.osc1Waveform    = loadChoice<params::Waveform>(pOsc1Waveform, (int) params::Waveform::NumWaveforms);
        out.osc1PitchSemi   = pOsc1Pitch->load();
        out.osc1DetuneCents = pOsc1Detune->load();
        out.osc1Pulsewidth  = pOsc1Pulsewidth->load();
        out.osc1Level       = pOsc1Level->load();

        // OSC2
        out.osc2Waveform    = loadChoice<params::Waveform>(pOsc2Waveform, (int) params::Waveform::NumWaveforms);
        out.osc2PitchSemi   = pOsc2Pitch->load();
        out.osc2DetuneCents = pOsc2Detune->load();
        out.osc2Pulsewidth  = pOsc2Pulsewidth->load();
        out.osc2Level       = pOsc2Level->load();

        // Sub osc
        out.subLevel    = pSubLevel->load();
        out.subWaveform = loadChoice<params::SubWaveform>(pSubWaveform, (int) params::SubWaveform::NumSubWaveforms);
        out.subOctave   = loadChoice<params::SubOctave>  (pSubOctave,   (int) params::SubOctave::NumSubOctaves);

        // HyperSaw shaping
        out.hyperDetune = pHyperDetune->load();
        out.hyperMix    = pHyperMix->load();

        // Wavetable scan
        out.wavetablePos = pWavetablePos->load();

        // OSC interop
        out.oscSync = pOscSync->load() > 0.5f;
        out.oscRing = pOscRing->load();
        out.oscFM   = pOscFM->load();

        // Noise
        out.noiseType  = loadChoice<params::NoiseType>(pNoiseType, (int) params::NoiseType::NumNoiseTypes);
        out.noiseLevel = pNoiseLevel->load();

        // Filter
        out.filterMode      = loadChoice<params::FilterMode>(pFilterMode, (int) params::FilterMode::NumModes);
        out.filterCutoff    = pFilterCutoff->load();
        out.filterRes01     = pFilterRes->load();
        out.filterDrive01   = pFilterDrive->load();
        out.filterKeytrack  = pFilterKeytrack->load();
        out.filterEnvAmount = pFilterEnvAmt->load();
        out.filterVelAmount = pFilterVelAmt->load();

        // Filter 2 + routing + saturator
        out.filterRouting = loadChoice<params::FilterRouting>(pFilterRouting, (int) params::FilterRouting::NumRoutings);
        out.filter2Mode   = loadChoice<params::FilterMode>(pFilter2Mode, (int) params::FilterMode::NumModes);
        out.filter2Cutoff = pFilter2Cutoff->load();
        out.filter2Res01  = pFilter2Res->load();
        out.satType       = loadChoice<params::SaturatorType>(pSatType, (int) params::SaturatorType::NumTypes);
        out.satAmount     = pSatAmount->load();

        // Envs
        out.fEnvA = pFilterAttack->load();
        out.fEnvD = pFilterDecay->load();
        out.fEnvS = pFilterSustain->load();
        out.fEnvR = pFilterRelease->load();

        out.ampA = pAmpAttack->load();
        out.ampD = pAmpDecay->load();
        out.ampS = pAmpSustain->load();
        out.ampR = pAmpRelease->load();

        // LFO1 — translate params::LfoShape (int) → dsp::Lfo::Shape (int, same order).
        const int shapeInt = juce::jlimit (0, (int) params::LfoShape::NumShapes - 1,
                                           static_cast<int>(pLfo1Shape->load()));
        out.lfo1Shape      = static_cast<dsp::Lfo::Shape>(shapeInt);
        out.lfo1RateHz     = pLfo1Rate->load();
        out.lfo1FadeInSec  = pLfo1FadeIn->load();
        out.lfo1ToCutoffOct= pLfo1ToCutoff->load();
        out.lfo1ToPitchSemi= pLfo1ToPitch->load();
        out.lfo1ToAmp01    = pLfo1ToAmp->load();

        // LFO1 tempo sync: if a division is chosen, override the manual rate.
        const auto lfoSync = loadChoice<params::SyncDivision>(pLfo1Sync, (int) params::SyncDivision::NumDivisions);
        const float lfoBeats = params::syncBeats(lfoSync);
        if (lfoBeats > 0.0f)
        {
            const float secondsPerBeat = 60.0f / juce::jmax(1.0f, currentBpm);
            const float cycleSeconds   = lfoBeats * secondsPerBeat;
            out.lfo1RateHz = (cycleSeconds > 0.0001f) ? (1.0f / cycleSeconds) : out.lfo1RateHz;
        }

        // Glide
        out.glideTime = pGlideTime->load();

        // LFO2 (per-voice). Tempo sync overrides the manual rate.
        out.lfo2Shape = static_cast<dsp::Lfo::Shape>(
            juce::jlimit (0, (int) params::LfoShape::NumShapes - 1, static_cast<int>(pLfo2Shape->load())));
        out.lfo2RateHz    = pLfo2Rate->load();
        out.lfo2FadeInSec = pLfo2FadeIn->load();
        {
            const auto s = loadChoice<params::SyncDivision>(pLfo2Sync, (int) params::SyncDivision::NumDivisions);
            const float beats = params::syncBeats(s);
            if (beats > 0.0f)
            {
                const float cyc = beats * (60.0f / juce::jmax(1.0f, currentBpm));
                if (cyc > 0.0001f) out.lfo2RateHz = 1.0f / cyc;
            }
        }

        // ENV3
        out.env3A = pEnv3Attack->load();
        out.env3D = pEnv3Decay->load();
        out.env3S = pEnv3Sustain->load();
        out.env3R = pEnv3Release->load();

        // Mod matrix slots
        for (int i = 0; i < params::kNumModSlots; ++i)
        {
            out.modSlots[(size_t) i].source =
                static_cast<params::ModSource>(juce::jlimit (0, (int) params::ModSource::NumSources - 1,
                                                             static_cast<int>(pModSrc[i]->load())));
            out.modSlots[(size_t) i].dest =
                static_cast<params::ModDest>(juce::jlimit (0, (int) params::ModDest::NumDests - 1,
                                                           static_cast<int>(pModDst[i]->load())));
            out.modSlots[(size_t) i].depth = pModDepth[i]->load();
            out.modSlots[(size_t) i].curve =
                static_cast<params::ModCurve>(juce::jlimit (0, (int) params::ModCurve::NumCurves - 1,
                                                            static_cast<int>(pModCurve[i]->load())));
        }

        // Mod sources from MIDI / GUI / global engine
        out.modWheel01     = currentModWheel01;
        out.aftertouch01   = currentAftertouch01;
        out.pitchBendNorm  = currentPitchBendNorm;
        out.lfo3Value      = currentLfo3Value;
        const float pbRange = pPitchBendRange->load();
        out.pitchBendSemis = currentPitchBendNorm * pbRange;

        // Per-voice pan = master pan + unison
        out.pan            = juce::jlimit(-1.0f, 1.0f, pMasterPan->load());
    }

    void PluginProcessor::handleMidiEvent (const juce::MidiMessage& m) noexcept
    {
        if (m.isNoteOn())
        {
            voiceManager.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        }
        else if (m.isNoteOff())
        {
            voiceManager.noteOff (m.getNoteNumber(), m.getFloatVelocity());
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            voiceManager.allNotesOff (m.isAllSoundOff());
        }
        else if (m.isResetAllControllers())
        {
            voiceManager.panic();
            currentModWheel01 = 0.0f;
            currentAftertouch01 = 0.0f;
            currentPitchBendNorm = 0.0f;
        }
        else if (m.isPitchWheel())
        {
            currentPitchBendNorm = (m.getPitchWheelValue() - 8192) / 8192.0f;
        }
        else if (m.isChannelPressure())
        {
            currentAftertouch01 = m.getChannelPressureValue() / 127.0f;
        }
        else if (m.isController())
        {
            const int cc  = m.getControllerNumber();
            const int val = m.getControllerValue();
            switch (cc)
            {
                case 1:   currentModWheel01 = val / 127.0f; break;   // mod wheel
                case 64:  voiceManager.sustainPedal (val >= 64); break;
                case 120: voiceManager.allNotesOff (true);   break;  // all sound off
                case 121: voiceManager.panic();              break;  // reset
                case 123: voiceManager.allNotesOff (false);  break;  // all notes off
                default: break;
            }
        }
    }

    void PluginProcessor::renderSubBlock (juce::AudioBuffer<float>& buffer, int start, int numSamples)
    {
        if (numSamples <= 0) return;

        applyUnisonSetting();

        dsp::VoiceParams vp;
        snapshotVoiceParams (vp);
        voiceManager.setPolyMode (loadChoice<params::PolyMode>(pPolyMode, (int) params::PolyMode::NumModes));
        voiceManager.setParams (vp);

        const int numChans = buffer.getNumChannels();
        float* L = buffer.getWritePointer (0);
        float* R = numChans > 1 ? buffer.getWritePointer (1) : L;
        voiceManager.render (L, R, start, numSamples);

        // Publish live modulation for the editor's knob rings.
        {
            float c = 12000.0f, r = 0.1f; bool a = false;
            voiceManager.getViz (c, r, a);
            if (a) { vizCutoffHz.store (c); vizRes01.store (r); }
            vizActive.store (a);
        }

        // The voice already applies its own pan; master pan is rolled into per-voice
        // via VoiceParams::pan, so the bus only needs master gain smoothing.
        const float targetGain = juce::Decibels::decibelsToGain (pMasterGain->load(), -60.0f);
        masterGain.setTargetValue (targetGain);

        for (int n = 0; n < numSamples; ++n)
        {
            const int i = start + n;
            const float g = masterGain.getNextValue();
            if (numChans > 1)
            {
                L[i] *= g;
                R[i] *= g;
            }
            else
            {
                L[i] *= g;
            }
        }
    }

    void PluginProcessor::applyFxBus (juce::AudioBuffer<float>& buffer, int start, int numSamples)
    {
        if (numSamples <= 0) return;
        const int numChans = buffer.getNumChannels();
        if (numChans == 0)   return;

        float* L = buffer.getWritePointer (0);
        float* R = numChans > 1 ? buffer.getWritePointer (1) : L;

        // --- 3-band EQ (front of FX chain) ---
        eq.setParams (pEqLowFreq->load(), pEqLowGain->load(),
                      pEqMidFreq->load(), pEqMidGain->load(), pEqMidQ->load(),
                      pEqHighFreq->load(), pEqHighGain->load());
        if (numChans > 1)
            eq.process (L + start, R + start, numSamples);

        // --- Chorus / Flanger / Phaser (insert before time-based FX) ---
        chorus.setMode    (loadChoice<params::ChorusMode>(pChorusMode, (int) params::ChorusMode::NumChorusModes));
        chorus.setRate    (pChorusRate->load());
        chorus.setDepth   (pChorusDepth->load());
        chorus.setFeedback(pChorusFeedback->load());
        chorus.setMix     (pChorusMix->load());
        if (numChans > 1)
            chorus.process (L + start, R + start, numSamples);

        // --- Delay ---
        const auto delSync = loadChoice<params::SyncDivision>(pDelaySync, (int) params::SyncDivision::NumDivisions);
        const float delBeats = params::syncBeats(delSync);
        if (delBeats > 0.0f)
        {
            const float spb = 60.0f / juce::jmax(1.0f, currentBpm);
            const float t   = delBeats * spb;
            delay.setTimes (t, t);
        }
        else
        {
            delay.setTimes (pDelayTimeL->load(), pDelayTimeR->load());
        }
        delay.setFeedback   (pDelayFeedback->load());
        delay.setCrossFeedback (pDelayPingPong->load());
        delay.setDampingCutoff (pDelayDamp->load());
        delay.setMix        (pDelayMix->load());
        delay.process (L + start, R + start, numSamples);

        // --- Reverb (Dattorro plate) ---
        reverb.setSize    (pReverbSize->load());
        reverb.setDamping (pReverbDamping->load());
        reverb.setWidth   (pReverbWidth->load());
        reverb.setMix     (pReverbMix->load());
        reverb.process (L + start, R + start, numSamples);

        // --- Compressor → makeup → brick-wall limiter (end of chain) ---
        {
            juce::dsp::AudioBlock<float> block (buffer);
            auto sub = block.getSubBlock (static_cast<size_t> (start), static_cast<size_t> (numSamples));
            juce::dsp::ProcessContextReplacing<float> ctx (sub);

            comp.setThreshold (pCompThresh->load());
            comp.setRatio     (juce::jmax (1.0f, pCompRatio->load()));
            comp.setAttack    (pCompAttack->load());
            comp.setRelease   (pCompRelease->load());
            comp.process (ctx);

            const float mk = juce::Decibels::decibelsToGain (pCompMakeup->load());
            if (std::abs (mk - 1.0f) > 1.0e-4f)
                for (int n = 0; n < numSamples; ++n)
                {
                    L[start + n] *= mk;
                    if (numChans > 1) R[start + n] *= mk;
                }

            limiter.process (ctx);
        }

        // ---- Visualisation feed: push the post-FX signal to the lock-free
        // ring so the editor's oscilloscope + spectrum analyser can render it.
        if (numChans > 1)
            vizBuffer.push (L + start, R + start, numSamples);
        else
            vizBuffer.push (L + start, L + start, numSamples);
    }

    void PluginProcessor::renderFromMidi (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midi, int numSamples)
    {
        int cursor = 0;
        for (const auto meta : midi)
        {
            const int eventTime = juce::jlimit (0, numSamples, meta.samplePosition);
            if (eventTime > cursor)
            {
                renderSubBlock (buffer, cursor, eventTime - cursor);
                cursor = eventTime;
            }
            handleMidiEvent (meta.getMessage());
        }
        if (cursor < numSamples)
            renderSubBlock (buffer, cursor, numSamples - cursor);
    }

    void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples = buffer.getNumSamples();
        const int numChans   = buffer.getNumChannels();

        for (int ch = 0; ch < numChans; ++ch)
            buffer.clear (ch, 0, numSamples);

        // Host tempo for synced LFO / delay / arp. Falls back to 120 BPM.
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto bpm = pos->getBpm())
                    currentBpm = static_cast<float>(*bpm);

        // Global LFO3: advance once per block (block-rate), value shared by all voices.
        lfo3Global.setShape (static_cast<dsp::Lfo::Shape>(
            juce::jlimit (0, (int) params::LfoShape::NumShapes - 1, static_cast<int>(pLfo3Shape->load()))));
        {
            float rate = pLfo3Rate->load();
            const auto s = loadChoice<params::SyncDivision>(pLfo3Sync, (int) params::SyncDivision::NumDivisions);
            const float beats = params::syncBeats(s);
            if (beats > 0.0f)
            {
                const float cyc = beats * (60.0f / juce::jmax(1.0f, currentBpm));
                if (cyc > 0.0001f) rate = 1.0f / cyc;
            }
            lfo3Global.setRateHz (rate);
        }
        for (int i = 0; i < numSamples; ++i)
            currentLfo3Value = lfo3Global.tick();

        // Inject MIDI from on-screen / PC keyboard into the incoming stream.
        keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

        // --- Arpeggiator: transform the MIDI stream before rendering -------
        const bool arpOn = pArpOn->load() > 0.5f;
        arp.setEnabled (arpOn);

        workMidi.clear();

        if (arpOn)
        {
            arp.setParams (loadChoice<params::ArpMode>(pArpMode, (int) params::ArpMode::NumModes),
                           params::syncBeats (loadChoice<params::SyncDivision>(
                                pArpRate, (int) params::SyncDivision::NumDivisions)),
                           static_cast<int>(pArpOctaves->load()),
                           pArpGate->load(),
                           currentBpm);

            for (const auto meta : midi)
            {
                const auto msg = meta.getMessage();
                if (msg.isNoteOn())
                    arp.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
                else if (msg.isNoteOff())
                    arp.noteOff (msg.getNoteNumber());
                else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                {
                    arp.allNotesOff();
                    workMidi.addEvent (msg, meta.samplePosition);  // still stop voices
                }
                else
                    workMidi.addEvent (msg, meta.samplePosition);  // CC / bend / AT pass-through
            }
            arp.process (workMidi, numSamples);   // injects arp note on/off
        }
        else
        {
            workMidi.addEvents (midi, 0, numSamples, 0);
            arp.process (workMidi, numSamples);   // flushes a leftover arp note-off, if any
        }

        renderFromMidi (buffer, workMidi, numSamples);

        // FX bus runs once over the whole block — stable delay / reverb behaviour.
        applyFxBus (buffer, 0, numSamples);

        midi.clear();
    }

    juce::AudioProcessorEditor* PluginProcessor::createEditor()
    {
        return new PluginEditor (*this);
    }

    void PluginProcessor::getStateInformation (juce::MemoryBlock& dest)
    {
        auto state = apvts.copyState();
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
    }

    void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
    {
        if (auto xml = getXmlFromBinary (data, sizeInBytes))
            if (xml->hasTagName (apvts.state.getType()))
                apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new bacillum::PluginProcessor();
}
