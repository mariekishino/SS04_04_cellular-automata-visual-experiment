#include <cmath>
#include <cstdio>
#include "core/input.hpp"
#include "io/wav.hpp"
#include "check.hpp"
using namespace cave;
int main() {
    // pulse timing: start 2, duration 0.5, period 2, count 2
    PulseParams p; p.start = 2.0; p.duration = 0.5; p.period = 2.0; p.count = 2; p.amplitude = 0.8f;
    PulseSource ps(p);
    CHECK(ps.amplitude(1.99) == 0.0f);
    CHECK(ps.amplitude(2.0) == 0.8f);
    CHECK(ps.amplitude(2.49) == 0.8f);
    CHECK(ps.amplitude(2.5) == 0.0f);
    CHECK(ps.amplitude(4.1) == 0.8f);   // second pulse
    CHECK(ps.amplitude(6.1) == 0.0f);   // no third pulse
    CHECK_NEAR(ps.duration_seconds(), 4.5, 1e-12);
    PulseParams inf = p; inf.count = -1;
    CHECK(PulseSource(inf).amplitude(100.1) == 0.8f);
    // a single pulse longer than the default period is not cut short (regression: Phase 2/3 "10 s" pulses)
    PulseParams lng; lng.start = 2.0; lng.duration = 10.0; lng.count = 1;   // period stays at the default 2.0
    CHECK(PulseSource(lng).amplitude(5.0) == 1.0f);
    CHECK(PulseSource(lng).amplitude(11.9) == 1.0f);
    CHECK(PulseSource(lng).amplitude(12.0) == 0.0f);
    // repeating pulses longer than the period merge into a continuous one
    PulseParams ov; ov.start = 0.0; ov.duration = 1.5; ov.period = 1.0; ov.count = 3;
    CHECK(PulseSource(ov).amplitude(0.5) == 1.0f && PulseSource(ov).amplitude(1.9) == 1.0f && PulseSource(ov).amplitude(3.4) == 1.0f && PulseSource(ov).amplitude(3.6) == 0.0f);

    // WAV round trip: 1 s of 0.5-amplitude sine at 8 kHz
    const int sr = 8000;
    std::vector<float> s(sr);
    for (int i = 0; i < sr; ++i) s[i] = 0.5f * std::sin(2.0f * 3.14159265f * 100.0f * i / sr);
    const std::string path = "test_input_tmp.wav";
    CHECK(write_wav_pcm16_mono(path, sr, s));
    WavData w; std::string err;
    CHECK(read_wav(path, w, &err));
    CHECK(w.sample_rate == sr && w.channels == 1 && w.bits == 16 && !w.is_float);
    CHECK(w.mono.size() == s.size());
    CHECK_NEAR(w.mono[20], s[20], 1e-3);
    std::remove(path.c_str());

    // RMS of a sine = amplitude / sqrt(2); 20 frames per second -> 20 windows
    std::vector<float> env = rms_envelope(w.mono, sr, 20.0);
    CHECK(env.size() == 20);
    CHECK_NEAR(env[5], 0.5 / std::sqrt(2.0), 2e-3);
    // smoothing: rises monotonically toward the level, slower with larger tau
    std::vector<float> step(50, 1.0f); step[0] = 0.0f;
    std::vector<float> a = smooth_ema(step, 20.0, 0.1), b = smooth_ema(step, 20.0, 1.0);
    CHECK(a[1] > 0.0f && a[1] < 1.0f && a[10] > a[1] && b[10] < a[10]);
    CHECK(smooth_ema(step, 20.0, 0.0) == step);
    // normalization
    std::vector<float> n = {0.2f, 0.4f, 0.1f};
    CHECK_NEAR(normalize_peak(n), 0.4, 1e-7); CHECK_NEAR(n[1], 1.0, 1e-7); CHECK_NEAR(n[0], 0.5, 1e-7);
    std::vector<float> z = {0.0f, 0.0f};
    CHECK(normalize_peak(z) == 0.0f);
    // envelope source lookup
    EnvelopeSource es({0.0f, 0.5f, 1.0f}, {0.0f, 0.25f, 0.5f}, 10.0, "test");
    CHECK(es.amplitude(0.15) == 0.25f && es.raw_amplitude(0.15) == 0.5f && es.amplitude(5.0) == 0.0f);
    CHECK_NEAR(es.duration_seconds(), 0.3, 1e-12);
    std::puts("test_input OK");
    return 0;
}
