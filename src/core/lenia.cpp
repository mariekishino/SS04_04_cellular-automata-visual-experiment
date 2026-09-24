#include "core/lenia.hpp"

#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>
#include <cstring>
#include <sstream>

namespace cave {

Lenia::Lenia(int width, int height, const LeniaParams& p)
    : p_(p), a_(width, height, 0.0f), next_(width, height, 0.0f) {
    build_kernel();
    build_index_tables();
}

float Lenia::kernel_core(float r) {
    if (r <= 0.0f || r >= 1.0f) return 0.0f;
    return std::exp(4.0f - 1.0f / (r * (1.0f - r)));
}

float Lenia::growth(float u, float mu, float sigma) {
    const float d = (u - mu) / sigma;
    return 2.0f * std::exp(-0.5f * d * d) - 1.0f;
}

void Lenia::build_kernel() {
    const int R = p_.R;
    const int K = 2 * R + 1;
    k_.assign(static_cast<std::size_t>(K) * K, 0.0f);
    double sum = 0.0;
    for (int dy = -R; dy <= R; ++dy) {
        for (int dx = -R; dx <= R; ++dx) {
            const float r = std::sqrt(static_cast<float>(dx * dx + dy * dy)) / static_cast<float>(R);
            const float w = kernel_core(r);
            k_[static_cast<std::size_t>(dy + R) * K + (dx + R)] = w;
            sum += w;
        }
    }
    for (float& w : k_) w = static_cast<float>(w / sum);
}

void Lenia::build_index_tables() {
    const int R = p_.R;
    const int W = a_.width(), H = a_.height();
    xi_.assign(static_cast<std::size_t>(W + 2 * R), -1);
    yi_.assign(static_cast<std::size_t>(H + 2 * R), -1);
    for (int i = 0; i < W + 2 * R; ++i) {
        const int x = i - R;
        xi_[i] = (p_.boundary == Boundary::Periodic) ? Grid::wrap(x, W) : ((x >= 0 && x < W) ? x : -1);
    }
    for (int i = 0; i < H + 2 * R; ++i) {
        const int y = i - R;
        yi_[i] = (p_.boundary == Boundary::Periodic) ? Grid::wrap(y, H) : ((y >= 0 && y < H) ? y : -1);
    }
}

const char* stim_mode_name(StimMode m) {
    switch (m) {
        case StimMode::None: return "none";
        case StimMode::Growth: return "growth";
        case StimMode::Mu: return "mu";
    }
    return "none";
}

StimMode stim_mode_from_name(const char* s) {
    if (std::strcmp(s, "growth") == 0) return StimMode::Growth;
    if (std::strcmp(s, "mu") == 0) return StimMode::Mu;
    return StimMode::None;
}

void Lenia::step(const Stimulus& stim) {
    const int R = p_.R;
    const int K = 2 * R + 1;
    const int W = a_.width(), H = a_.height();
    const float* a = a_.raw().data();
    float* out = next_.raw().data();
    // drive = gain * s(t). Exactly 0 when there is no stimulus or the mode is None,
    // so the branch below leaves the Phase 1 arithmetic untouched.
    // Phase 3: the slow state seen by this step is the one accumulated from PAST stimuli
    // (read before update), so the response depends on history, not on the current sample.
    float gain_eff = p_.stim_gain;
    if (p_.adapt_k > 0.0f) {
        gain_eff = p_.stim_gain / (1.0f + p_.adapt_k * m_);
        const float alpha = 1.0f - std::exp(-1.0f / p_.adapt_tau_steps);
        m_ += alpha * (stim.amplitude - m_);
    }
    const float drive = (p_.stim_mode == StimMode::None) ? 0.0f : gain_eff * stim.amplitude;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // Potential U = K * A. Reads only from a_ (current), writes only to next_.
            double u = 0.0;
            for (int dy = -R; dy <= R; ++dy) {
                const int yy = yi_[y + dy + R];
                if (yy < 0) continue;  // Fixed boundary: outside reads 0, contributes nothing
                const float* row = a + static_cast<std::size_t>(yy) * W;
                const float* krow = k_.data() + static_cast<std::size_t>(dy + R) * K;
                for (int dx = -R; dx <= R; ++dx) {
                    const int xx = xi_[x + dx + R];
                    if (xx < 0) continue;
                    u += static_cast<double>(krow[dx + R]) * row[xx];
                }
            }
            float g;
            if (drive != 0.0f && p_.stim_mode == StimMode::Mu) {
                g = growth(static_cast<float>(u), p_.mu + drive * stimulus_weight(stim.shape, x, y, W, H), p_.sigma);
            } else {
                g = growth(static_cast<float>(u), p_.mu, p_.sigma);
                if (drive != 0.0f) g += drive * stimulus_weight(stim.shape, x, y, W, H);  // Growth mode
            }
            const float v = a[static_cast<std::size_t>(y) * W + x] + p_.dt * g;
            out[static_cast<std::size_t>(y) * W + x] = std::min(1.0f, std::max(0.0f, v));
        }
    }
    a_.swap(next_);
    ++steps_;
}

std::vector<Channel> Lenia::channels() const {
    return {{"A", &a_}};
}

std::vector<std::pair<std::string, std::string>> Lenia::parameters() const {
    auto f = [](float v) { std::ostringstream os; os.precision(6); os << v; return os.str(); };
    return {
        {"R", std::to_string(p_.R)},
        {"mu", f(p_.mu)},
        {"sigma", f(p_.sigma)},
        {"dt", f(p_.dt)},
        {"kernel_core", "exp(4 - 1/(r(1-r)))"},
        {"growth", "2 exp(-(u-mu)^2 / (2 sigma^2)) - 1"},
        {"stim_mode", stim_mode_name(p_.stim_mode)},
        {"stim_gain", f(p_.stim_gain)},
        {"adapt_k", f(p_.adapt_k)},
        {"adapt_tau_steps", f(p_.adapt_tau_steps)},
        {"adapt_rule", "m += (1-exp(-1/tau))(s-m); g_eff = gain/(1+k m)"},
    };
}

std::string Lenia::formula() const {
    return "Lenia-type (Chan 2019, kn=1, gn=1, b=[1]): A' = clip(A + dt * G(K * A), 0, 1)";
}

std::vector<std::pair<std::string, double>> Lenia::slow_states() const {
    return {{"adapt_m", static_cast<double>(m_)}};
}

void Lenia::save_state(std::ostream& os) const {
    os.write(reinterpret_cast<const char*>(&steps_), sizeof(steps_));
    os.write(reinterpret_cast<const char*>(&m_), sizeof(m_));
    write_grid(os, a_);
}

void Lenia::load_state(std::istream& is) {
    is.read(reinterpret_cast<char*>(&steps_), sizeof(steps_));
    is.read(reinterpret_cast<char*>(&m_), sizeof(m_));
    read_grid(is, a_);
    next_ = Grid(a_.width(), a_.height(), 0.0f);
    build_index_tables();
}

}  // namespace cave
