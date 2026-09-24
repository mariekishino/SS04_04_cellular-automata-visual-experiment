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
// Cost per step: W * H * (2R+1)^2 multiply-adds.

#include <string>
#include <vector>

#include "core/model.hpp"

namespace cave {

struct LeniaParams {
    int R = 13;             // kernel radius in cells
    float mu = 0.15f;       // growth center
    float sigma = 0.015f;   // growth width
    float dt = 0.1f;        // time step = 1 / T
    Boundary boundary = Boundary::Periodic;
};

class Lenia : public Model {
public:
    Lenia(int width, int height, const LeniaParams& p);

    std::string name() const override { return "lenia"; }
    void step() override;
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

    static float kernel_core(float r);
    static float growth(float u, float mu, float sigma);

private:
    void build_kernel();
    void build_index_tables();

    LeniaParams p_;
    Grid a_;        // current state A
    Grid next_;     // written during step(), then swapped
    std::vector<float> k_;   // (2R+1)^2 kernel weights, row-major
    // Wrapped index lookup: xi_[x + dx + R] gives the source column, or -1 outside (Fixed).
    std::vector<int> xi_, yi_;
};

}  // namespace cave
