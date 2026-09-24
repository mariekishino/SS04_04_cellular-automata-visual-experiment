#include "core/presets.hpp"

#include <cmath>
#include <stdexcept>

#include "core/orbium_cells.hpp"
#include "core/rng.hpp"

namespace cave {

std::vector<PresetInfo> list_presets() {
    return {
        {"lenia", "orbium", "R=13 mu=0.15 sigma=0.015 dt=0.1 (Chan, Orbium unicaudatus)"},
        {"grayscott", "coral", "Du=1 Dv=0.5 F=0.0545 k=0.062 dt=1 (Sims)"},
        {"grayscott", "mitosis", "Du=1 Dv=0.5 F=0.0367 k=0.0649 dt=1 (Sims)"},
    };
}

std::vector<PresetInfo> list_inits() {
    return {
        {"lenia", "orbium", "Orbium 20x20 pattern placed at the grid center"},
        {"lenia", "noise", "uniform random [0,1] inside a centered square of side W/2, 0 elsewhere"},
        {"lenia", "blobs", "6 gaussian blobs (radius ~R/2) at random positions"},
        {"grayscott", "square", "u=1 v=0 everywhere; centered square (side W/10) with u=0.5 v=0.25"},
        {"grayscott", "squares", "u=1 v=0; 8 random small squares (side 4) with u=0.5 v=0.25"},
    };
}

namespace {

float get_f(const Overrides& ov, const std::string& key, float def) {
    auto it = ov.find(key);
    return it == ov.end() ? def : std::stof(it->second);
}
int get_i(const Overrides& ov, const std::string& key, int def) {
    auto it = ov.find(key);
    return it == ov.end() ? def : std::stoi(it->second);
}

}  // namespace

std::unique_ptr<Model> make_model(const std::string& model, const std::string& preset, int width, int height,
                                  Boundary boundary, const Overrides& ov) {
    if (model == "lenia") {
        LeniaParams p;
        if (preset == "orbium") { p.R = 13; p.mu = 0.15f; p.sigma = 0.015f; p.dt = 0.1f; }
        else throw std::runtime_error("unknown lenia preset: " + preset);
        p.R = get_i(ov, "R", p.R);
        p.mu = get_f(ov, "mu", p.mu);
        p.sigma = get_f(ov, "sigma", p.sigma);
        p.dt = get_f(ov, "dt", p.dt);
        p.boundary = boundary;
        if (auto it = ov.find("stim_mode"); it != ov.end()) p.stim_mode = stim_mode_from_name(it->second.c_str());
        p.stim_gain = get_f(ov, "stim_gain", p.stim_gain);
        return std::make_unique<Lenia>(width, height, p);
    }
    if (model == "grayscott") {
        GrayScottParams p;
        if (preset == "coral") { p.F = 0.0545f; p.k = 0.062f; }
        else if (preset == "mitosis") { p.F = 0.0367f; p.k = 0.0649f; }
        else throw std::runtime_error("unknown grayscott preset: " + preset);
        p.Du = get_f(ov, "Du", p.Du);
        p.Dv = get_f(ov, "Dv", p.Dv);
        p.F = get_f(ov, "F", p.F);
        p.k = get_f(ov, "k", p.k);
        p.dt = get_f(ov, "dt", p.dt);
        p.boundary = boundary;
        return std::make_unique<GrayScott>(width, height, p);
    }
    throw std::runtime_error("unknown model: " + model);
}

void apply_init(Model& m, const std::string& init, unsigned seed) {
    Rng rng(seed);
    if (auto* L = dynamic_cast<Lenia*>(&m)) {
        Grid& a = L->state();
        const int W = a.width(), H = a.height();
        a.fill(0.0f);
        if (init == "orbium") {
            if (W < ORBIUM_W || H < ORBIUM_H) throw std::runtime_error("grid too small for orbium");
            const int ox = (W - ORBIUM_W) / 2, oy = (H - ORBIUM_H) / 2;
            for (int y = 0; y < ORBIUM_H; ++y)
                for (int x = 0; x < ORBIUM_W; ++x)
                    a.at(ox + x, oy + y) = ORBIUM_CELLS[y * ORBIUM_W + x];
        } else if (init == "noise") {
            const int s = W / 2, ox = (W - s) / 2, oy = (H - s) / 2;
            for (int y = 0; y < s; ++y)
                for (int x = 0; x < s; ++x)
                    a.at(ox + x, oy + y) = rng.uniform01();
        } else if (init == "blobs") {
            const float rad = static_cast<float>(L->params().R) * 0.5f;
            for (int n = 0; n < 6; ++n) {
                const int cx = rng.uniform_int(0, W - 1), cy = rng.uniform_int(0, H - 1);
                const float amp = 0.5f + 0.5f * rng.uniform01();
                for (int y = 0; y < H; ++y)
                    for (int x = 0; x < W; ++x) {
                        const float dx = static_cast<float>(x - cx), dy = static_cast<float>(y - cy);
                        const float v = amp * std::exp(-(dx * dx + dy * dy) / (2.0f * rad * rad));
                        a.at(x, y) = std::min(1.0f, a.at(x, y) + v);
                    }
            }
        } else {
            throw std::runtime_error("unknown lenia init: " + init);
        }
        return;
    }
    if (auto* G = dynamic_cast<GrayScott*>(&m)) {
        Grid& u = G->u();
        Grid& v = G->v();
        const int W = u.width(), H = u.height();
        u.fill(1.0f);
        v.fill(0.0f);
        auto seed_square = [&](int cx, int cy, int side) {
            for (int y = cy - side / 2; y < cy - side / 2 + side; ++y)
                for (int x = cx - side / 2; x < cx - side / 2 + side; ++x) {
                    if (x < 0 || y < 0 || x >= W || y >= H) continue;
                    u.at(x, y) = 0.5f;
                    v.at(x, y) = 0.25f;
                }
        };
        if (init == "square") {
            seed_square(W / 2, H / 2, std::max(6, W / 10));
        } else if (init == "squares") {
            for (int n = 0; n < 8; ++n) seed_square(rng.uniform_int(0, W - 1), rng.uniform_int(0, H - 1), 4);
        } else {
            throw std::runtime_error("unknown grayscott init: " + init);
        }
        return;
    }
    throw std::runtime_error("apply_init: unsupported model");
}

}  // namespace cave
