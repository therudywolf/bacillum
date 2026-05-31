# Changelog

All notable changes to Bacillum. Format loosely follows
[Keep a Changelog](https://keepachangelog.com/); versions are git tags.

## [Unreleased]
- Modulation-value rings on the filter Cutoff / Resonance knobs (live, audio→UI).
- Double-click-to-default and hover tooltips on every control.
- Repo housekeeping: LICENSE, .editorconfig, CHANGELOG, spec moved to `docs/`.

## [v0.6.0] — Dattorro reverb + OSC interop + tests
- **Dattorro plate reverb** replaces the Freeverb-derived `juce::Reverb`.
- **OSC interop**: hard sync (OSC2→OSC1), ring mod, FM / phase-mod.
- **Mod-matrix curves** per slot (Linear / Exp / Quad / S).
- **Unit tests** (Catch2, 13 cases) + CI test job; pluginval in CI.
- Factory bank grown to 34 patches showcasing the new engine.

## [v0.5.0] — Dual filter, wavetable, EQ/comp, CI
- **2nd filter** + Single / Serial / Parallel / Split routing + **saturator**
  (tanh / soft / hard / fold / bitcrush / rate-reduce) between filters.
- **Wavetable oscillator** (mip-mapped, 8-frame morph, alias-free).
- **3-band EQ** at the front of the FX bus; **compressor + brick-wall limiter**
  at the end.
- **GitHub Actions CI**: Windows build + pluginval, Linux portability build.

## [v0.4.0] — Modulation system
- **8-slot modulation matrix** (14 sources, 17 destinations), pull-based,
  control-rate.
- **LFO2** (per-voice) + **LFO3** (global) + **ENV3** (free ADSR).
- UI: modulators row + full-width mod-matrix grid.

## [v0.3.0] — Glide, ladder, tempo sync, arp, presets
- Glide / portamento; **Moog ladder** filter (Huovilainen, 2× oversampled);
  tempo sync (LFO / delay / arp) via host BPM.
- **Arpeggiator** (Up / Down / Up-Down / Random / As-Played).
- Preset system + browser + 24-patch factory bank.

## [v0.2.0] — First public commit
- Custom 16-voice manager, PolyBLEP + HyperSaw oscillators, sub + noise,
  TPT SVF filter, 2 ADSRs, unison, LFO1, chorus/delay/reverb FX,
  oscilloscope + spectrum analyser, PC-keyboard input,
  wolf-cyberpunk LookAndFeel. JUCE 8 + CMake, VST3 + Standalone.
