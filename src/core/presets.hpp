#pragma once
// Presets (parameter sets) and initial conditions for Phase 0.
// Everything here is explicit so that a run can be reproduced from
// (model, preset, init, seed, size, boundary).
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/gray_scott.hpp"
#include "core/lenia.hpp"
#include "core/model.hpp"

namespace cave {

struct PresetInfo {
    std::string model;   // "lenia" | "grayscott"
    std::string name;
    std::string description;
};

std::vector<PresetInfo> list_presets();
std::vector<PresetInfo> list_inits();

// key=value overrides: for lenia R,mu,sigma,dt,stim_mode,stim_gain,adapt_k,adapt_tau_steps,plast_rate,plast_return_steps ; for grayscott Du,Dv,F,k,dt.
using Overrides = std::map<std::string, std::string>;

std::unique_ptr<Model> make_model(const std::string& model, const std::string& preset, int width, int height,
                                  Boundary boundary, const Overrides& ov);

// Apply a named initial condition. seed is used only by random inits.
void apply_init(Model& m, const std::string& init, unsigned seed);

}  // namespace cave
