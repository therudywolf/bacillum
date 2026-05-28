# Bacillum — Roadmap

Re-actualized after the v0.2 milestone (visualisers, HyperSaw, sub-osc options,
Chorus/Flanger/Phaser, cyberpunk LookAndFeel).

The goal remains: **hardware-class virtual-analog polyphonic synth** — Virus TI /
Nord Lead A1 / JP-8000 quality, not "another bedroom JUCE example".

Each phase ends with: `pluginval --strict-level 7` passing, plugin loading in
Logic / FL / Ableton / Reaper / Cubase, no realtime allocations under Instruments.

---

## Done (v0.1 + v0.2)

- [x] Plugin skeleton (VST3 + Standalone) via JUCE 8 + modern CMake.
- [x] APVTS with ~60 parameters; XML state save/load.
- [x] Custom `VoiceManager` (16 voices, full stealing priority).
- [x] Sample-accurate MIDI dispatch (sub-block splitting).
- [x] PolyBLEP oscillator (Sine / Tri / Saw / Square+PWM).
- [x] HyperSaw oscillator (7-saw Roland-JP8000-style, Szabo paper).
- [x] Sub osc with selectable waveform (Sine / Tri / Square) and octave (-1 / -2).
- [x] White + Pink noise (Paul Kellet).
- [x] TPT SVF filter — LP12 / LP24 / HP / BP / Notch / Peak, tanh drive, key-track,
      velocity-to-cutoff, ±5 oct env modulation.
- [x] Two ADSRs (filter + amp) with soft-retrigger.
- [x] Unison 1–8 with cents detune + stereo spread.
- [x] LFO 1 (per-voice key-trig, 8 shapes, fade-in, mod-wheel-scaled depth).
- [x] Mod wheel / pitch bend / sustain / channel AT / all-notes-off / panic.
- [x] FX bus: Chorus/Flanger/Phaser → Stereo delay → Reverb → Master gain.
- [x] Real-time oscilloscope (zero-cross triggered).
- [x] Real-time spectrum analyser (FFT 2048, log/log, peak hold).
- [x] PC-keyboard piano (`A W S E D F T G Y H U J K`, `Z X` octave).
- [x] Wolf-cyberpunk `LookAndFeel` (Consolas mono, cyan + blood, terminal header).
- [x] UTF-8 source toolchain (`/utf-8` flag).

---

## v0.3 — Modulation & sources (next, ~1–2 weeks)

The goal: a fully-modulated, deeply patchable synth.

- [ ] **LFO 2** — second per-voice key-triggered LFO with independent routings
      (target list: cutoff, pitch, PW, pan, FX sends).
- [ ] **LFO 3** — global, free-running, tempo-syncable LFO; routes to master pan,
      cutoff (all voices), delay/reverb FX.
- [ ] **Tempo sync** for LFOs and delay via `AudioPlayHead::PositionInfo::getBpm()`.
      Note divisions: 1/1, 1/2, 1/4, 1/4T, 1/8, 1/8T, 1/16, 1/32.
- [ ] **8-slot Mod Matrix** (spec §2.2). Sources: ENV1-3, LFO1-3, Vel, Note,
      Aftertouch (channel + poly), ModWheel, PB, Breath, Expression, Sustain,
      RandomPerNote, RandomPerSample, Constant1. Destinations: every continuous
      param. Curves: linear / exp / log / quad / S.
- [ ] **3rd envelope** (free / DAHDSR) for the mod matrix.
- [ ] **Glide / portamento** — constant-time, legato-only mode.
- [ ] **Velocity curve** options (linear / fixed / exponential / S-curve).

---

## v0.4 — Sound sources & filter quality (~1–2 weeks)

- [ ] **Wavetable oscillator** — mip-mapped, Lagrange-3 interp, ≥16 factory tables
      programmatically generated (harmonic-series saw/square/tri/sine, organ,
      vocal formant, FM, additive bell, etc.). Scan via mod.
- [ ] **OSC interop**:
      - Hard sync OSC2 → OSC1 with BLEP correction (Brandt MinBLEP).
      - Ring mod OSC1 × OSC2.
      - PM / linear-FM OSC2 → OSC1.
      - Cross-mod (OSC1 audio modulates OSC2 freq).
