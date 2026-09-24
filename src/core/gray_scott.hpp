#pragma once
// Gray-Scott reaction-diffusion, two channels u (substrate) and v (activator).
//
// Source of the equations: J. E. Pearson, "Complex Patterns in a Simple
// System", Science 261 (1993) 189-192.
// Discretization and constants follow Karl Sims' tutorial
// (https://www.karlsims.com/rd.html): 9-point Laplacian with weights
// center -1, edge 0.2, corner 0.05; Du = 1.0, Dv = 0.5, dt = 1.0.
//
//   u' = u + dt * (Du * lap(u) - u v^2 + F (1 - u))
//   v' = v + dt * (Dv * lap(v) + u v^2 - (F + k) v)
//
// No clamping: if the explicit scheme blows up, the metrics must show it.
// Fixed boundary: outside reads the rest state (u, v) = (1, 0).

#include <string>
#include <vector>

#include "core/model.hpp"

namespace cave {

struct GrayScottParams {
    float Du = 1.0f;
    float Dv = 0.5f;
    float F = 0.055f;   // feed rate
    float k = 0.062f;   // kill rate
    float dt = 1.0f;
    Boundary boundary = Boundary::Periodic;
};

class GrayScott : public Model {
public:
    GrayScott(int width, int height, const GrayScottParams& p);

    std::string name() const override { return "grayscott"; }
    void step(const Stimulus& stim) override;  // stimulus is ignored (recorded in parameters())
    using Model::step;
    float dt() const override { return p_.dt; }
    Boundary boundary() const override { return p_.boundary; }
    std::vector<Channel> channels() const override;
    std::vector<std::pair<std::string, std::string>> parameters() const override;
    std::string formula() const override;
    void save_state(std::ostream& os) const override;
    void load_state(std::istream& is) override;

    Grid& u() { return u_; }
    Grid& v() { return v_; }
    const Grid& u() const { return u_; }
    const Grid& v() const { return v_; }
    const GrayScottParams& params() const { return p_; }

    // 9-point Laplacian at (x, y) with the given outside value for Fixed boundary.
    static float laplacian(const Grid& g, int x, int y, Boundary b, float outside);

private:
    GrayScottParams p_;
    Grid u_, v_;          // current
    Grid un_, vn_;        // next
};

}  // namespace cave
