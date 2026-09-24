#pragma once
// InputSource: produces the stimulus amplitude s(t) for a real time t (seconds).
// The simulation maps its step k to real time by k / steps_per_second.
//
//   PulseSource    synthetic: `count` pulses of `amplitude`, each `duration`
//                  seconds long, starting at `start`, repeating every `period`.
//   EnvelopeSource an amplitude series sampled at a fixed rate, e.g. the RMS
//                  envelope of a WAV file (see rms_envelope / smooth_ema).
#include <memory>
#include <string>
#include <vector>

namespace cave {

class InputSource {
public:
    virtual ~InputSource() = default;
    virtual float amplitude(double t_seconds) const = 0;       // smoothed, what the model gets
    virtual float raw_amplitude(double t_seconds) const { return amplitude(t_seconds); }
    virtual double duration_seconds() const = 0;              // 0 = unbounded
    virtual std::string describe() const = 0;                 // for config.json
};

struct PulseParams {
    double start = 2.0;
    double duration = 0.5;
    double period = 2.0;
    int count = 1;             // -1 = repeat forever
    float amplitude = 1.0f;
};

class PulseSource : public InputSource {
public:
    explicit PulseSource(const PulseParams& p) : p_(p) {}
    float amplitude(double t) const override;
    double duration_seconds() const override;
    std::string describe() const override;
private:
    PulseParams p_;
};

class EnvelopeSource : public InputSource {
public:
    EnvelopeSource(std::vector<float> raw, std::vector<float> smoothed, double rate_hz, std::string description);
    float amplitude(double t) const override;
    float raw_amplitude(double t) const override;
    double duration_seconds() const override;
    std::string describe() const override { return desc_; }
    const std::vector<float>& smoothed() const { return smooth_; }
    void truncate(double seconds);   // silence from `seconds` on
private:
    std::vector<float> raw_, smooth_;
    double rate_;
    std::string desc_;
};

// Max of several sources: a history pattern plus a probe pulse, for example.
class CompositeSource : public InputSource {
public:
    void add(std::unique_ptr<InputSource> s) { parts_.push_back(std::move(s)); }
    std::size_t size() const { return parts_.size(); }
    float amplitude(double t) const override;
    float raw_amplitude(double t) const override;
    double duration_seconds() const override;
    std::string describe() const override;
private:
    std::vector<std::unique_ptr<InputSource>> parts_;
};

// RMS over consecutive windows of sample_rate / frames_per_second samples.
std::vector<float> rms_envelope(const std::vector<float>& mono, int sample_rate, double frames_per_second);
// Exponential moving average with time constant tau (seconds). tau <= 0 = no smoothing.
std::vector<float> smooth_ema(const std::vector<float>& env, double frames_per_second, double tau_seconds);
// Divide by the peak so the maximum is 1, then clamp to [0,1]. Returns the peak used (0 if all zero).
float normalize_peak(std::vector<float>& env);

}  // namespace cave
