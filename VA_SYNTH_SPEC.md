# Virtual-Analog Synthesizer — Полное техническое задание

**Цель проекта.** Сделать VST3 / AU / Standalone виртуально-аналоговый полифонический синтезатор уровня **Access Virus TI** и **Nord Lead A1/4** с поддержкой FL Studio, Logic Pro, Ableton Live, Cubase, Reaper.

**Стек.** C++20, JUCE 7.x+, CMake. Без Projucer — современный CMake API с `juce_add_plugin`.

**Платформы (MVP).** macOS (AU + VST3 + Standalone) и Windows (VST3 + Standalone). Linux — bonus (тот же CMake-проект соберётся).

**Аудитория этого документа.** AI-агент, который будет писать код (Cursor / Claude Code / Cline), и инженер, который будет проверять результат.

---

## 0. Как использовать этот документ

1. Раздел 1–3 — обзор и архитектура. Прочитать целиком.
2. Раздел 4 — фазы (Phase 0…6). Реализовывать строго последовательно. Не переходить к следующей пока pluginval не зелёный и Logic/FL не открыли плагин.
3. Раздел 5 — DSP-формулы. Скопировать/адаптировать псевдокод.
4. Раздел 6 — список референс-репозиториев. Если что-то непонятно — открыть указанный файл.
5. Раздел 7 — **финальный промт для нейросети.** Это то, что ты копируешь в Cursor/Claude Code в начале каждой фазы.
6. Раздел 8 — критерии приёмки и тестирование.
7. Раздел 9 — безопасность и подпись (важно, если выкладывать публично).

---

## 1. Обзорная архитектура

### 1.1. High-level signal flow

```
MIDI in ─► VoiceManager (16-32 voices, custom; НЕ juce::Synthesiser)
            │
            └─► per Voice:
                  ┌─ OSC1 (multi-mode) ─┐
                  ├─ OSC2 (multi-mode) ─┼─► OSC Mixer ─► Filter1 ─┬─► [serial] ─► Filter2 ─► Saturator ─► VCA(AmpEG)
                  ├─ OSC3 / Sub        ─┤                         └─► [parallel] sum
                  └─ Noise              ─┘
                  
                  ModMatrix (per-voice + global) feeds: pitch, PWM, sync, FM amt,
                  cutoff1/2, res1/2, OSC mix, pan, FX sends, ENV/LFO params

      Voice Sum L/R
            │
            └─► Global FX chain:
                  EQ ─► Drive/Distortion ─► Chorus/Phaser ─► Delay ─► Reverb ─► Compressor/Limiter
                  │
                  └─► Stereo Out
```

### 1.2. Threads и owners

| Поток | Что делает | Запреты |
|---|---|---|
| **Audio (RT)** | `processBlock`, DSP, MIDI handling | malloc/free, locks, IO, exceptions, syscalls |
| **Message** | UI, preset I/O, file load | Не блокировать audio (atomic / lock-free для общения) |
| **Worker (наш)** | Загрузка пресетов с диска, экспорт | Готовит данные в RAM, swap pointer-атомиком в audio |

Общение `Message ↔ Audio` — через:
- `juce::AbstractFifo` для команд (`LoadPreset`, `Panic`, …)
- `std::atomic<float*>` для параметров (APVTS даёт это из коробки через `getRawParameterValue()`)
- SPSC очередь Cameron Desrochers `moodycamel::ReaderWriterQueue`
- Атомарный swap указателей на иммутабельные preset-структуры (RCU-pattern)

### 1.3. Sample-accurate model

`processBlock` разбивается на sub-blocks по MIDI events. Между событиями параметры APVTS читаются один раз → линейная интерполяция (smoothing) до конца sub-block. Modulation matrix обновляется на **control rate = 32 samples** (≈ 666 Hz @ 48 kHz). Pitch, amp envelope и cutoff (если sample-modulated audio rate) — обновляются per-sample внутри voice.

---

## 2. Feature spec (что точно должно быть)

### 2.1. Per-voice

| Блок | Спецификация |
|---|---|
| **OSC × 3** | Режимы: Classic VA (saw/pulse-PWM/triangle/sine + sub) с PolyBLEP; HyperSaw (7-saw c per-saw sub); Wavetable (mip-mapped, мин. 32 фабричных таблицы); Noise (white/pink/filtered) |
| **OSC interop** | Hard sync OSC2→OSC1 (BLEP), Ring Mod OSC1×OSC2, FM OSC2→OSC1 (PM/exponential), cross-mod |
| **Mixer** | Уровни 3 OSC + sub + noise + ring; routing на Filter1/Filter2/обе |
| **Filter × 2** | Каждый: multi-mode (LP12/LP24/HP/BP/Notch/Peak/Allpass), модели TPT-SVF и Moog-ladder (Huovilainen). Cutoff, Res, Drive, Key-track |
| **Routing F1/F2** | Serial / Parallel / Split (раздельные cutoff/res) |
| **Saturator** | Между фильтрами: tanh / soft-poly / hard-clip / foldback / bit-crush / rate-reduce |
| **Envelopes × 3** | DAHDSR + curve (lin/exp), velocity sens, key tracking, loop mode; назначения: VCA, F-cutoff, free (mod-matrix) |
| **LFO × 3** | 8 базовых форм (sine/tri/saw↑/saw↓/sqr/PWM/S&H/smooth-rand) + 8 wavetable LFO; free-run / tempo-sync / key-trig; fade-in, delay, phase offset; **2 per-voice + 1 global** |
| **Glide** | Portamento constant-time, legato-only mode |

### 2.2. Modulation matrix

- **8 user slots**, каждый: `source → destination, amount [-1..+1], side-chain (modulator), curve`
- Sources: ENV1-3, LFO1-3, Velocity, Note number, Aftertouch (channel + poly), Mod wheel (CC1), Pitch bend, Breath (CC2), Expression (CC11), Sustain (CC64), Random per-note, Random per-sample, Constant 1
- Destinations: всё, что параметр (cutoff1/2, res1/2, OSC pitch/PWM/level/FM/sync, ENV times, LFO rate/depth, pan, FX sends, drive, glide…)
- Fixed routings (всегда активны): Vel→VCA, ModWheel→LFO1.amount, PB→OSC pitch, AmpEG→VCA
- Curves: linear, exponential, logarithmic, quadratic, S-shape

