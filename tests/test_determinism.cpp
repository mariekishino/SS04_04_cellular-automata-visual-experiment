// Same config twice => identical bytes. Save/load then continue => identical to uninterrupted run.
#include <sstream>
#include "core/presets.hpp"
#include "check.hpp"
using namespace cave;
static void run_case(const char* model, const char* preset, const char* init) {
    auto a = make_model(model, preset, 48, 40, Boundary::Periodic, {});
    auto b = make_model(model, preset, 48, 40, Boundary::Periodic, {});
    apply_init(*a, init, 7); apply_init(*b, init, 7);
    for (int i = 0; i < 30; ++i) { a->step(); b->step(); }
    for (std::size_t c = 0; c < a->channels().size(); ++c)
        CHECK(a->channels()[c].grid->raw() == b->channels()[c].grid->raw());
    // checkpoint round trip
    std::stringstream ss;
    a->save_state(ss);
    auto c = make_model(model, preset, 48, 40, Boundary::Periodic, {});
    c->load_state(ss);
    CHECK(c->step_count() == 30);
    for (int i = 0; i < 20; ++i) { a->step(); c->step(); }
    for (std::size_t k = 0; k < a->channels().size(); ++k)
        CHECK(a->channels()[k].grid->raw() == c->channels()[k].grid->raw());
    // a different seed for a random init changes the result
    auto d = make_model(model, preset, 48, 40, Boundary::Periodic, {});
    apply_init(*d, init, 8);
    if (std::string(init) != "orbium" && std::string(init) != "square")
        CHECK(d->channels()[0].grid->raw() != b->channels()[0].grid->raw());
}
int main() {
    run_case("lenia", "orbium", "orbium");
    run_case("lenia", "orbium", "noise");
    run_case("grayscott", "coral", "square");
    run_case("grayscott", "mitosis", "squares");
    std::puts("test_determinism OK");
    return 0;
}
