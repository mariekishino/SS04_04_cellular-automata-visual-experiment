#pragma once
// Model: the interface every simulation implements.
//
// A model owns its state (one or more Grids) and its parameters. step()
// advances the state by one fixed time step. Nothing here knows about
// pixels, files, or audio.

#include <cstdint>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#include "core/grid.hpp"
#include "core/stimulus.hpp"

namespace cave {

struct Channel {
    std::string name;   // e.g. "A", "u", "v"
    const Grid* grid;
};

class Model {
public:
    virtual ~Model() = default;

    virtual std::string name() const = 0;

    // Advance by one fixed time step dt() under the given stimulus.
    // step() with no argument is the stimulus-free update (Phase 0/1 behaviour).
    virtual void step(const Stimulus& stim) = 0;
    void step() { step(Stimulus::none()); }

    virtual float dt() const = 0;
    virtual Boundary boundary() const = 0;

    // All state channels, in a stable order. channels()[0] is what we draw.
    virtual std::vector<Channel> channels() const = 0;

    // Human-readable parameters for config.json. Order is preserved.
    virtual std::vector<std::pair<std::string, std::string>> parameters() const = 0;

    // Slow internal states (Phase 3+), e.g. {"adapt_m", m}. Empty if none.
    virtual std::vector<std::pair<std::string, double>> slow_states() const { return {}; }

    // Short description of the update rule and its source, for the record.
    virtual std::string formula() const = 0;

    // Checkpoint: raw state, enough to resume exactly.
    virtual void save_state(std::ostream& os) const = 0;
    virtual void load_state(std::istream& is) = 0;

    std::uint64_t step_count() const { return steps_; }
    double time() const { return static_cast<double>(steps_) * dt(); }

protected:
    std::uint64_t steps_ = 0;
};

// Helpers shared by models for checkpoint I/O.
void write_grid(std::ostream& os, const Grid& g);
void read_grid(std::istream& is, Grid& g);

}  // namespace cave
