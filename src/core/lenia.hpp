#pragma once
// Lenia-type continuous cellular automaton (single channel, single ring).
//
// Simplified from B. Chan, "Lenia: Biology of Artificial Life" (2019),
// https://arxiv.org/abs/1812.05433 and the reference code at
// https://github.com/Chakazul/Lenia (kn=1 exponential kernel core,
// gn=1 exponential growth, one ring b=[1]).
//
//   r        = |dx,dy| / R                          (normalized distance)
//   K(r)     = exp(4 - 1 / (r (1 - r)))   for 0 < r < 1, else 0
//   K        = K / sum(K)                            (kernel sums to 1)
//   U(x)     = sum_{n} K(n) * A(x + n)               (potential, direct convolution)
//   G(u)     = 2 exp(-(u - mu)^2 / (2 sigma^2)) - 1  (growth in [-1, 1])
//   A'(x)    = clip(A(x) + dt * G(U(x)), 0, 1)       (dt = 1 / T)
//
// Stimulus (Phase 2): see StimMode. With amplitude 0 the update is identical
// to the stimulus-free one, bit for bit.
// Adaptation (Phase 3): a scalar slow state m tracks the stimulus with time
// constant tau; the effective gain is gain / (1 + k m). With k = 0 the Phase 2
// arithmetic is untouched.
//
// Cost per step: W * H * (2R+1)^2 multiply-adds.

#include <string>
#include <vector>

#include "core/model.hpp"

namespace cave {

enum class StimMode {
    None,    // stimulus ignored
    Growth,  // A' = clip(A + dt * (G(U) + gain * s * h(x)))
    Mu       // mu_eff(x) = mu + gain * s * h(x)
};
const char* stim_mode_name(StimMode m);
StimMode stim_mode_from_name(const char* s);

struct LeniaParams {
    int R = 13;             // kernel radius in cells
    float mu = 0.15f;       // growth center
    float sigma = 0.015f;   // growth width
    float dt = 0.1f;        // time step = 1 / T
    Boundary boundary = Boundary::Periodic;
    StimMode stim_mode = StimMode::None;
    float stim_gain = 0.0f; // Growth: added to G (G is in [-1,1]); Mu: added to mu (mu is 0.15)
    // Phase 3 adaptation: m += alpha (s - m), alpha = 1 - exp(-1/tau); g_eff = gain / (1 + k m).
    float adapt_k = 0.0f;            // 0 = off (Phase 2 arithmetic untouched)
    float adapt_tau_steps = 600.0f;  // time constant in steps (600 = 10 s at 60 steps/s)
};

class Lenia : public Model {
public:
    Lenia(int width, int height, const LeniaParams& p);

    std::string name() const override { return "lenia"; }
    void step(const Stimulus& stim) override;
    using Model::step;
    float dt() const override { return p_.dt; }
    Boundary boundary() const override { return p_.boundary; }
    std::vector<Channel> channels() const override;
    std::vector<std::pair<std::string, std::string>> parameters() const override;
    std::string formula() const override;
    void save_state(std::ostream& os) const override;
    void load_state(std::istream& is) override;

    Grid& state() { return a_; }             // for setting initial conditions
    const Grid& state() const { return a_; }
    const std::vector<float>& kernel() const { return k_; }
    int kernel_size() const { return 2 * p_.R + 1; }
    const LeniaParams& params() const { return p_; }
    float adaptation() const { return m_; }   // slow state m in [0,1]
    std::vector<std::pair<std::string, double>> slow_states() const override;

    static float kernel_core(float r);
    static float growth(float u, float mu, float sigma);

private:
    void build_kernel();
    void build_index_tables();

    LeniaParams p_;
    float m_ = 0.0f;   // slow state: EMA of the stimulus amplitude (Phase 3)
    Grid a_;        // current state A
    Grid next_;     // written during step(), then swapped
    std::vector<float> k_;   // (2R+1)^2 kernel weights, row-major
    // Wrapped index lookup: xi_[x + dx + R] gives the source column, or -1 outside (Fixed).
    std::vector<int> xi_, yi_;
};

}  // namespace cave
