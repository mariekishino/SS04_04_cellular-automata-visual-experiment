#include "core/input.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace cave {

float PulseSource::amplitude(double t) const {
    if (t < p_.start || p_.duration <= 0.0) return 0.0f;
    const double rel = t - p_.start;
    // A single pulse (count 1) or no period: one window of `duration`.
    if (p_.count == 1 || p_.period <= 0.0) return rel < p_.duration ? p_.amplitude : 0.0f;
    // Repeating pulses. A pulse longer than the period overlaps the next one, so check every
    // pulse that could still be active at `rel`, not only the most recent one.
    const long k_hi = static_cast<long>(std::floor(rel / p_.period));
    const long span = static_cast<long>(std::ceil(p_.duration / p_.period));
    for (long k = k_hi; k >= 0 && k >= k_hi - span; --k) {
        if (p_.count >= 0 && k >= p_.count) continue;
        const double off = rel - static_cast<double>(k) * p_.period;
        if (off >= 0.0 && off < p_.duration) return p_.amplitude;
    }
    return 0.0f;
}

double PulseSource::duration_seconds() const {
    if (p_.count < 0) return 0.0;
    return p_.start + (p_.count > 0 ? (p_.count - 1) * p_.period + p_.duration : 0.0);
}

std::string PulseSource::describe() const {
    std::ostringstream os;
    os << "pulse start=" << p_.start << " duration=" << p_.duration << " period=" << p_.period << " count=" << p_.count << " amplitude=" << p_.amplitude;
    return os.str();
}

EnvelopeSource::EnvelopeSource(std::vector<float> raw, std::vector<float> smoothed, double rate_hz, std::string description)
    : raw_(std::move(raw)), smooth_(std::move(smoothed)), rate_(rate_hz), desc_(std::move(description)) {}

float EnvelopeSource::amplitude(double t) const {
    if (t < 0.0) return 0.0f;
    const std::size_t i = static_cast<std::size_t>(t * rate_);
    return i < smooth_.size() ? smooth_[i] : 0.0f;
}

float EnvelopeSource::raw_amplitude(double t) const {
    if (t < 0.0) return 0.0f;
    const std::size_t i = static_cast<std::size_t>(t * rate_);
    return i < raw_.size() ? raw_[i] : 0.0f;
}

double EnvelopeSource::duration_seconds() const { return static_cast<double>(smooth_.size()) / rate_; }

void EnvelopeSource::truncate(double seconds) {
    const std::size_t n = static_cast<std::size_t>(std::max(0.0, seconds) * rate_);
    if (n < raw_.size()) raw_.resize(n);
    if (n < smooth_.size()) smooth_.resize(n);
    desc_ += " [truncated at " + std::to_string(seconds) + " s]";
}

float CompositeSource::amplitude(double t) const {
    float a = 0.0f;
    for (const auto& p : parts_) a = std::max(a, p->amplitude(t));
    return a;
}
float CompositeSource::raw_amplitude(double t) const {
    float a = 0.0f;
    for (const auto& p : parts_) a = std::max(a, p->raw_amplitude(t));
    return a;
}
double CompositeSource::duration_seconds() const {
    double d = 0.0;
    for (const auto& p : parts_) { if (p->duration_seconds() == 0.0) return 0.0; d = std::max(d, p->duration_seconds()); }
    return d;
}
std::string CompositeSource::describe() const {
    std::string s = "max of {";
    for (std::size_t i = 0; i < parts_.size(); ++i) s += (i ? "; " : "") + parts_[i]->describe();
    return s + "}";
}

std::vector<float> rms_envelope(const std::vector<float>& mono, int sample_rate, double frames_per_second) {
    const double win_d = sample_rate / frames_per_second;
    const std::size_t win = std::max<std::size_t>(1, static_cast<std::size_t>(win_d + 0.5));
    std::vector<float> env;
    env.reserve(mono.size() / win + 1);
    for (std::size_t start = 0; start < mono.size(); start += win) {
        const std::size_t end = std::min(mono.size(), start + win);
        double acc = 0.0;
        for (std::size_t i = start; i < end; ++i) acc += static_cast<double>(mono[i]) * mono[i];
        env.push_back(static_cast<float>(std::sqrt(acc / static_cast<double>(end - start))));
    }
    return env;
}

std::vector<float> smooth_ema(const std::vector<float>& env, double frames_per_second, double tau_seconds) {
    if (tau_seconds <= 0.0 || env.empty()) return env;
    const double alpha = 1.0 - std::exp(-1.0 / (tau_seconds * frames_per_second));
    std::vector<float> out(env.size());
    double y = 0.0;
    for (std::size_t i = 0; i < env.size(); ++i) {
        y += alpha * (env[i] - y);
        out[i] = static_cast<float>(y);
    }
    return out;
}

float normalize_peak(std::vector<float>& env) {
    float peak = 0.0f;
    for (float v : env) peak = std::max(peak, v);
    if (peak > 0.0f) for (float& v : env) v = std::min(1.0f, std::max(0.0f, v / peak));
    return peak;
}

}  // namespace cave
