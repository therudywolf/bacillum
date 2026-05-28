# Bacillum

Virtual-analog polyphonic synthesizer plugin in the spirit of **Access Virus TI**,
**Nord Lead A1/4** and **Roland JP-8000**.

VST3 + Standalone for Windows/macOS, written in C++20 on top of JUCE 8.

```
root@bacillum:~$ ./synth -mode poly -voices 16 -engine VA
```

---

## What works today

### Synthesis engine

| Block | Status | Notes |
|---|---|---|
| Voice manager | done | custom 16-voice pool, NOT `juce::Synthesiser`. Stealing priority: same-note > idle > released-lowest-env > oldest, with anti-click fast-kill of the stolen slot. |
| OSC1 / OSC2 | done | PolyBLEP-band-limited Sine / Triangle / Saw / Square+PWM / **HyperSaw** (7-voice Roland-JP8000-style supersaw). Pitch ±24 semi, detune ±50 cents, level. |
| Sub osc | done | Independent sine / triangle / square, -1 or -2 octave switch, own level. |
| Noise | done | White (xorshift32, per-note seed) + pink (Paul Kellet). |
| Filter | done | TPT State-Variable (Cytomic / Andy Simper): LP12, **LP24** cascade, HP, BP, Notch, Peak. Drive (tanh) pre-filter. |
| Filter envelope | done | Linear ADSR, ±5 octaves cutoff mod, key-tracking, velocity-to-cutoff. |
| Amp envelope | done | Linear ADSR with soft-retrigger (no clicks on overlapping notes), velocity-squared response. |
| LFO 1 | done | Per-voice key-triggered; 8 shapes (sine/tri/saw↑/saw↓/square/PWM/S&H/smooth-random); fade-in; routings to cutoff (±5 oct), pitch (±12 semi), amp (tremolo); mod-wheel scales all depths (fixed routing). |
| Unison | done | 1–8 voices per note, symmetric cents detune, equal-power stereo spread per pair. |
| Glide / mono / legato | done | Mono + legato note priority (last). |
| Pitch bend, sustain, AT, panic | done | Sample-accurate MIDI dispatch in `processBlock`. |

### FX bus

Voice mix → Chorus/Flanger/Phaser → Stereo Delay → Reverb → Master gain → Out.

| FX | Status | Notes |
|---|---|---|
| Chorus / Flanger / Phaser | done | One unit with mode select. Quadrature LFO (90° L/R offset), Lagrange-3 delay line, 6-stage allpass cascade for phaser. |
| Delay | done | Stereo with independent L/R times, ping-pong cross-feedback, damping LP in feedback path. |
| Reverb | done | `juce::Reverb` (Freeverb-derived). Plate / Dattorro implementation planned. |

### Visualisation (real-time)

Lock-free SPSC ring buffer fed by audio thread post-FX; UI Timer at 30 Hz reads
without blocking the audio path.

| Component | Notes |
|---|---|
| Oscilloscope | Rising zero-cross trigger, 1024-sample window, cyan trace with glow. |
| Spectrum analyser | 2048-pt Hann-windowed FFT, log frequency axis, log magnitude (dB), per-bin attack/release smoothing, peak-hold dots, cyan→red gradient fill. |

### Input

- **MIDI 1.0** from any DAW / external hardware.
- **PC keyboard piano** via `juce::MidiKeyboardComponent` — works in Standalone
  even when a knob has focus (keys forwarded by editor override).
  Layout: `A W S E D F T G Y H U J K` (one octave white+black), `Z` / `X` octave down/up.
- On-screen clickable keyboard.

### UI

- Custom **wolf-cyberpunk** `LookAndFeel`: black / cyan / blood-red palette,
  Consolas mono everywhere, sharp angular sections with cyan accent stripes,
  terminal-style header (`root@bacillum:~$ ...` with blinking caret),
  concentric cyber knobs with bipolar fill-from-centre.
- Resizable (960×720 → 2200×1400).
- All ~60 parameters in APVTS, host-automatable, state save/load via XML.

---

## Build

### Prerequisites

- Windows: **Visual Studio 2022 Build Tools** + Windows 11 SDK + **CMake ≥ 3.22**.
- macOS: Xcode 14+ (untested in this revision but the CMake project is portable).
- JUCE 8.0.4 is fetched automatically via `FetchContent` — no submodule, no
  Projucer.

### Build commands

```powershell
# From a Developer PowerShell for VS 2022 (so cl.exe is on PATH)
cd C:\path\to\bacillum
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config RelWithDebInfo --parallel 8
```

Artefacts land in:

```
build\Bacillum_artefacts\RelWithDebInfo\
  Standalone\Bacillum.exe                                 ~14 MB
  VST3\Bacillum.vst3\Contents\x86_64-win\Bacillum.vst3    ~15 MB
```

