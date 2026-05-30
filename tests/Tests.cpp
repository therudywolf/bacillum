// Offline DSP unit tests (Catch2 v3). Build with -DBACILLUM_BUILD_TESTS=ON.
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

#include "dsp/oscillators/Polyblep.h"
#include "dsp/oscillators/Oscillator.h"
#include "dsp/oscillators/HyperSaw.h"
#include "dsp/oscillators/Wavetable.h"
#include "dsp/oscillators/Noise.h"
#include "dsp/envelopes/Adsr.h"
#include "dsp/filters/SvfTpt.h"
#include "dsp/filters/MoogLadder.h"
#include "dsp/DcBlocker.h"
#include "dsp/effects/Delay.h"
#include "dsp/effects/Saturator.h"
#include "dsp/effects/Eq3.h"
#include "dsp/lfo/Lfo.h"
#include "params/Params.h"

using namespace bacillum;
using Catch::Approx;

static bool allFinite (const std::vector<float>& v)
{
    for (float x : v) if (! std::isfinite (x)) return false;
    return true;
}

TEST_CASE ("PolyBLEP residual is zero away from discontinuities", "[osc]")
{
    const float dt = 0.01f;
    REQUIRE (dsp::polyBlep (0.5f, dt) == Approx (0.0f));   // mid-phase: no correction
    REQUIRE (dsp::polyBlep (0.0f, dt) != Approx (0.0f));   // at the edge: non-zero
}

TEST_CASE ("Saw oscillator is band-limited (alias floor well below fundamental)", "[osc][spectrum]")
{
    constexpr int order = 14, N = 1 << order;
    const double sr = 48000.0;
    const float  f0 = 440.0f;

    dsp::Oscillator osc;
    osc.prepare (sr);
    osc.setWaveform (params::Waveform::Saw);
    osc.setFrequency (f0);

    std::vector<float> buf ((size_t) N * 2, 0.0f);
    for (int i = 0; i < N; ++i) buf[(size_t) i] = osc.tick();
    REQUIRE (allFinite (buf));

    juce::dsp::WindowingFunction<float> win ((size_t) N, juce::dsp::WindowingFunction<float>::hann);
    win.multiplyWithWindowingTable (buf.data(), (size_t) N);
    juce::dsp::FFT fft (order);
    fft.performFrequencyOnlyForwardTransform (buf.data());

    const int   fundBin = (int) std::round (f0 * N / sr);
    const float fundMag = buf[(size_t) fundBin];
    REQUIRE (fundMag > 0.0f);

    // Largest magnitude in bins that are NOT near a harmonic of f0.
    float maxAlias = 0.0f;
    for (int bin = 4; bin < N / 2; ++bin)
    {
        const float harmonic = (float) bin * (float) sr / (float) N / f0;   // bin freq / f0
        const float nearest  = std::round (harmonic);
        if (std::abs (harmonic - nearest) < 0.25f) continue;                // skip harmonic bins
        maxAlias = std::max (maxAlias, buf[(size_t) bin]);
    }
    // PolyBLEP should keep folded aliasing far below the fundamental.
    REQUIRE (maxAlias < fundMag * 0.25f);
}

TEST_CASE ("Linear ADSR reaches sustain and releases to zero", "[env]")
{
    const double sr = 48000.0;
    dsp::AdsrLinear env;
    env.prepare (sr);
    env.setAttack (0.01f); env.setDecay (0.02f); env.setSustain (0.5f); env.setRelease (0.02f);

    env.noteOn();
    REQUIRE (env.isActive());

    // After attack+decay (~30 ms) the level should sit at sustain.
    for (int i = 0; i < (int) (0.05 * sr); ++i) env.tick();
    REQUIRE (env.getLevel() == Approx (0.5f).margin (0.02f));

    env.noteOff();
    for (int i = 0; i < (int) (0.05 * sr); ++i) env.tick();
    REQUIRE (env.getLevel() == Approx (0.0f).margin (0.001f));
    REQUIRE_FALSE (env.isActive());
}

TEST_CASE ("DC blocker removes a constant offset", "[filter]")
{
    dsp::DcBlocker dc;
    dc.prepare (48000.0);
    float out = 0.0f;
    for (int i = 0; i < 20000; ++i) out = dc.process (1.0f);   // constant DC input
    REQUIRE (out == Approx (0.0f).margin (0.01f));
}

TEST_CASE ("SVF low-pass passes DC, high-pass blocks it", "[filter]")
{
    const double sr = 48000.0;

    dsp::SvfTpt lp; lp.prepare (sr); lp.setMode (dsp::SvfTpt::Mode::LP12); lp.setCutoff (1000.0f); lp.setQ (0.707f);
    dsp::SvfTpt hp; hp.prepare (sr); hp.setMode (dsp::SvfTpt::Mode::HP);   hp.setCutoff (1000.0f); hp.setQ (0.707f);

    float lo = 0.0f, hi = 0.0f;
    for (int i = 0; i < 8000; ++i) { lo = lp.process (1.0f); hi = hp.process (1.0f); }
    REQUIRE (lo == Approx (1.0f).margin (0.02f));   // DC passes the LP
    REQUIRE (hi == Approx (0.0f).margin (0.02f));   // DC blocked by the HP
}

