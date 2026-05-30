#pragma once

#include "dsp/voice/VoiceManager.h"
#include "dsp/effects/Delay.h"
#include "dsp/effects/Chorus.h"
#include "dsp/effects/Eq3.h"
#include "dsp/effects/DattorroReverb.h"
#include "dsp/arp/Arpeggiator.h"
#include "dsp/util/AudioVizBuffer.h"
#include "params/Params.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

namespace bacillum
{
    class PluginProcessor final : public juce::AudioProcessor
    {
    public:
        PluginProcessor();
        ~PluginProcessor() override = default;

        void prepareToPlay (double sampleRate, int samplesPerBlock) override;
        void releaseResources() override {}

        bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

        void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
        using juce::AudioProcessor::processBlock;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        const juce::String getName() const override { return "Bacillum"; }
        bool acceptsMidi()  const override { return true; }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }
        double getTailLengthSeconds() const override { return 3.0; }  // for reverb tail

        int getNumPrograms() override        { return 1; }
        int getCurrentProgram() override     { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}

        void getStateInformation (juce::MemoryBlock& dest) override;
        void setStateInformation (const void* data, int sizeInBytes) override;

        juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
        juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }
        const dsp::AudioVizBuffer& getVizBuffer() const noexcept { return vizBuffer; }
        double getCurrentSampleRate() const noexcept { return currentSampleRate; }

    private:
        void handleMidiEvent (const juce::MidiMessage& m) noexcept;
        void snapshotVoiceParams (dsp::VoiceParams& out) const noexcept;
        void applyUnisonSetting() noexcept;
        void renderFromMidi (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples);
        void renderSubBlock (juce::AudioBuffer<float>& buffer, int start, int numSamples);
        void applyFxBus (juce::AudioBuffer<float>& buffer, int start, int numSamples);

        // Helper: load APVTS choice index, clamped.
        template <typename EnumT>
        EnumT loadChoice (std::atomic<float>* p, int maxExclusive) const noexcept
        {
            return static_cast<EnumT>(juce::jlimit (0, maxExclusive - 1, static_cast<int>(p->load())));
        }

        juce::AudioProcessorValueTreeState apvts;
        juce::MidiKeyboardState keyboardState;

        // ----- Cached APVTS pointers ----------------------------------------
        std::atomic<float>* pMasterGain    { nullptr };
        std::atomic<float>* pMasterPan     { nullptr };

        std::atomic<float>* pOsc1Waveform   { nullptr };
        std::atomic<float>* pOsc1Pitch      { nullptr };
        std::atomic<float>* pOsc1Detune     { nullptr };
        std::atomic<float>* pOsc1Pulsewidth { nullptr };
        std::atomic<float>* pOsc1Level      { nullptr };

        std::atomic<float>* pOsc2Waveform   { nullptr };
        std::atomic<float>* pOsc2Pitch      { nullptr };
        std::atomic<float>* pOsc2Detune     { nullptr };
        std::atomic<float>* pOsc2Pulsewidth { nullptr };
        std::atomic<float>* pOsc2Level      { nullptr };

        std::atomic<float>* pSubLevel       { nullptr };
        std::atomic<float>* pSubWaveform    { nullptr };
        std::atomic<float>* pSubOctave      { nullptr };
        std::atomic<float>* pHyperDetune    { nullptr };
        std::atomic<float>* pHyperMix       { nullptr };
        std::atomic<float>* pWavetablePos   { nullptr };
        std::atomic<float>* pOscSync        { nullptr };
        std::atomic<float>* pOscRing        { nullptr };
        std::atomic<float>* pOscFM          { nullptr };
        std::atomic<float>* pNoiseType      { nullptr };
        std::atomic<float>* pNoiseLevel     { nullptr };

        std::atomic<float>* pChorusMode     { nullptr };
        std::atomic<float>* pChorusMix      { nullptr };
        std::atomic<float>* pChorusRate     { nullptr };
        std::atomic<float>* pChorusDepth    { nullptr };
        std::atomic<float>* pChorusFeedback { nullptr };

        std::atomic<float>* pGlideTime      { nullptr };
        std::atomic<float>* pLfo1Sync       { nullptr };
        std::atomic<float>* pDelaySync      { nullptr };

        std::atomic<float>* pArpOn          { nullptr };
        std::atomic<float>* pArpMode        { nullptr };
        std::atomic<float>* pArpRate        { nullptr };
        std::atomic<float>* pArpOctaves     { nullptr };
        std::atomic<float>* pArpGate        { nullptr };

        std::atomic<float>* pLfo2Shape      { nullptr };
        std::atomic<float>* pLfo2Rate       { nullptr };
        std::atomic<float>* pLfo2Sync       { nullptr };
        std::atomic<float>* pLfo2FadeIn     { nullptr };

        std::atomic<float>* pLfo3Shape      { nullptr };
        std::atomic<float>* pLfo3Rate       { nullptr };
        std::atomic<float>* pLfo3Sync       { nullptr };

        std::atomic<float>* pEnv3Attack     { nullptr };
        std::atomic<float>* pEnv3Decay      { nullptr };
        std::atomic<float>* pEnv3Sustain    { nullptr };
        std::atomic<float>* pEnv3Release    { nullptr };

        std::atomic<float>* pModSrc[params::kNumModSlots]   { nullptr };
        std::atomic<float>* pModDst[params::kNumModSlots]   { nullptr };
        std::atomic<float>* pModDepth[params::kNumModSlots] { nullptr };
        std::atomic<float>* pModCurve[params::kNumModSlots] { nullptr };

        std::atomic<float>* pFilterMode     { nullptr };
        std::atomic<float>* pFilterCutoff   { nullptr };
        std::atomic<float>* pFilterRes      { nullptr };
        std::atomic<float>* pFilterDrive    { nullptr };
        std::atomic<float>* pFilterKeytrack { nullptr };
        std::atomic<float>* pFilterEnvAmt   { nullptr };
        std::atomic<float>* pFilterVelAmt   { nullptr };

        std::atomic<float>* pFilterRouting  { nullptr };
        std::atomic<float>* pFilter2Mode    { nullptr };
        std::atomic<float>* pFilter2Cutoff  { nullptr };
        std::atomic<float>* pFilter2Res     { nullptr };
        std::atomic<float>* pSatType        { nullptr };
        std::atomic<float>* pSatAmount      { nullptr };

        std::atomic<float>* pFilterAttack   { nullptr };
        std::atomic<float>* pFilterDecay    { nullptr };
        std::atomic<float>* pFilterSustain  { nullptr };
        std::atomic<float>* pFilterRelease  { nullptr };

        std::atomic<float>* pAmpAttack      { nullptr };
        std::atomic<float>* pAmpDecay       { nullptr };
        std::atomic<float>* pAmpSustain     { nullptr };
        std::atomic<float>* pAmpRelease     { nullptr };

        std::atomic<float>* pLfo1Shape      { nullptr };
        std::atomic<float>* pLfo1Rate       { nullptr };
        std::atomic<float>* pLfo1FadeIn     { nullptr };
        std::atomic<float>* pLfo1ToCutoff   { nullptr };
        std::atomic<float>* pLfo1ToPitch    { nullptr };
        std::atomic<float>* pLfo1ToAmp      { nullptr };

        std::atomic<float>* pUnisonCount    { nullptr };
        std::atomic<float>* pUnisonDetune   { nullptr };
        std::atomic<float>* pUnisonSpread   { nullptr };

        std::atomic<float>* pDelayMix       { nullptr };
        std::atomic<float>* pDelayTimeL     { nullptr };
        std::atomic<float>* pDelayTimeR     { nullptr };
        std::atomic<float>* pDelayFeedback  { nullptr };
        std::atomic<float>* pDelayPingPong  { nullptr };
        std::atomic<float>* pDelayDamp      { nullptr };

        std::atomic<float>* pReverbMix      { nullptr };
        std::atomic<float>* pReverbSize     { nullptr };
        std::atomic<float>* pReverbDamping  { nullptr };
        std::atomic<float>* pReverbWidth    { nullptr };

        std::atomic<float>* pEqLowFreq      { nullptr };
        std::atomic<float>* pEqLowGain      { nullptr };
        std::atomic<float>* pEqMidFreq      { nullptr };
        std::atomic<float>* pEqMidGain      { nullptr };
        std::atomic<float>* pEqMidQ         { nullptr };
        std::atomic<float>* pEqHighFreq     { nullptr };
        std::atomic<float>* pEqHighGain     { nullptr };

        std::atomic<float>* pCompThresh     { nullptr };
        std::atomic<float>* pCompRatio      { nullptr };
        std::atomic<float>* pCompAttack     { nullptr };
        std::atomic<float>* pCompRelease    { nullptr };
        std::atomic<float>* pCompMakeup     { nullptr };

        std::atomic<float>* pPolyMode       { nullptr };
        std::atomic<float>* pPitchBendRange { nullptr };

        // ----- Bus state -----------------------------------------------------
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterGain;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterPanL;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterPanR;

        dsp::VoiceManager voiceManager;
        dsp::Arpeggiator  arp;
        dsp::Lfo          lfo3Global;   // global LFO, advanced per block
        dsp::Eq3          eq;           // front of FX chain
        dsp::ChorusFx     chorus;
        dsp::StereoDelay  delay;
        dsp::DattorroReverb reverb;          // Dattorro plate
        juce::dsp::Compressor<float> comp;   // end of FX chain
        juce::dsp::Limiter<float>    limiter; // brick-wall safety
        dsp::AudioVizBuffer vizBuffer;

        // Scratch MIDI buffer for arp/keyboard merging (reused; clear() keeps capacity).
        juce::MidiBuffer  workMidi;

        // ----- Per-block runtime state ---------------------------------------
        float currentSampleRate    { 48000.0f };
        float currentPitchBendNorm { 0.0f };
        float currentModWheel01    { 0.0f };
        float currentAftertouch01  { 0.0f };
        float currentBpm           { 120.0f };  // from host playhead, fallback 120
        float currentLfo3Value     { 0.0f };    // global LFO, one value per block

        // Last applied unison config — used to avoid re-applying every block.
        int   lastUnisonCount { 1 };
        float lastUnisonDetune{ 7.0f };
        float lastUnisonSpread{ 0.5f };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
    };
}