Drop the `.vst3` into `C:\Program Files\Common Files\VST3\` to expose it to
FL Studio, Ableton Live, Cubase, Reaper, Studio One, Bitwig…

---

## Architecture

```
            ┌────────────────────────────────────────────────┐
   MIDI ──► │  PluginProcessor::processBlock                 │
            │  ├─ sample-accurate sub-block split            │
            │  ├─ MidiKeyboardState.processNextMidiBuffer    │ ← PC + on-screen kbd
            │  └─ for each sub-block:                        │
            │      VoiceManager::setParams(snapshot)         │
            │      VoiceManager::render(L,R,start,n)         │
            │      ▼                                         │
            │  16 × Voice:                                   │
            │    OSC1 ─┐                                     │
            │    OSC2 ─┼─► mix ──► drive ──► SVF ──► VCA     │
            │    Sub  ─┤                ▲          (AmpEnv)  │
            │    Noise┘                 │                    │
            │                       FilterEnv + LFO1 + Vel   │
            │      ▼                                         │
            │   per-voice DC-block + equal-power pan         │
            │      ▼                                         │
            │  Σ Voices  ──► Chorus/Flanger/Phaser ──►       │
            │              Delay ──► Reverb ──► MasterGain   │
            │      ▼                                         │
            │  Push post-FX to AudioVizBuffer (lock-free)    │
            └────────────────────────────────────────────────┘
                            │
                            ▼
                 Editor (Message thread)
                  ├─ APVTS attachments
                  ├─ Oscilloscope (30 fps timer)
                  └─ Spectrum analyser (30 fps timer + 2048-pt FFT)
```

Hard rules followed in the audio thread:

1. No allocations (no `new`, no STL containers that grow, no `std::string`).
2. No locks. GUI↔audio is `std::atomic` (APVTS raw pointers) + SPSC ring buffers.
3. No exceptions (everything reachable from `processBlock` is `noexcept`).
4. No virtual dispatch in hot loops.
5. DC blocker on every voice output.
6. Sample-accurate MIDI via sub-block splitting on `samplePosition`.
7. Smoothed parameters where it matters (`juce::SmoothedValue` for master gain).
8. Control-rate update for cutoff (every 16 samples) to avoid burning `tan()`.

---

## Scientific references

The DSP is built on canonical work from the audio-research community.
Code-level citations are inline in the relevant headers.

### Oscillators / anti-aliasing
- **Välimäki & Huovilainen**, *"Antialiasing Oscillators in Subtractive Synthesis"*,
  IEEE Signal Processing Magazine, March 2007. — PolyBLEP foundation.
  <https://ieeexplore.ieee.org/document/4117934>
- **Stilson & Smith**, *"Alias-Free Digital Synthesis of Classic Analog Waveforms"*,
  ICMC 1996. — BLIT method.
  <https://ccrma.stanford.edu/~stilti/papers/blit.pdf>
- **Eli Brandt**, *"Hard Sync Without Aliasing"*, ICMC 2001. — MinBLEP for hard sync.
  <https://www.cs.cmu.edu/~eli/papers/icmc01-hardsync.pdf>
- **Adam Szabo**, *"How to Emulate the Super Saw"*, 2009/2010. — Reverse-engineering
  of the Roland JP-8000 SuperSaw, used by Bacillum's HyperSaw mode.

### Filters
- **Andy Simper / Cytomic**, *"Linear Trapezoidal Integrated State Variable Filter"*,
  2014. — TPT SVF used as Bacillum's main filter.
  <https://cytomic.com/files/dsp/SvfLinearTrapezoidal.pdf>
- **Vadim Zavalishin**, *"The Art of VA Filter Design"* (rev. 2.1.2), 2018. —
  Foundational text on zero-delay-feedback / TPT filter design.
  <https://www.discodsp.net/VAFilterDesign_2.1.2.pdf>
- **Antti Huovilainen**, *"Non-linear digital implementation of the Moog ladder filter"*,
  DAFx 2004. — Used by Bacillum's planned ladder filter.
  <https://www.dafx.de/paper-archive/2004/P_061.PDF>
- **Robert Bristow-Johnson**, *"Audio EQ Cookbook"*. — RBJ biquad coefficients used in
  the planned 3-band EQ.
  <https://www.w3.org/TR/audio-eq-cookbook/>

### Effects
- **Jon Dattorro**, *"Effect Design Part 1: Reverberator and Other Filters"*,
  Journal of the AES, 1997. — Plate reverb structure planned for v2 reverb.
  <https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf>

### Interpolation
- **Olli Niemitalo**, *"Polynomial Interpolators for High-Quality Resampling of
  Oversampled Audio"*, 2001. — Lagrange-3 / Hermite delay-line interpolation.
  <http://yehar.com/blog/wp-content/uploads/2009/08/deip.pdf>

### Real-time audio engineering
- **Ross Bencina**, *"Real-time audio programming 101: time waits for nothing"*. —
  Why no allocations / locks / IO on the audio thread.
  <http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing>
- **Dave Rowland & Fabian Renn-Giles**, *"Real-time 101"*, CppCon 2019. —
  Lock-free patterns for GUI↔audio communication.

### Reference codebases (read for sanity)
- **Surge XT** (GPLv3) — gold-standard open VA + wavetable. <https://github.com/surge-synthesizer/surge>
- **Vital** (GPLv3) — wavetable engine, SIMD voice processing. <https://github.com/mtytel/vital>
- **OB-Xd / OB-Xf** (GPLv3) — compact JUCE VA. <https://github.com/surge-synthesizer/OB-Xf>
- **chowdsp_utils** (BSD-3) — JUCE-style DSP primitives.

---

## Roadmap

See [`ROADMAP.md`](./ROADMAP.md).

---

## License

The source code in `source/` is © therudywolf, all rights reserved (for now).
JUCE is dual-licensed (GPLv3 / commercial); building Bacillum statically pulls in
JUCE, so any redistribution must comply with JUCE's GPLv3 unless you hold a
commercial JUCE licence.
