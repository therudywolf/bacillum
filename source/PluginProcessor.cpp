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
        pNoiseType      = apvts.getRawParameterValue (params::ids::noiseType);
        pNoiseLevel     = apvts.getRawParameterValue (params::ids::noiseLevel);

        pChorusMode     = apvts.getRawParameterValue (params::ids::chorusMode);
        pChorusMix      = apvts.getRawParameterValue (params::ids::chorusMix);
        pChorusRate     = apvts.getRawParameterValue (params::ids::chorusRate);
        pChorusDepth    = apvts.getRawParameterValue (params::ids::chorusDepth);
        pChorusFeedback = apvts.getRawParameterValue (params::ids::chorusFeedback);

        pFilterMode     = apvts.getRawParameterValue (params::ids::filterMode);
        pFilterCutoff   = apvts.getRawParameterValue (params::ids::filterCutoff);
        pFilterRes      = apvts.getRawParameterValue (params::ids::filterRes);
        pFilterDrive    = apvts.getRawParameterValue (params::ids::filterDrive);
        pFilterKeytrack = apvts.getRawParameterValue (params::ids::filterKeytrack);
        pFilterEnvAmt   = apvts.getRawParameterValue (params::ids::filterEnvAmt);
        pFilterVelAmt   = apvts.getRawParameterValue (params::ids::filterVelAmt);

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
        chorus.prepare (sampleRate, samplesPerBlock);
        delay.prepare (sampleRate, 4.0);
        reverb.setSampleRate (sampleRate);
        reverb.reset();
        vizBuffer.reset();

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

        // Mod sources from MIDI / GUI
        out.modWheel01     = currentModWheel01;
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

        // --- Chorus / Flanger / Phaser (insert before time-based FX) ---
        chorus.setMode    (loadChoice<params::ChorusMode>(pChorusMode, (int) params::ChorusMode::NumChorusModes));
        chorus.setRate    (pChorusRate->load());
        chorus.setDepth   (pChorusDepth->load());
        chorus.setFeedback(pChorusFeedback->load());
        chorus.setMix     (pChorusMix->load());
        if (numChans > 1)
            chorus.process (L + start, R + start, numSamples);

        // --- Delay ---
        delay.setTimes      (pDelayTimeL->load(), pDelayTimeR->load());
        delay.setFeedback   (pDelayFeedback->load());
        delay.setCrossFeedback (pDelayPingPong->load());
        delay.setDampingCutoff (pDelayDamp->load());
        delay.setMix        (pDelayMix->load());
        delay.process (L + start, R + start, numSamples);

        // --- Reverb ---
        juce::Reverb::Parameters rp;
        rp.roomSize  = pReverbSize->load();
        rp.damping   = pReverbDamping->load();
        rp.width     = pReverbWidth->load();
        rp.wetLevel  = pReverbMix->load();
        rp.dryLevel  = 1.0f - rp.wetLevel * 0.5f;  // keep dry mostly intact
        rp.freezeMode= 0.0f;
        reverb.setParameters (rp);

        if (numChans > 1)
            reverb.processStereo (L + start, R + start, numSamples);
        else
            reverb.processMono   (L + start, numSamples);

        // ---- Visualisation feed: push the post-FX signal to the lock-free
        // ring so the editor's oscilloscope + spectrum analyser can render it.
        if (numChans > 1)
            vizBuffer.push (L + start, R + start, numSamples);
        else
            vizBuffer.push (L + start, L + start, numSamples);
    }

    void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples = buffer.getNumSamples();
        const int numChans   = buffer.getNumChannels();

        for (int ch = 0; ch < numChans; ++ch)
            buffer.clear (ch, 0, numSamples);

        // Inject MIDI from on-screen / PC keyboard.
        keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

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