### 2.3. Voice management

- **32-voice pool**, allocation на noteOn
- Stealing priority: same-note > released (lowest env) > quietest > oldest
- Unison: 1–8 voices/нота, detune (cents), stereo spread, phase init (free/random/synced)
- Mono / Legato / Poly modes; note priority Last/Low/High в моно

### 2.4. Global FX chain (фиксированный порядок)

1. **3-band parametric EQ** (low shelf + peak + high shelf, biquad RBJ)
2. **Drive / Distortion** (tanh / tube / fold / bit-crush) с oversampling 2x/4x опцией
3. **Chorus / Flanger / Phaser** (mode select)
4. **Delay** (stereo, ping-pong, BPM-sync, feedback filter)
5. **Reverb** (Dattorro plate + room/hall variants)
6. **Compressor / Limiter** (output bus, опционально на)

### 2.5. MIDI

- MIDI 1.0 + опциональный **MPE** (per-note pitch / slide CC74 / pressure)
- Sample-accurate event handling
- Pitch bend (range через RPN 0,0; UI control)
- CC1, CC2, CC7, CC10, CC11, CC64 (sustain), CC66 (sostenuto), CC120/121/123 panic
- Program change → preset switching
- MIDI Learn UI (right-click параметра → next CC binds)
- Channel + poly aftertouch как mod source

### 2.6. Preset system

- Формат: `juce::ValueTree` сериализованный в **бинарный** `MemoryBlock` (для `getStateInformation`) и в `.vstpreset` (VST3 auto via JUCE)
- Метаданные: `name, author, category, tags[], bpm_hint, description, plugin_version, state_version`
- Категории: Lead, Bass, Pad, Pluck, Keys, Brass, Strings, Arp, Sequence, FX, Drum, Init
- 128 factory presets минимум (embedded via `juce_binarydata`)
- Browser UI: фильтр по категории + поиск
- Versioning: `state_version` int + upgrade chain `v1→v2→v3`
- Импорт/экспорт single preset / bank

### 2.7. GUI

