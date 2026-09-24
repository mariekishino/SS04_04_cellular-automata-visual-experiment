// Stimulus path: amplitude 0 == Phase 1 bit for bit; growth/mu modes change the state;
// uniform vs gradient differ; Gray-Scott ignores it.
#include <cmath>
#include "core/presets.hpp"
#include "core/stimulus.hpp"
#include "check.hpp"
using namespace cave;
static std::unique_ptr<Model> orbium(const Overrides& ov) {
    auto m = make_model("lenia", "orbium", 48, 48, Boundary::Periodic, ov);
    apply_init(*m, "orbium", 0);
    return m;
}
int main() {
    CHECK(stimulus_weight(StimulusShape::Uniform, 0, 0, 64, 64) == 1.0f);
    CHECK(stimulus_weight(StimulusShape::GradientX, 0, 5, 64, 64) == 0.0f);
    CHECK(stimulus_weight(StimulusShape::GradientX, 63, 5, 64, 64) == 1.0f);

    // 1. amplitude 0 (any mode, any gain) == no stimulus, bit for bit
    auto base = orbium({});
    auto g0 = orbium({{"stim_mode", "growth"}, {"stim_gain", "0.5"}});
    auto m0 = orbium({{"stim_mode", "mu"}, {"stim_gain", "0.05"}});
    Stimulus zero; zero.amplitude = 0.0f;
    for (int i = 0; i < 30; ++i) { base->step(); g0->step(zero); m0->step(zero); }
    CHECK(base->channels()[0].grid->raw() == g0->channels()[0].grid->raw());
    CHECK(base->channels()[0].grid->raw() == m0->channels()[0].grid->raw());

    // 2. mode none ignores a non-zero amplitude
    auto none = orbium({{"stim_gain", "0.5"}});
    Stimulus s1; s1.amplitude = 1.0f;
    for (int i = 0; i < 30; ++i) none->step(s1);
    CHECK(base->channels()[0].grid->raw() == none->channels()[0].grid->raw());

    // 3. growth mode with positive uniform stimulus raises mass relative to baseline
    auto g1 = orbium({{"stim_mode", "growth"}, {"stim_gain", "0.2"}});
    for (int i = 0; i < 30; ++i) g1->step(s1);
    double mb = 0, mg = 0;
    for (float v : base->channels()[0].grid->raw()) mb += v;
    for (float v : g1->channels()[0].grid->raw()) { CHECK(std::isfinite(v)); mg += v; }
    CHECK(mg > mb);

    // 4. mu mode changes the state; uniform and gradient differ from each other
    auto mu_u = orbium({{"stim_mode", "mu"}, {"stim_gain", "0.01"}});
    auto mu_g = orbium({{"stim_mode", "mu"}, {"stim_gain", "0.01"}});
    Stimulus sg; sg.amplitude = 1.0f; sg.shape = StimulusShape::GradientX;
    for (int i = 0; i < 30; ++i) { mu_u->step(s1); mu_g->step(sg); }
    CHECK(mu_u->channels()[0].grid->raw() != base->channels()[0].grid->raw());
    CHECK(mu_u->channels()[0].grid->raw() != mu_g->channels()[0].grid->raw());

    // 5. Gray-Scott ignores the stimulus
    auto ga = make_model("grayscott", "coral", 32, 32, Boundary::Periodic, {});
    auto gb = make_model("grayscott", "coral", 32, 32, Boundary::Periodic, {});
    apply_init(*ga, "square", 0); apply_init(*gb, "square", 0);
    for (int i = 0; i < 50; ++i) { ga->step(); gb->step(s1); }
    CHECK(ga->channels()[0].grid->raw() == gb->channels()[0].grid->raw());
    std::puts("test_stimulus OK");
    return 0;
}