- [ ] **Moog ladder filter** (Huovilainen 2004) — 4-stage tanh ladder with feedback
      compensation. Wrapped in `juce::dsp::Oversampling 2x` (FIR halfband).
- [ ] **Filter type switch** per voice: SVF / Ladder / TB-303-style.
- [ ] **2nd filter** with serial / parallel / split routing (spec §1.1).
- [ ] **Saturator stage** between filters (tanh / hard / fold / bitcrush / decimator).

---

## v0.5 — Performance & FX polish (~1–2 weeks)

- [ ] **Arpeggiator** — modes: up / down / up-down / random / chord / order;
      gate, swing, octave range, sync to host.
- [ ] **Step sequencer** (4×16 patterns) — pitch, velocity, gate, tied steps.
- [ ] **Dattorro plate reverb** (replace `juce::Reverb`) — pre-delay, 4 input AP,
      tank with modulated AP, damping LP, multi-tap output (Dattorro 1997).
- [ ] **EQ** — 3-band RBJ biquads (low shelf + peak + high shelf).
- [ ] **Drive** — proper oversampled tanh / tube / fold / bitcrush with
      `juce::dsp::Oversampling` 2x / 4x.
- [ ] **Compressor / Limiter** on output bus (RMS detector, soft knee).
- [ ] **MPE** support (`juce::MPESynthesiser` pattern) — per-note pitch, slide
      (CC74), pressure.

---

## v0.6 — Presets & GUI polish (~2–3 weeks)

- [ ] **Preset format**: `ValueTree` + metadata (`name`, `author`, `category`,
      `tags[]`, `bpm_hint`, `description`, `plugin_version`, `state_version`).
- [ ] **128 factory presets** across categories (Lead, Bass, Pad, Pluck, Keys,
      Brass, Strings, Arp, Sequence, FX, Drum, Init).
- [ ] **Preset browser** in editor — filter by category + tag, instant search,
      A/B compare, init/random buttons.
- [ ] **Import / export** single preset + bank (`.bcl` JSON or binary).
- [ ] **MIDI Learn** — right-click any param → next CC binds. Persist mapping
      in state. Indicator badge on mapped controls.
- [ ] **State upgrade chain** — `state_version` int + per-version migrator.
- [ ] **GUI polish**:
      - Knob highlight ring showing live modulated value (semi-transparent).
      - Tooltip + value display + double-click reset.
      - Vector SVG knobs (replace LookAndFeel `Path` drawing if needed).
      - Wolf-head SVG mark in the header.
      - Optional: Foleys GUI Magic XML for declarative skinning.

---

## v0.7 — Distribution & QA (~1 week)

- [ ] `pluginval --strict-level 10` clean.
- [ ] No realtime allocations verified under Instruments / Tracy.
- [ ] 60-minute stress test (32 unison × 8 voices) → CPU + RAM stable.
- [ ] DAW matrix: Logic, FL, Ableton, Cubase, Reaper, Studio One, Bitwig.
- [ ] **macOS** Universal Binary (arm64 + x86_64), codesign + notarization.
- [ ] **Windows** EV / OV signtool.
- [ ] GitHub Actions CI: build + pluginval on `macos-13` + `windows-2022`.
- [ ] Installer: Packages (Mac), Inno Setup (Win).

---

## Stretch / research (no committed schedule)

- [ ] **Multi-timbral** 4-slot architecture (Nord Lead-style splits/layers).
- [ ] **Voice-stack architecture** for layered patches.
- [ ] **Vocoder** module (16-band).
- [ ] **Granular** mode for OSC3 (Plaits-inspired).
- [ ] **Convolution reverb** (low-priority IR support).
- [ ] **AAX** (Pro Tools) build (requires Avid NDA + dev account).
- [ ] **LV2** for Linux audiophiles.
- [ ] **Audio-rate modulation** (FM matrix at audio rate, not control rate).

---

## What we are *not* going to build

- **Lua / scripting** in the patch format — attack surface, not worth it.
- **Cloud presets / phone-home telemetry** — privacy first, never.
- **"AI" sound design** — out of scope; the synth is a tool, not a prompt.