- Resizable (50–150%), LookAndFeel custom, vector knobs (SVG)
- Логические панели: OSC | Mixer | Filter | Mod | EG/LFO | FX | Preset Browser | Modulation Routing
- Modulation Matrix визуализация: matrix view + per-knob "right-click → assign mod source"
- Knob: показывать актуальное модулированное значение полупрозрачным ring'ом
- Tooltip + value display + double-click reset
- Стек: JUCE `Component` + кастомный LookAndFeel, ИЛИ Foleys GUI Magic (https://github.com/ffAudio/foleys_gui_magic) если хочется declarative XML

---

## 3. Технологический стек и зависимости

### 3.1. Обязательные

| Что | Версия | Зачем |
|---|---|---|
| **JUCE** | 7.0.10+ | Audio plugin framework, GUI, MIDI, DSP basics |
| **C++ standard** | C++20 | concepts, ranges, designated initializers |
| **CMake** | 3.22+ | Build system (JUCE CMake API) |
| **Xcode** | 14+ | Mac builds, codesigning |
| **MSVC** | 2022 | Windows builds |

### 3.2. Сильно рекомендуемые

| Что | Зачем |
|---|---|
| **Pamplejuce template** ([sudara/pamplejuce](https://github.com/sudara/pamplejuce)) | Готовый CMake + CI + tests + pluginval scaffold |
| **chowdsp_utils** ([Chowdhury-DSP/chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils)) | Качественные DSP примитивы в JUCE-стиле: filters, oscillators, waveshapers, oversampling |
| **moodycamel ReaderWriterQueue** | SPSC очередь для GUI↔Audio |
| **pluginval** ([Tracktion/pluginval](https://github.com/Tracktion/pluginval)) | CI-валидатор плагинов, обязателен |
| **Catch2** или **doctest** | Unit-тесты DSP в офлайне |

### 3.3. Опциональные

| Что | Зачем |
|---|---|
| **Foleys GUI Magic** | Declarative GUI с биндингом к APVTS |
| **melatonin_audio_sparklines** | Дебаг-визуализация буфера |
| **melatonin_perfetto** | Профайлинг audio thread |
| **sst-basic-blocks**, **sst-effects** (Surge Synth Team) | Готовые building blocks |

### 3.4. VST3 / AU специфика

- **VST3 SDK**: подтягивается JUCE автоматически. Лицензия dual: GPLv3 ИЛИ Steinberg proprietary. Если плагин closed-source — зарегистрировать продукт на https://developer.steinberg.help/ и подписать VST3 Licensing Agreement.
- **AU**: только macOS, валидация `auval -v aumu <Code> <Manu>`. Documentation: https://developer.apple.com/library/archive/technotes/tn2247/_index.html
- **AAX** (Pro Tools): требует подписания Avid NDA + dev account. На MVP — пропустить.

---

## 4. Roadmap по фазам (строго последовательно)

> **Правило для каждой фазы:** до перехода к следующей — `pluginval --strict` зелёный, Logic Pro/FL Studio открывают плагин и проигрывают звук, нет утечек на 30-минутном бегущем тесте.

### Phase 0 — Bootstrap (1–2 дня)

- [ ] Клонировать Pamplejuce, переименовать проект
- [ ] CMake собирает VST3 + AU + Standalone на Mac, VST3 + Standalone на Windows
- [ ] Плагин загружается в Logic / FL / Reaper, проходит pluginval basic
- [ ] CI (GitHub Actions): build на 2 платформах, run pluginval, run unit tests
- [ ] APVTS со скелетом параметров (gain, pan)
- [ ] Простой sine-test в `processBlock` для проверки I/O

**Acceptance:** плагин виден в DAW, выдаёт sine 440 Гц по MIDI note 69.

### Phase 1 — MIDI + Voice manager + simple OSC + ADSR (3–5 дней)

- [ ] Свой `VoiceManager` (НЕ `juce::Synthesiser`) — pool 16 голосов, allocation/stealing
- [ ] Sample-accurate MIDI event splitting в `processBlock`
- [ ] Один OSC: PolyBLEP saw/square/sine/triangle, phase accumulator uint32, формулы из §5.1
- [ ] DC blocker (1-pole HP) на выходе OSC
- [ ] Amp envelope: linear ADSR с retrigger soft-ramp (§5.4)
- [ ] Voice = OSC → ADSR → VCA → mix L/R
- [ ] Базовые параметры в APVTS: OSC type, ADSR times

**Acceptance:** играется полифонично, без щелчков, без алиасинга на C4. pluginval `--strict-level 5`.

### Phase 2 — Filters + 2nd OSC + Mixer (5–7 дней)

- [ ] **TPT SVF** (Andy Simper) с LP/HP/BP/Notch/Peak выходами (§5.2)
- [ ] **Moog ladder** (Huovilainen 2004) с tanh nonlinearity и compensation (§5.3)
- [ ] Multi-mode filter UI: select mode + slope
- [ ] OSC2 + Sub + Noise
- [ ] OSC Mixer + drive перед filter
- [ ] Filter envelope (отдельный ADSR), key tracking, velocity→cutoff
- [ ] Hard sync OSC2→OSC1 c BLEP correction (§5.1)

**Acceptance:** characterful звук, фильтр self-oscillates корректно, нет explosion на extreme Q.

### Phase 3 — LFO + Modulation Matrix (5–7 дней)

- [ ] 3 LFO: per-voice (LFO1/2) + global (LFO3)
- [ ] Все 8 базовых форм, tempo sync через `AudioPlayHead::PositionInfo::getBpm()`
- [ ] **Mod Matrix** с 8 slots, sources/destinations table из §2.2
- [ ] Pull-based architecture: каждый control-block destination опрашивает свои sources
- [ ] Curves (linear/exp/log/quad)
- [ ] Smoothing для не-sample-accurate destinations (`juce::SmoothedValue`)

**Acceptance:** wobble bass работает, filter LFO звучит без зипперов, mod wheel модулирует cutoff.

### Phase 4 — Unison, HyperSaw, Wavetable, Glide (5–7 дней)

- [ ] Unison 1–8 voices с detune (cents) и stereo spread
- [ ] HyperSaw: 7 detuned saw + per-saw sub (см. §5.1.5)
- [ ] Wavetable OSC: mip-mapped, минимум 16 фабричных таблиц (можно сгенерировать программно: sine, saw, square, triangle harmonic series, organ, vocal формантные)
- [ ] Lagrange-3 интерполяция между сэмплами таблицы
- [ ] Glide (portamento): exponential approach, constant-time mode

**Acceptance:** trance lead с hypersaw — слышен фирменный жирный звук. Wavetable scan через mod-wheel.

### Phase 5 — Effects chain (7–10 дней)

- [ ] EQ: 3-band biquads (RBJ cookbook, §5.5)
- [ ] Drive: tanh + softclip + bitcrush с `juce::dsp::Oversampling 2x/4x`
- [ ] Chorus/Flanger/Phaser (mode select): delay line + LFO + feedback
- [ ] Delay: stereo, ping-pong, BPM sync, feedback LP/HP filter
- [ ] Reverb: **Dattorro plate** (§5.5) — fallback `juce::Reverb` если время поджимает
- [ ] Compressor/Limiter на выходной шине (опционально включается)

**Acceptance:** built-in patches звучат "готовыми", не нужен внешний reverb.

### Phase 6 — Preset system + GUI polish (7–14 дней)

- [ ] Preset format (`ValueTree` + metadata)
- [ ] Browser UI: список + фильтр + категории
- [ ] 128 factory presets (создать руками в течение sound-design итераций)
- [ ] Import/Export
- [ ] MIDI Learn UI
- [ ] Resizable UI, LookAndFeel custom, vector knobs
- [ ] Modulation Matrix visual editor (matrix view) + per-knob assignment

**Acceptance:** новичок может выбрать пресет за 3 клика, юзер может сделать свой patch без чтения мануала.

### Phase 7 — Sign + Distribute

- [ ] macOS: codesign + notarize (см. §9)
- [ ] Windows: EV/OV certificate, signtool
- [ ] Installer: Packages (Mac), Inno Setup (Win)
- [ ] pluginval `--strict-level 10` зелёный
- [ ] Тест в Logic, FL, Ableton, Cubase, Reaper, Studio One — все запускают, не падают, автоматизация работает

---

## 5. DSP-формулы (готовый псевдокод)

### 5.1. Oscillators

#### 5.1.1. PolyBLEP residual

```cpp
// dt = freq / sampleRate, phase ∈ [0, 1)
inline float polyBlep(float t, float dt) noexcept {
    if (t < dt) {                  // left of discontinuity
        t /= dt;
        return t + t - t*t - 1.0f;
    }
    if (t > 1.0f - dt) {           // right of discontinuity
        t = (t - 1.0f) / dt;
        return t*t + t + t + 1.0f;
    }
    return 0.0f;
}
```

#### 5.1.2. PolyBLEP saw [-1, +1]

```cpp
phase += dt; if (phase >= 1.0f) phase -= 1.0f;
float naive = 2.0f * phase - 1.0f;
float saw   = naive - polyBlep(phase, dt);
```

#### 5.1.3. PolyBLEP square с PWM

```cpp
phase += dt; if (phase >= 1.0f) phase -= 1.0f;
float naive = (phase < pw) ? 1.0f : -1.0f;
float sq = naive
         + polyBlep(phase, dt)                                  // rising edge at 0
         - polyBlep(std::fmod(phase - pw + 1.0f, 1.0f), dt);    // falling edge at pw
```

#### 5.1.4. Triangle (leaky integrator от square)

```cpp
// per-sample: square_out = polyBlep_square(...);
state = 0.999f * state + dt * square_out;  // leaky LP integrator
float tri = state * 4.0f;                  // scale to ~[-1, +1]
```

#### 5.1.5. HyperSaw (7-voice supersaw)

```cpp
// 7 detuned saws, центральный = базовая частота
constexpr int N = 7;
float spread = detuneAmount;  // 0..1
float detuneCents[N] = { -32, -22, -11, 0, 11, 22, 32 };  // примерные
float mix[N]  = { 0.5f, 0.65f, 0.85f, 1.0f, 0.85f, 0.65f, 0.5f };
float out = 0.0f;
for (int i = 0; i < N; ++i) {
    float f_i = f0 * std::pow(2.0f, detuneCents[i] * spread / 1200.0f);
    out += sawPolyBlep(phase[i], f_i / sr) * mix[i];
}
out *= 1.0f / N;
```

Референс: статья Adam Szabo "How to Emulate the Super Saw" — https://www.kvraudio.com/forum/viewtopic.php?t=303536 (или PDF в сети).

#### 5.1.6. Hard sync с BLEP

```cpp
// master определяет phase reset slave
master_phase += dt_master;
if (master_phase >= 1.0f) {
    master_phase -= 1.0f;
    // в момент reset фаза slave прыгает с slave_phase до 0
    float slave_value_before_reset = 2.0f * slave_phase - 1.0f;  // для saw
    // BLEP residual нужно масштабировать на величину разрыва
    blep_amount = slave_value_before_reset - (-1.0f);  // discontinuity height
    slave_phase = (master_phase / dt_master) * dt_slave;  // sub-sample reset
    pending_blep_correction = blep_amount;
}
```

См. Eli Brandt MinBLEP paper, https://www.cs.cmu.edu/~eli/papers/icmc01-hardsync.pdf

#### 5.1.7. Noise

**Pink (Paul Kellet):**
```cpp
float w = (rand01() * 2.0f - 1.0f);
b0 = 0.99886f * b0 + w * 0.0555179f;
b1 = 0.99332f * b1 + w * 0.0750759f;
b2 = 0.96900f * b2 + w * 0.1538520f;
b3 = 0.86650f * b3 + w * 0.3104856f;
b4 = 0.55000f * b4 + w * 0.5329522f;
b5 = -0.7616f * b5 - w * 0.0168980f;
float pink = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362f) * 0.11f;
b6 = w * 0.115926f;
```

#### 5.1.8. DC blocker

```cpp
// R ≈ 0.995 → fc ≈ 38 Hz @ 48 kHz
y = x - x_prev + R * y_prev;
x_prev = x; y_prev = y;
```

### 5.2. TPT State Variable Filter (Andy Simper)

```cpp
struct SvfTpt {
    float ic1eq = 0.0f, ic2eq = 0.0f;

    void process(float in, float fc, float Q, float sr,
                 float& lp, float& bp, float& hp, float& notch) {
        float g  = std::tan(juce::MathConstants<float>::pi * fc / sr);
        float k  = 1.0f / Q;
        float a1 = 1.0f / (1.0f + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;

        float v3 = in - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        lp    = v2;
        bp    = v1;
        hp    = in - k * v1 - v2;
        notch = in - k * v1;
    }
};
```

Paper: https://cytomic.com/files/dsp/SvfLinearTrapezoidal.pdf

### 5.3. Moog Ladder (Huovilainen 2004)

```cpp
struct MoogLadder {
    float stage[4]    = {0,0,0,0};
    float stageZ1[4]  = {0,0,0,0};
    float delay[4]    = {0,0,0,0};
    float fc = 1000.0f, res = 0.5f, sr = 48000.0f;

    float process(float in) {
        const float fcRel = fc / sr;
        const float g = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * fcRel);  // tuning
        const float k = 4.0f * res;  // 0..4 для self-osc

        // passband compensation
        float input = in - k * (stage[3] - 0.5f * in);

        for (int i = 0; i < 4; ++i) {
            float src = (i == 0) ? input : stage[i - 1];
            stage[i] = stageZ1[i] + 2.0f * g * (std::tanh(src) - std::tanh(delay[i]));
            stageZ1[i] = stage[i];
            delay[i] = stage[i];
        }
        return stage[3];  // LP4 output
        // HP = in - 4*stage[0] + 6*stage[1] - 4*stage[2] + stage[3];
        // BP = stage[1] - stage[3] (примерно);
    }
};
```

**Oversampling 2x обязателен** для Moog: tanh + feedback × 4 = aliasing source. Используй `juce::dsp::Oversampling<float>` с FIR halfband или `chowdsp::Upsampler/Downsampler`.

Paper: https://www.dafx.de/paper-archive/2004/P_061.PDF

### 5.4. Envelopes

#### 5.4.1. Linear ADSR (минимум)

```cpp
enum class Stage { Idle, Attack, Decay, Sustain, Release };

struct AdsrLinear {
    Stage stage = Stage::Idle;
    float a, d, s, r;      // секунды для a/d/r, [0..1] для s
    float sr, level = 0.0f, releaseFrom = 0.0f;
    float aInc, dInc, rInc;

    void noteOn() { stage = Stage::Attack; aInc = 1.0f / (a * sr); }
    void noteOff() { releaseFrom = level; stage = Stage::Release; rInc = releaseFrom / (r * sr); }

    float tick() {
        switch (stage) {
            case Stage::Attack:  level += aInc; if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; dInc = (1.0f - s) / (d * sr); } break;
            case Stage::Decay:   level -= dInc; if (level <= s)    { level = s; stage = Stage::Sustain; } break;
            case Stage::Sustain: level = s; break;
            case Stage::Release: level -= rInc; if (level <= 0.0f) { level = 0.0f; stage = Stage::Idle; } break;
            case Stage::Idle:    break;
        }
        return level;
    }
};
```

#### 5.4.2. Exponential ADSR (Pirkle-style, музыкальнее)

```cpp
// один-полюсный приближение target:
//   y[n] = target + (y[n-1] - target) * coeff,  coeff = exp(-1 / (timeSec * sr))
// attack target = 1.0 + overshoot (0.2..0.3), decay/release — sustainLevel или 0.
// "Закрытие" фазы по порогу: |y - target| < eps
```

Pirkle `EG_AnalogADSR` — см. SynthLab SDK.

### 5.5. Effects

#### 5.5.1. RBJ Cookbook biquads (EQ)

Канон: https://www.w3.org/TR/audio-eq-cookbook/. Для каждого band:
```
ω0 = 2π * f0 / fs
α  = sin(ω0) / (2Q)
A  = 10^(gainDb / 40)
```
Для peak EQ:
```
b0 = 1 + α*A,   b1 = -2*cos(ω0),   b2 = 1 - α*A
a0 = 1 + α/A,   a1 = -2*cos(ω0),   a2 = 1 - α/A
```
Нормализовать на `a0` и применять direct-form-II-transposed.

В JUCE: `juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, freq, Q, gainLinear)`.

#### 5.5.2. Dattorro plate reverb

Paper Jon Dattorro — https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf. Структура:
- Pre-delay
- Input diffusion: 4 каскадных allpass (small)
- Tank: 2 петли с modulated allpass + comb-like delay + low-pass damping
- Out-tap matrix (несколько точек съёма с разных delay lines для stereo)

Время на реализацию: 3–5 дней. Альтернатива на старте: `juce::Reverb` (Freeverb-derived, бесплатно, звучит сносно).

ValhallaDSP блог — must-read для понимания: https://valhalladsp.com/blog/

#### 5.5.3. Chorus / Flanger / Phaser

Общий блок: модулированная delay line (`juce::dsp::DelayLine<float, Lagrange3rd>`). Только параметры разные:

| | Delay | Feedback | Mix | LFO depth |
|---|---|---|---|---|
| Chorus  | 15–35 ms | 0 | 50% | 1–5 ms |
| Flanger | 0.5–10 ms | 0.5–0.9 | 50% | 0.5–5 ms |
| Phaser  | — (used 4–12 allpass cascaded) | 0.3–0.7 | 50% | модулирует AP coefficient |

### 5.6. Voice manager (sketch)

```cpp
class VoiceManager {
    static constexpr int kMaxVoices = 32;
    std::array<Voice, kMaxVoices> voices;
    std::array<int, kMaxVoices>   ageStamps;
    int globalAge = 0;

    Voice* findFreeVoice(int midiNote);  // same-note > idle > released-quietest > oldest

public:
    void noteOn(int note, float velocity, int sampleOffset);
    void noteOff(int note, float velRel, int sampleOffset);
    void allNotesOff();
    void renderBlock(juce::AudioBuffer<float>& out, int startSample, int numSamples);
};
```

Stealing pseudo-code:
```cpp
Voice* VoiceManager::findFreeVoice(int note) {
    // 1. same-note retrigger
    for (auto& v : voices) if (v.isPlaying() && v.midiNote == note) return &v;
    // 2. idle voice
    for (auto& v : voices) if (!v.isPlaying()) return &v;
    // 3. released, lowest envelope level
    Voice* candidate = nullptr;
    float lowestLevel = 1.0f;
    for (auto& v : voices)
        if (v.isInRelease() && v.envLevel() < lowestLevel) { lowestLevel = v.envLevel(); candidate = &v; }
    if (candidate) return candidate;
    // 4. oldest
    int oldest = 0;
    for (int i = 1; i < kMaxVoices; ++i)
        if (ageStamps[i] < ageStamps[oldest]) oldest = i;
    return &voices[oldest];
}
```

---

## 6. Open-source референсы — что куда смотреть

| Проект | Лицензия | URL | Что взять оттуда |
|---|---|---|---|
| **Surge XT** | GPLv3 | https://github.com/surge-synthesizer/surge | DSP золотой стандарт: oscillators, 12 filter types, mod matrix, FX. Файлы: `src/common/dsp/oscillators/`, `src/common/dsp/filters/`, `src/common/dsp/effects/`, `src/common/SurgeVoice.cpp` |
| **Vital / Vitalium** | GPLv3 / GPLv3 | https://github.com/mtytel/vital, https://github.com/DISTRHO/DISTRHO-Ports | Wavetable engine + SIMD voice processing |
| **Helm** | GPLv3 | https://github.com/mtytel/helm | Classic subtractive arch, mod matrix |
| **OB-Xd / OB-Xf** | GPLv3 | https://github.com/reales/OB-Xd, https://github.com/surge-synthesizer/OB-Xf | Компактный VA-плагин на JUCE, читаемый код |
| **Dexed** | GPLv3 | https://github.com/asb2m10/dexed | DX7 FM + отличный preset/cartridge management |
| **Odin2** | GPLv3 | https://github.com/TheWaveWarden/odin2 | JUCE-style арх, читаемая APVTS |
| **TAL-NoiseMaker** | various | https://github.com/kunitoki/TAL-NoiseMaker | Minimal VA |
| **chowdsp_utils** | BSD-3 | https://github.com/Chowdhury-DSP/chowdsp_utils | Готовые JUCE-модули filters, waveshapers, oversampling — **используй напрямую** |
| **sst-basic-blocks** | MIT | https://github.com/surge-synthesizer/sst-basic-blocks | Extracted Surge primitives |
| **sst-effects** | GPLv3 | https://github.com/surge-synthesizer/sst-effects | Surge FX как отдельная библиотека |
| **Plaits (Mutable Instr.)** | MIT | https://github.com/pichenettes/eurorack/tree/master/plaits | Гениальные DSP формулы (embedded, но читаемо) |
| **Pamplejuce** | MIT | https://github.com/sudara/pamplejuce | **СТАРТОВЫЙ ШАБЛОН** проекта — CMake/CI/tests |
| **JUCE examples** | mixed | https://github.com/juce-framework/JUCE/tree/master/examples | Эталонные туториалы |

**Книги (обязательно):**

| Автор | Название | Где |
|---|---|---|
| Vadim Zavalishin | The Art of VA Filter Design (rev 2.1.2, free PDF) | https://www.discodsp.net/VAFilterDesign_2.1.2.pdf |
| Will Pirkle | Designing Software Synthesizer Plugins in C++ (2nd ed.) | https://www.routledge.com/9780367510480 |
| Will Pirkle | Designing Audio Effect Plugins in C++ (2nd ed.) | https://www.routledge.com/9781138591899 |
| Udo Zölzer | DAFX: Digital Audio Effects (2nd ed.) | https://www.dafx.de/DAFX_Book_Page_2nd_edition/ |
| Julius O. Smith | Online books (free) | https://ccrma.stanford.edu/~jos/ |

**Статьи / PDF:**

- Välimäki & Huovilainen, "Antialiasing Oscillators in Subtractive Synthesis", IEEE SPM 2007 — https://ieeexplore.ieee.org/document/4117934
- Brandt, MinBLEP / hard sync — https://www.cs.cmu.edu/~eli/papers/icmc01-hardsync.pdf
- Stilson & Smith, BLIT — https://ccrma.stanford.edu/~stilti/papers/blit.pdf
- Niemitalo, Polynomial Interpolators — http://yehar.com/blog/wp-content/uploads/2009/08/deip.pdf
- Cytomic / Andy Simper, SVF — https://cytomic.com/files/dsp/SvfLinearTrapezoidal.pdf и https://cytomic.com/technical-papers/
- Huovilainen, Moog VCF — https://www.dafx.de/paper-archive/2004/P_061.PDF
- Dattorro, Plate Reverb — https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf
- RBJ Cookbook — https://www.w3.org/TR/audio-eq-cookbook/
- Ross Bencina, "Realtime audio programming 101" — http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing
- Sudara, codesign+notarize гайд — https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/

**Видео:**
- Dave Rowland & Fabian Renn-Giles "Real-time 101" CppCon 2019 — https://www.youtube.com/watch?v=Q0vrQFyAdWI
- The Audio Programmer — https://www.youtube.com/@TheAudioProgrammer (плейлист "Build a Synth")

---

## 7. Финальный промт для нейросети-кодера

> Скопируй этот блок в Cursor / Claude Code / Cline в начале каждой фазы. Текст self-contained — нейросеть не обязана читать остальной документ, но ссылается на него.

```markdown
ROLE: Senior C++ audio plugin engineer (10+ years JUCE/VST3/AU).

GOAL: Implement a virtual-analog polyphonic synthesizer plugin (VST3 + AU + Standalone)
inspired by Access Virus TI and Nord Lead A1/4. Target hosts: FL Studio, Logic Pro,
Ableton Live, Cubase, Reaper. macOS + Windows.

STACK: C++20, JUCE 7.0.10+, CMake 3.22+, modern juce_add_plugin (no Projucer).
Use Pamplejuce as project skeleton (https://github.com/sudara/pamplejuce).

NON-NEGOTIABLE RULES (real-time audio thread):
1. No allocations (malloc/new/std::vector::push_back/std::string), no locks
   (mutexes/condvar/spinlocks), no IO (file/network/printf), no exceptions
   (use noexcept everywhere in processBlock and downstream), no syscalls,
   no virtual dispatch in hot loops (use CRTP or std::variant if needed)
   inside processBlock and any code it calls.
2. Communication GUI→Audio is via std::atomic, AbstractFifo, or moodycamel SPSC.
3. APVTS for all DAW-automatable parameters; raw atomic pointer reads in processBlock.
4. Sample-accurate MIDI: split processBlock by midi event positions.
5. Modulation matrix at control rate of 32 samples; pitch/amp envelope per-sample.
6. Custom VoiceManager (NOT juce::Synthesiser) — needs proper voice stealing
   priority: same-note > idle > released-lowest-env > oldest.
7. Use juce::dsp::Oversampling 2x for any nonlinearity (tanh, hard clip,
   resonant filter at high Q, ring mod, hard sync).
8. DC blocker (1-pole HP, R≈0.995) on every oscillator output.
9. Smoothing on every per-block parameter that audio depends on
   (juce::SmoothedValue Multiplicative for frequency, Linear for gain/pan).
10. Buffer-size-agnostic (don't assume specific numSamples).

ARCHITECTURE:
Voice = OSC1 + OSC2 + OSC3/Sub + Noise -> Mixer -> Filter1 -> (saturator) ->
        Filter2 (serial|parallel|split) -> VCA(AmpEnv) -> per-voice pan -> sum bus.
Global FX bus: EQ -> Drive -> Chorus/Phaser -> Delay -> Reverb -> Comp/Limiter.
ModMatrix: 8 user slots + fixed routings; pull-based; sources include
ENV1-3, LFO1-3 (2 per-voice + 1 global), Vel, Note, AT, ModWheel, PB,
Breath, Expression, Sustain, RandomPerNote, RandomPerSample, Constant1.

PER-VOICE FEATURES:
- 3 OSC: PolyBLEP VA (saw/pulse-PWM/tri/sine + sub), HyperSaw (7-saw with per-saw sub),
  Wavetable (mip-mapped, Lagrange-3 interp), Noise (white/pink/filtered).
- Hard sync, ring mod, FM (PM-style), cross-mod between OSCs.
- 2 filters: TPT SVF (Cytomic) and Moog Ladder (Huovilainen 2004) with multimode
  LP12/LP24/HP/BP/Notch outputs, drive, key-tracking.
- Saturator between filters: tanh / soft-poly / hard / fold / bitcrush / rate-reduce.
- 3 envelopes DAHDSR with linear or exponential curves, velocity sens, key-tracking,
  loop mode.
- 3 LFO (2 per-voice + 1 global): 8+ waveforms, free/tempo-sync/key-trig, fade-in,
  phase offset.
- Unison 1-8 voices with detune (cents) + stereo spread + phase init.
- Glide (portamento), mono/legato/poly modes, note priority Last/Low/High.

MIDI:
- MIDI 1.0 + optional MPE (juce::MPESynthesiser pattern).
- Pitch bend (range via RPN 0,0), CC1/2/7/10/11/64/66/120/121/123.
- Channel AT + poly AT as mod source.
- Program change → preset switching (lock-free swap of preset struct pointer).
- MIDI Learn UI (right-click param → next CC binds).

PRESETS:
- juce::ValueTree serialized to binary MemoryBlock (getStateInformation) +
  VST3 .vstpreset via JUCE.
- Metadata: name, author, category, tags[], bpm_hint, description,
  plugin_version, state_version.
- 128 factory presets embedded via juce_binarydata.
- Versioning: state_version int + upgrade chain.

DSP REFERENCES (use these formulas/papers):
- PolyBLEP: Välimäki & Huovilainen "Antialiasing Oscillators in Subtractive Synthesis"
  (IEEE SPM 2007). Pseudocode in spec §5.1.
- Hard sync BLEP: Eli Brandt MinBLEP (CMU). Spec §5.1.6.
- TPT SVF: Andy Simper / Cytomic — https://cytomic.com/files/dsp/SvfLinearTrapezoidal.pdf.
  Spec §5.2.
- Moog ladder: Huovilainen 2004 DAFx P_061. Spec §5.3.
- ZDF/TPT general: Zavalishin "The Art of VA Filter Design" rev 2.1.2.
- RBJ EQ cookbook: https://www.w3.org/TR/audio-eq-cookbook/
- Dattorro plate reverb: https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf
- Pink noise (Paul Kellet): see spec §5.1.7.

PROJECT LAYOUT (target):
```
.
├── CMakeLists.txt
├── source/
│   ├── PluginProcessor.{cpp,h}
│   ├── PluginEditor.{cpp,h}
│   ├── dsp/
│   │   ├── oscillators/  (Polyblep.h, HyperSaw.h, Wavetable.h, Noise.h, OscBank.{cpp,h})
│   │   ├── filters/      (SvfTpt.h, MoogLadder.h, FilterMulti.{cpp,h})
│   │   ├── envelopes/    (Dahdsr.{cpp,h})
│   │   ├── lfo/          (Lfo.{cpp,h})
│   │   ├── voice/        (Voice.{cpp,h}, VoiceManager.{cpp,h})
│   │   ├── modulation/   (ModMatrix.{cpp,h})
│   │   └── effects/      (Eq3.h, Drive.h, ChorusPhaser.{cpp,h}, Delay.{cpp,h},
│   │                      Reverb.{cpp,h}, Comp.h)
│   ├── midi/             (MidiRouter.{cpp,h})
│   ├── params/           (Params.{cpp,h}  // APVTS layout + IDs)
│   ├── presets/          (PresetManager.{cpp,h})
│   └── gui/              (LookAndFeel, KnobBig, ModSourceMenu, ...)
├── resources/  (svgs, factory presets, fonts)
└── tests/      (Catch2 / doctest unit tests for DSP)
```

CURRENT PHASE: <PASTE PHASE FROM ROADMAP §4>
DELIVERABLES THIS PHASE:
<PASTE bullets from that Phase's checklist>

ACCEPTANCE GATES (must all pass before declaring this phase done):
- Builds clean: macOS arm64+x86_64 universal, Windows x64.
- Loads in: Logic Pro, FL Studio, Reaper. (Test the user can hear sound.)
- pluginval --strict-level 7 passes (no failures).
- No realtime allocations: verify with juce::DefaultMessageManager assertions
  + run under macOS Instruments "Allocations" with audio thread filter.
- 30-min continuous playback test: no leaks, no glitches, no rising CPU.
- Unit tests for DSP modules touched in this phase pass (Catch2/doctest).

OUTPUT STYLE:
- Idiomatic modern C++ (RAII, value semantics, [[nodiscard]] on getters, noexcept
  on hot paths, constexpr where possible).
- Each new DSP module = single .h with inline implementation if header-only,
  or .{cpp,h} pair.
- Comments only where they add intent that's not obvious from code; reference
  the paper/equation source for any nontrivial formula.
- No dead code, no commented-out code, no TODO without an associated issue.

START BY READING THE PROJECT STRUCTURE, NOT WRITING CODE. Then state your
implementation plan for the current phase (3-7 bullets), wait for confirmation,
then implement file by file.
```

---

## 8. Тестирование и приёмка

### 8.1. Unit-тесты DSP (offline, Catch2/doctest)

Для каждого DSP-модуля:
- **Oscillator**: спектральный тест — FFT 8192 точек на 1 секунду saw @ A4, проверить отсутствие пиков выше Nyquist (aliasing), измерить SNR aliased components > 80 dB.
- **Filter (SVF, Moog)**: импульсный отклик → FFT → проверить корректность cutoff и slope (12 dB/oct для SVF LP, 24 dB/oct для Moog).
- **ADSR**: проверить достижение sustain level за время A+D, релиз до нуля за R секунд.
- **LFO**: правильная частота при заданном rate, корректная фаза при tempo sync.
- **DelayLine**: единичный импульс → пик ровно через N samples (для целочисленного delay).
- **DC blocker**: DC input → output → 0 за разумное время.

### 8.2. pluginval

```bash
pluginval --strict-level 10 --validate-in-process \
  --skip-gui-tests=false \
  path/to/YourSynth.vst3
```

Должен пройти на 10 (самый строгий).

### 8.3. DAW-тесты (ручные)

| DAW | OS | Format | Что проверить |
|---|---|---|---|
| Logic Pro | macOS | AU | Загрузка, MIDI play, automation, preset save/load, validation (`auval -v aumu CODE MANU`) |
| FL Studio | macOS/Win | VST3 | То же + multi-instance |
| Ableton Live | macOS/Win | VST3 / AU | Plus automation, MIDI clock sync |
| Cubase | macOS/Win | VST3 | Самый строгий к VST3 spec |
| Reaper | macOS/Win | VST3 / AU | Хороший для отладки MIDI events |
| Studio One | macOS/Win | VST3 | Параметры в Channel Strip |
| Bitwig | macOS/Win | VST3 | Modulation map, MPE |

### 8.4. Realtime safety

- macOS Instruments → Allocations с фильтром по audio thread = **0 allocations** в установившемся режиме (warmup допустим).
- Запустить `juce_audio_plugin_client/AU/Validation` mode.
- Lock check: macOS `dispatch_qos_assert` или собственные assert'ы вокруг lock acquire.
- 60-минутный stress: 32 голоса unison × 8 = 256 виртуальных нот, рандомные параметры через MIDI Learn, проверить CPU stable + RAM stable.

### 8.5. Sound design QA

Sound designer (или сам) делает 32 пресета по категориям и оценивает:
- "Звучит как Virus/Nord или как игрушка?"
- Низ есть (sub, kick-bass)?
- Верх не режет уши на высоких нотах (filter не алиасит)?
- Filter self-oscillation музыкален?
- Hypersaw "ширится"?
- FX intelligible (delay не размазывает атаки)?

---

## 9. Безопасность, подпись, дистрибуция

> Релевантно для пользователя с ролью Security Engineer.

### 9.1. Supply chain

- Все зависимости — фиксированные коммиты в `submodules` или CMake `FetchContent` с конкретным GIT_TAG.
- `cmake/dependencies.cmake` — single source of truth для версий.
- Проверка подписи на JUCE при первом клонировании (опционально GPG).
- Dependabot/Renovate для бампа версий с проверкой changelog.

### 9.2. Сборка

- CI — GitHub Actions с pinned runner versions (`macos-13`, `windows-2022`), не `latest`.
- Сборка должна быть детерминистической (Reproducible Builds желательно).
- Включить compiler hardening flags: `-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`, `-fPIE`. На Windows — `/GS /guard:cf /DYNAMICBASE /NXCOMPAT`.
- Address Sanitizer + UB Sanitizer в Debug; включить тестовый прогон unit-тестов под ASan в CI.

### 9.3. macOS code-signing + notarization

```bash
# 1. Build → .component (AU), .vst3 (VST3), .app (Standalone)
# 2. Sign each bundle:
codesign --force --deep --options runtime --timestamp \
  --sign "Developer ID Application: Your Name (TEAMID)" \
  YourSynth.component
codesign --force --deep --options runtime --timestamp \
  --sign "Developer ID Application: Your Name (TEAMID)" \
  YourSynth.vst3
# 3. Zip and submit for notarization:
ditto -c -k --keepParent YourSynth.component YourSynth-AU.zip
xcrun notarytool submit YourSynth-AU.zip \
  --apple-id you@example.com --team-id TEAMID \
  --password "app-specific-password" --wait
# 4. Staple ticket:
xcrun stapler staple YourSynth.component
```

Полный гайд: https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/

### 9.4. Windows code-signing

```powershell
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 ^
  /a YourSynth.vst3
```

EV сертификат → нет SmartScreen warning. OV → 30-day warmup до доверия.

### 9.5. Безопасность плагина в runtime

- Все входные MIDI данные — clamp/validate (note ∈ [0,127], velocity ∈ [0,127], CC ∈ [0,127]). Не верить тому, что DAW шлёт sane data — багнутые DAW существуют.
- Загрузка пресета: проверить версию state, fall back на default если struct corrupted, не делать UB.
- Никаких eval / dynamic code execution. Никаких user-supplied scripts (если кто-то скажет "Lua для модуляции" — нет, это attack surface).
- Запись файлов (preset save) только в `~/Library/...` / `%APPDATA%` — никогда системные локации, никаких suid операций.
- При наличии URL в metadata (custom preset с `description` поле) — не делать никаких HTTP запросов из плагина. Парсить как просто текст.

### 9.6. Privacy

- Никакой телеметрии, аналитики, "phone home" в плагине.
- Если потом захочешь crash reports — отдельный opt-in модуль с явным согласием при первом запуске. Sentry/Crashpad подходят.

---

## 10. Roadmap timeline (ориентир)

| Фаза | Время full-time solo | Время с командой 2 чел |
|---|---|---|
| 0 — Bootstrap | 1–2 дня | 1 день |
| 1 — MIDI + Voice + 1 OSC + ADSR | 3–5 дней | 2–3 дня |
| 2 — Filters + OSC2 + Mixer | 5–7 дней | 3–4 дня |
| 3 — LFO + Mod Matrix | 5–7 дней | 3–4 дня |
| 4 — Unison + HyperSaw + Wavetable | 5–7 дней | 3–4 дня |
| 5 — FX chain | 7–10 дней | 5–7 дней |
| 6 — Presets + GUI | 7–14 дней | 5–10 дней |
| 7 — Sign + Distribute | 3–5 дней | 2–3 дня |
| **Итого** | **~35–55 дней** | **~24–36 дней** |

Это базовый MVP. Sound design (создание 128 хороших пресетов) — отдельная статья, ~2–4 недели звукорежиссёра.

---

## 11. Чек-лист первого дня

- [ ] Установить JUCE 7.x (или подключить через CPM/FetchContent)
- [ ] Установить CMake ≥ 3.22, Xcode (Mac), VS 2022 (Win)
- [ ] Установить **pluginval** (https://github.com/Tracktion/pluginval/releases)
- [ ] Получить Apple Developer ID (если будешь распространять на Mac публично) — $99/год
- [ ] Зарегистрироваться на Steinberg Developer Portal (для VST3 distribution) — бесплатно
- [ ] Сгенерировать `PLUGIN_MANUFACTURER_CODE` (4-байтный) и `PLUGIN_CODE` (4-байтный), уникальные
- [ ] Клонировать Pamplejuce, переименовать, проверить что плагин-скелет открывается в Logic / FL
- [ ] Настроить CI (GitHub Actions): macos-13 + windows-2022, плагин-build + pluginval
- [ ] Создать репо, init commit с этим документом в `docs/VA_SYNTH_SPEC.md`

---

## 12. Открытые вопросы / решения отложить до Phase 4+

- **MPE поддержка** — Phase 3+ если нужна, иначе skip.
- **Multi-timbral (4-slot architecture как Nord)** — большой scope, рассмотреть после MVP.
- **Arpeggiator** — Phase 4–5, после mod matrix.
- **Vocoder** — пропустить в MVP, очень специфический модуль.
- **Atomizer-style audio input processing** — пропустить, Virus-фича для hardware.
- **GUI framework**: JUCE Component vs Foleys vs custom OpenGL — решить в Phase 6, не блокирующее.

---

## 13. Кратко: что отличает этот синт от "ещё одного VA"

Эти фичи — обязательны, иначе синт будет "очередным":

1. **HyperSaw на dedicated OSC** (Virus-style), не просто unison.
2. **Dual filter с serial/parallel/split routing** + saturator между.
3. **Глубокая mod matrix** (8 user slots + side-chain modulators).
4. **Per-voice LFO** (не только global).
5. **Множество wavetables** + smooth scan через mod.
6. **TPT/ZDF фильтры** (не bilinear) — звучат заметно лучше на резонансе.
7. **Хороший reverb** built-in (Dattorro plate, не Freeverb).
8. **Polished GUI** с modulation visualization (modulated value ring на кнопках).

---

**Конец документа.**
