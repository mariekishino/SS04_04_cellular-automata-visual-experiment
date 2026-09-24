// Phase 3: adaptation slow state, cos shape, composite source.
#include <sstream>
#include <cmath>
#include "core/input.hpp"
#include "core/lenia.hpp"
#include "core/presets.hpp"
#include "check.hpp"
using namespace cave;
static std::unique_ptr<Model> orbium(const Overrides& ov) {
    auto m = make_model("lenia", "orbium", 48, 48, Boundary::Periodic, ov);
    apply_init(*m, "orbium", 0);
    return m;
}
static double mass(const Model& m) { double s = 0; for (float v : m.channels()[0].grid->raw()) s += v; return s; }
int main() {
    // cos shape: 0 at the seam, 1 at the center, continuous across the seam
    CHECK_NEAR(stimulus_weight(StimulusShape::CosX, 0, 0, 64, 64), 0.0, 1e-6);
    CHECK_NEAR(stimulus_weight(StimulusShape::CosX, 32, 0, 64, 64), 1.0, 1e-6);
    CHECK(std::fabs(stimulus_weight(StimulusShape::CosX, 63, 0, 64, 64) - stimulus_weight(StimulusShape::CosX, 0, 0, 64, 64)) < 0.01);

    Stimulus one; one.amplitude = 1.0f;
    // 1. adapt_k = 0 => identical to Phase 2 under stimulus
    auto a = orbium({{"stim_mode", "growth"}, {"stim_gain", "0.2"}});
    auto b = orbium({{"stim_mode", "growth"}, {"stim_gain", "0.2"}, {"adapt_k", "0"}, {"adapt_tau_steps", "100"}});
    for (int i = 0; i < 40; ++i) { a->step(one); b->step(one); }
    CHECK(a->channels()[0].grid->raw() == b->channels()[0].grid->raw());
    // 2. adapt on but no stimulus => identical to no adaptation (m stays 0)
    auto c = orbium({{"stim_mode", "growth"}, {"stim_gain", "0.2"}});
    auto d = orbium({{"stim_mode", "growth"}, {"stim_gain", "0.2"}, {"adapt_k", "3"}, {"adapt_tau_steps", "100"}});
    for (int i = 0; i < 40; ++i) { c->step(); d->step(); }
    CHECK(c->channels()[0].grid->raw() == d->channels()[0].grid->raw());
    CHECK(dynamic_cast<Lenia&>(*d).adaptation() == 0.0f);
    // 3. m follows the analytic EMA under a constant stimulus
    auto e = orbium({{"stim_mode", "growth"}, {"stim_gain", "0.0"}, {"adapt_k", "1"}, {"adapt_tau_steps", "50"}});
    for (int i = 0; i < 100; ++i) e->step(one);
    CHECK_NEAR(dynamic_cast<Lenia&>(*e).adaptation(), 1.0 - std::exp(-100.0 / 50.0), 1e-4);
    CHECK(e->slow_states().size() == 1 && e->slow_states()[0].first == "adapt_m");
    // 4. history reduces the probe response: 300 steps of stimulus vs 300 steps of silence, then a 30-step probe
    auto h1 = orbium({{"stim_mode", "growth"}, {"stim_gain", "0.2"}, {"adapt_k", "3"}, {"adapt_tau_steps", "300"}});
    auto h0 = orbium({{"stim_mode", "growth"}, {"stim_gain", "0.2"}, {"adapt_k", "3"}, {"adapt_tau_steps", "300"}});
    for (int i = 0; i < 300; ++i) { h1->step(one); h0->step(); }
    for (int i = 0; i < 100; ++i) { h1->step(); h0->step(); }   // short common silence (m decays a little)
    const double m1 = mass(*h1), m0 = mass(*h0);
    for (int i = 0; i < 30; ++i) { h1->step(one); h0->step(one); }
    const double r1 = mass(*h1) - m1, r0 = mass(*h0) - m0;
    CHECK(r0 > 5.0);          // the naive probe response is visible
    CHECK(r1 < 0.7 * r0);     // and clearly smaller after a loud history
    // 5. checkpoint round trip carries m
    std::stringstream ss; h1->save_state(ss);
    auto h2 = orbium({{"stim_mode", "growth"}, {"stim_gain", "0.2"}, {"adapt_k", "3"}, {"adapt_tau_steps", "300"}});
    h2->load_state(ss);
    CHECK(dynamic_cast<Lenia&>(*h2).adaptation() == dynamic_cast<Lenia&>(*h1).adaptation());
    // 6. composite = max
    PulseParams p; p.start = 1; p.duration = 1; p.count = 1; p.amplitude = 0.4f;
    PulseParams q; q.start = 1.5; q.duration = 1; q.count = 1; q.amplitude = 0.9f;
    CompositeSource cs; cs.add(std::make_unique<PulseSource>(p)); cs.add(std::make_unique<PulseSource>(q));
    CHECK(cs.amplitude(1.2) == 0.4f); CHECK(cs.amplitude(1.7) == 0.9f); CHECK(cs.amplitude(3.0) == 0.0f);
    CHECK_NEAR(cs.duration_seconds(), 2.5, 1e-12);
    EnvelopeSource es({1, 1, 1, 1}, {1, 1, 1, 1}, 2.0, "e"); es.truncate(1.0);
    CHECK(es.amplitude(0.4) == 1.0f && es.amplitude(1.2) == 0.0f);
    std::puts("test_adapt OK");
    return 0;
}
