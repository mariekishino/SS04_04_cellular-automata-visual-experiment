#include <cmath>
#include "core/lenia.hpp"
#include "core/presets.hpp"
#include "check.hpp"
using namespace cave;
int main() {
    LeniaParams p;
    Lenia L(64, 64, p);
    // kernel sums to 1, center weight is 0 (ring), symmetric
    double s = 0; for (float w : L.kernel()) s += w;
    CHECK_NEAR(s, 1.0, 1e-4);
    const int K = L.kernel_size();
    CHECK(L.kernel()[static_cast<std::size_t>(p.R) * K + p.R] == 0.0f);
    CHECK(L.kernel()[0] == L.kernel()[static_cast<std::size_t>(K) * K - 1]);
    // growth: +1 at mu, -1 far away
    CHECK_NEAR(Lenia::growth(p.mu, p.mu, p.sigma), 1.0, 1e-6);
    CHECK_NEAR(Lenia::growth(1.0f, p.mu, p.sigma), -1.0, 1e-6);
    // empty grid stays empty (G(0) = -1, clipped at 0)
    L.step();
    for (float v : L.state().raw()) CHECK(v == 0.0f);
    CHECK(L.step_count() == 1);
    // orbium: mass stays in a plausible range after 50 steps and values stay in [0,1]
    auto m = make_model("lenia", "orbium", 64, 64, Boundary::Periodic, {});
    apply_init(*m, "orbium", 0);
    for (int i = 0; i < 50; ++i) m->step();
    double mass = 0; for (float v : m->channels()[0].grid->raw()) { CHECK(std::isfinite(v)); CHECK(v >= 0.0f && v <= 1.0f); mass += v; }
    CHECK(mass > 20.0 && mass < 200.0);
    // fixed vs periodic differ when the pattern touches the edge
    auto a = make_model("lenia", "orbium", 24, 24, Boundary::Periodic, {});
    auto b = make_model("lenia", "orbium", 24, 24, Boundary::Fixed, {});
    apply_init(*a, "orbium", 0); apply_init(*b, "orbium", 0);
    for (int i = 0; i < 5; ++i) { a->step(); b->step(); }
    CHECK(a->channels()[0].grid->raw() != b->channels()[0].grid->raw());
    std::puts("test_lenia OK");
    return 0;
}