TEST_CASE ("Moog ladder stays stable at maximum resonance", "[filter]")
{
    dsp::MoogLadder m; m.prepare (48000.0); m.setCutoff (1200.0f); m.setResonance01 (1.0f);
    dsp::WhiteNoise n;
    float peak = 0.0f;
    for (int i = 0; i < 48000; ++i)
    {
        const float y = m.process (n.tick());
        REQUIRE (std::isfinite (y));
        peak = std::max (peak, std::abs (y));
    }
    REQUIRE (peak < 10.0f);   // self-oscillates but never blows up
}

TEST_CASE ("Stereo delay reproduces an impulse after the set time", "[fx]")
{
    const double sr = 48000.0;
    dsp::StereoDelay d; d.prepare (sr, 1.0);
    d.setMix (1.0f); d.setFeedback (0.0f); d.setCrossFeedback (0.0f);
    d.setDampingCutoff (20000.0f);
    const float seconds = 0.01f;
    d.setTimes (seconds, seconds);

    std::vector<float> L (2000, 0.0f), R (2000, 0.0f);
    L[0] = R[0] = 1.0f;
    d.process (L.data(), R.data(), 2000);

    int peakIdx = 0; float peak = 0.0f;
    for (int i = 1; i < 2000; ++i) if (std::abs (L[(size_t) i]) > peak) { peak = std::abs (L[(size_t) i]); peakIdx = i; }
    const int expected = (int) (seconds * sr);
    REQUIRE (peakIdx == Approx (expected).margin (4));
}

TEST_CASE ("Saturator output stays bounded", "[fx]")
{
    dsp::Saturator sat;
    for (auto type : { params::SaturatorType::Tanh, params::SaturatorType::SoftClip,
                       params::SaturatorType::HardClip, params::SaturatorType::Foldback })
    {
        sat.setType (type); sat.setAmount (1.0f); sat.reset();
        for (float x = -4.0f; x <= 4.0f; x += 0.05f)
        {
            const float y = sat.process (x);
            REQUIRE (std::isfinite (y));
            REQUIRE (std::abs (y) < 2.0f);
        }
    }
}

TEST_CASE ("3-band EQ is transparent at 0 dB", "[fx]")
{
    dsp::Eq3 eq; eq.prepare (48000.0);
    eq.setParams (120.0f, 0.0f, 1000.0f, 0.0f, 0.7f, 8000.0f, 0.0f);

    std::vector<float> L, R;
    dsp::WhiteNoise n;
    for (int i = 0; i < 512; ++i) { const float s = n.tick(); L.push_back (s); R.push_back (s); }
    auto in = L;
    eq.process (L.data(), R.data(), (int) L.size());
    // Flat EQ → biquads settle to unity; compare after the transient.
    for (size_t i = 64; i < L.size(); ++i)
        REQUIRE (L[i] == Approx (in[i]).margin (1.0e-3f));
}

TEST_CASE ("Wavetable oscillator is bounded and non-silent", "[osc]")
{
    dsp::WavetableOsc wt; wt.prepare (48000.0); wt.setFrequency (220.0f);
    float peak = 0.0f;
    for (float pos = 0.0f; pos <= 1.0f; pos += 0.25f)
    {
        wt.setPosition (pos);
        for (int i = 0; i < 4096; ++i)
        {
            const float y = wt.tick();
            REQUIRE (std::isfinite (y));
            REQUIRE (std::abs (y) < 2.0f);
            peak = std::max (peak, std::abs (y));
        }
    }
    REQUIRE (peak > 0.1f);
}

TEST_CASE ("HyperSaw is bounded", "[osc]")
{
    dsp::HyperSaw hs; hs.prepare (48000.0); hs.setFrequency (110.0f); hs.setDetune (0.7f); hs.setMix (0.6f);
    for (int i = 0; i < 8192; ++i)
    {
        const float y = hs.tick();
        REQUIRE (std::isfinite (y));
        REQUIRE (std::abs (y) < 2.0f);
    }
}

TEST_CASE ("Mod-matrix curves shape and stay bounded", "[mod]")
{
    using params::ModCurve;
    using params::applyModCurve;

    REQUIRE (applyModCurve (ModCurve::Linear,    0.5f)  == Approx (0.5f));
    REQUIRE (applyModCurve (ModCurve::Quadratic, 0.5f)  == Approx (0.25f));
    REQUIRE (applyModCurve (ModCurve::Quadratic, -0.5f) == Approx (-0.25f));   // sign preserved
    REQUIRE (applyModCurve (ModCurve::Exponential, 0.0f) == Approx (0.0f).margin (1.0e-6f));
    REQUIRE (applyModCurve (ModCurve::Exponential, 1.0f) == Approx (1.0f));
    REQUIRE (applyModCurve (ModCurve::SCurve, 0.5f) == Approx (0.5f));         // smoothstep(0.5)=0.5

    for (float x = -1.0f; x <= 1.0f; x += 0.1f)
        for (auto c : { ModCurve::Linear, ModCurve::Exponential, ModCurve::Quadratic, ModCurve::SCurve })
            REQUIRE (std::abs (applyModCurve (c, x)) <= 1.0001f);
}

TEST_CASE ("LFO completes one cycle at its set rate", "[lfo]")
{
    const double sr = 48000.0;
    dsp::Lfo lfo; lfo.prepare (sr); lfo.setShape (dsp::Lfo::Shape::Sine); lfo.setRateHz (10.0f);
    lfo.reset();
    const int period = (int) (sr / 10.0);
    const float first = lfo.tick();
    for (int i = 1; i < period; ++i) lfo.tick();
    const float afterOne = lfo.tick();
    REQUIRE (afterOne == Approx (first).margin (0.05f));   // back near the start
}
