#include "dsp/voice/VoiceManager.h"

namespace bacillum::dsp
{
    void VoiceManager::prepare(double sr) noexcept
    {
        for (auto& v : voices)
            v.prepare(sr);
        reset();
    }

    void VoiceManager::reset() noexcept
    {
        for (auto& v : voices)
            v.reset();
        sustainDown = false;
        sustainedNote.fill(false);
        ageCounter = 0;
    }

    void VoiceManager::setParams(const VoiceParams& p) noexcept
    {
        params = p;
        for (auto& v : voices)
            v.applyParams(params);
    }

    void VoiceManager::setUnison(int count, float detuneCents, float spread01) noexcept
    {
        unisonCount  = juce::jlimit(1, 8, count);
        unisonDetune = juce::jlimit(0.0f, 50.0f, detuneCents);
        unisonSpread = juce::jlimit(0.0f, 1.0f, spread01);
    }

    Voice* VoiceManager::findVoiceForNoteOn(int midiNote, bool allowSameNoteRetrigger) noexcept
    {
        // 1. same-note retrigger (only when not in unison mode).
        if (allowSameNoteRetrigger)
        {
            for (auto& v : voices)
                if (v.isPlaying() && v.getMidiNote() == midiNote && ! v.isInRelease())
                    return &v;
            for (auto& v : voices)
                if (v.isPlaying() && v.getMidiNote() == midiNote)
                    return &v;
        }

        // 2. any idle voice.
        for (auto& v : voices)
            if (! v.isPlaying())
                return &v;

        // 3. released voice with lowest envelope level.
        Voice* candidate = nullptr;
        float  lowest    = 2.0f;
        for (auto& v : voices)
        {
            if (v.isInRelease() && v.envLevel() < lowest)
            {
                lowest = v.envLevel();
                candidate = &v;
            }
        }
        if (candidate)
            return candidate;

        // 4. oldest voice, fast-killed so the new note doesn't click.
        Voice* oldest = &voices[0];
        for (auto& v : voices)
            if (v.getStartStamp() < oldest->getStartStamp())
                oldest = &v;
        oldest->killFast();
        return oldest;
    }

    void VoiceManager::triggerOneVoice(int midiNote, float vel,
                                       float centsOffset, float panOffset) noexcept
    {
        // In unison mode we disable same-note retrigger so the unison group
        // doesn't collapse into a single voice on repeated key strikes.
        const bool unison = (unisonCount > 1);
        Voice* slot = findVoiceForNoteOn(midiNote, !unison);

        slot->setUnisonOffsets(centsOffset, panOffset);
        slot->applyParams(params);
        slot->noteOn(midiNote, vel);
        slot->setStartStamp(++ageCounter);
    }

    void VoiceManager::noteOn(int midiNote, float vel) noexcept
    {
        if (midiNote < 0 || midiNote > 127) return;
        const float v = juce::jlimit(0.0f, 1.0f, vel);
        if (v <= 0.0f)
        {
            noteOff(midiNote, 0.0f);
            return;
        }

        // Mono / Legato use a single voice slot; ignore unison.
        if (polyMode == params::PolyMode::Mono || polyMode == params::PolyMode::Legato)
        {
            Voice& mono = voices[0];
            const bool wasPlaying = mono.isPlaying() && ! mono.isInRelease();
            if (polyMode == params::PolyMode::Legato && wasPlaying)
            {
                mono.setNote(midiNote);
            }
            else
            {
                mono.setUnisonOffsets(0.0f, 0.0f);
                mono.applyParams(params);
                mono.noteOn(midiNote, v);
                mono.setStartStamp(++ageCounter);
            }
            for (size_t i = 1; i < voices.size(); ++i)
                voices[i].killFast();
            return;
        }

        // Poly + Unison: spawn `unisonCount` voices with symmetric detune & pan.
        // unisonCount=1 → single voice, no detune; same as before unison existed.
        const int N = unisonCount;
        if (N == 1)
        {
            triggerOneVoice(midiNote, v, 0.0f, 0.0f);
            return;
        }

        for (int i = 0; i < N; ++i)
        {
            // Normalised position [-1..+1] across the unison group.
            const float t = (N == 1) ? 0.0f
                                     : (2.0f * static_cast<float>(i) / static_cast<float>(N - 1) - 1.0f);
            const float cents = t * unisonDetune;
            const float pan   = t * unisonSpread;
            triggerOneVoice(midiNote, v, cents, pan);
        }
    }

    void VoiceManager::noteOff(int midiNote, float velRel) noexcept
    {
        if (midiNote < 0 || midiNote > 127) return;

        if (sustainDown)
        {
            sustainedNote[(size_t) midiNote] = true;
            return;
        }

        for (auto& v : voices)
            if (v.isPlaying() && v.getMidiNote() == midiNote && ! v.isInRelease())
                v.noteOff(velRel);
    }

    void VoiceManager::sustainPedal(bool down) noexcept
    {
        if (down == sustainDown) return;
        sustainDown = down;
        if (! down)
        {
            for (auto& v : voices)
            {
                if (v.isPlaying() && ! v.isInRelease())
                {
                    const int n = v.getMidiNote();
                    if (n >= 0 && sustainedNote[(size_t) n])
                        v.noteOff(0.0f);
                }
            }
            sustainedNote.fill(false);
        }
    }

    void VoiceManager::allNotesOff(bool fast) noexcept
    {
        for (auto& v : voices)
        {
            if (! v.isPlaying()) continue;
            if (fast) v.killFast();
            else      v.noteOff(0.0f);
        }
        sustainDown = false;
        sustainedNote.fill(false);
    }

    void VoiceManager::panic() noexcept
    {
        for (auto& v : voices)
            v.reset();
        sustainDown = false;
        sustainedNote.fill(false);
    }

    void VoiceManager::render(float* outL, float* outR, int startSample, int numSamples) noexcept
    {
        for (auto& v : voices)
            v.renderAdd(outL, outR, startSample, numSamples);
    }
}
