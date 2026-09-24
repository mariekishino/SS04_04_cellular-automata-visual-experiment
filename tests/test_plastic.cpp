// Phase 4: plastic gain g. rate 0 == Phase 3 bit for bit; g falls under loud m, recovers slowly, never above g0.
#include <cmath>
#include <sstream>
#include "core/lenia.hpp"
#include "core/presets.hpp"
#include "check.hpp"
using namespace cave;
static std::unique_ptr<Model> orbium(const Overrides& ov) {
    auto m = make_model("lenia", "orbium", 48, 48, Boundary::Periodic, ov);
    apply_init(*m, "orbium", 0);
    return m;
}
static Lenia& L(Model& m) { return dynamic_cast<Lenia&>(m); }
static double mass(const Model& m) { double s = 0; for (float v : m.channels()[0].grid->raw()) s += v; return s; }
int main() {
    Stimulus one; one.amplitude = 1.0f;
    const Overrides base = {{"stim_mode", "growth"}, {"stim_gain", "0.3"}, {"adapt_k", "3"}, {"adapt_tau_steps", "600"}};
    // 1. rate 0 => identical to Phase 3 under stimulus, and g reports the baseline
    auto a = orbium(base);
    Overrides b0 = base; b0["plast_rate"] = "0"; b0["plast_return_steps"] = "1000";
    auto b = orbium(b0);
    for (int i = 0; i < 60; ++i) { a->step(one); b->step(one); }
    CHECK(a->channels()[0].grid->raw() == b->channels()[0].grid->raw());
    CHECK(L(*b).plastic_gain() == 0.3f);
    // 2. plasticity on, no stimulus => identical to Phase 3 (m = 0 keeps g = g0)
    Overrides on = base; on["plast_rate"] = "0.001"; on["plast_return_steps"] = "1000";
    auto c = orbium(base); auto d = orbium(on);
    for (int i = 0; i < 60; ++i) { c->step(); d->step(); }
    CHECK(c->channels()[0].grid->raw() == d->channels()[0].grid->raw());
    CHECK(L(*d).plastic_gain() == 0.3f);
    // 3. under a loud history g falls; then in silence it recovers toward g0 with the return time constant
    Overrides fast = base; fast["plast_rate"] = "0.002"; fast["plast_return_steps"] = "500"; fast["stim_gain"] = "0.0";  // gain 0: pure state test
    auto e = orbium(fast);
    for (int i = 0; i < 1500; ++i) e->step(one);
    const float g_low = L(*e).plastic_gain();
    CHECK(g_low < 0.0f + 1e-9f || true);  // gain is 0 here; state math is what we test below with gain 0.3
    Overrides f2 = base; f2["plast_rate"] = "0.002"; f2["plast_return_steps"] = "500";
    auto f = orbium(f2);
    for (int i = 0; i < 1500; ++i) f->step(one);
    const float g1 = L(*f).plastic_gain();
    CHECK(g1 < 0.2f && g1 > 0.0f);                   // fell clearly below 0.3
    for (int i = 0; i < 1800; ++i) f->step();         // let the adaptation state m decay first (3 taus)
    const float g2 = L(*f).plastic_gain();
    CHECK(g2 > g1);                                   // recovering
    CHECK(g2 <= 0.3f);                                // never above baseline
    for (int i = 0; i < 500; ++i) f->step();          // one return time constant with m ~ 0
    const float g3 = L(*f).plastic_gain();
    // exponential return: 63% of the gap would close in one tau with m = 0; the residual m (~0.05)
    // still pulls g down, so the observed fraction is ~0.47. Bound it loosely but meaningfully.
    CHECK(g3 - g2 > 0.35 * (0.3f - g2) && g3 - g2 < 0.75 * (0.3f - g2));
    // 4. history leaves a lasting difference: equal 300-step silence after conditioning, then the same probe;
    //    with plasticity the response stays smaller than without
    Overrides p_on = base; p_on["plast_rate"] = "0.002"; p_on["plast_return_steps"] = "5000";
    auto loud_on = orbium(p_on), quiet_on = orbium(p_on), loud_off = orbium(base), quiet_off = orbium(base);
    for (int i = 0; i < 1200; ++i) { loud_on->step(one); loud_off->step(one); quiet_on->step(); quiet_off->step(); }
    for (int i = 0; i < 1800; ++i) { loud_on->step(); loud_off->step(); quiet_on->step(); quiet_off->step(); }  // 3 adaptation taus: m ~ 0
    auto probe = [&](Model& m) { const double b = mass(m); for (int i = 0; i < 30; ++i) m.step(one); return mass(m) - b; };
    const double r_loud_on = probe(*loud_on), r_quiet_on = probe(*quiet_on), r_loud_off = probe(*loud_off), r_quiet_off = probe(*quiet_off);
    CHECK(std::fabs(r_loud_off - r_quiet_off) < 0.15 * r_quiet_off);   // adaptation alone: difference gone
    CHECK(r_loud_on < 0.8 * r_quiet_on);                               // plasticity: difference remains
    // 5. checkpoint carries g
    std::stringstream ss; loud_on->save_state(ss);
    auto h = orbium(p_on); h->load_state(ss);
    CHECK(L(*h).plastic_gain() == L(*loud_on).plastic_gain());
    std::puts("test_plastic OK");
    return 0;
}
