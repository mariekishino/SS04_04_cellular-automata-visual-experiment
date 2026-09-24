#include <cmath>
#include "core/gray_scott.hpp"
#include "core/presets.hpp"
#include "check.hpp"
using namespace cave;
int main() {
    Grid c(8, 8, 0.7f);
    CHECK_NEAR(GrayScott::laplacian(c, 3, 3, Boundary::Periodic, 0.0f), 0.0, 1e-6);
    CHECK_NEAR(GrayScott::laplacian(c, 0, 0, Boundary::Periodic, 0.0f), 0.0, 1e-6);
    CHECK_NEAR(GrayScott::laplacian(c, 0, 0, Boundary::Fixed, 0.7f), 0.0, 1e-6);  // outside = same value
    CHECK(GrayScott::laplacian(c, 0, 0, Boundary::Fixed, 0.0f) < 0.0f);            // outside = 0 drains
    // rest state (u=1, v=0) is a fixed point
    GrayScottParams p;
    GrayScott G(16, 16, p);
    for (int i = 0; i < 10; ++i) G.step();
    for (float v : G.u().raw()) CHECK_NEAR(v, 1.0, 1e-6);
    for (float v : G.v().raw()) CHECK(v == 0.0f);
    // coral from a square seed stays finite and bounded for 300 steps
    auto m = make_model("grayscott", "coral", 64, 64, Boundary::Periodic, {});
    apply_init(*m, "square", 0);
    for (int i = 0; i < 300; ++i) m->step();
    for (const Channel& ch : m->channels())
        for (float v : ch.grid->raw()) { CHECK(std::isfinite(v)); CHECK(v >= -0.01f && v <= 1.01f); }
    std::puts("test_gray_scott OK");
    return 0;
}
