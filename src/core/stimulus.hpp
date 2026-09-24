#pragma once
// Stimulus: what the environment hands to the model at one step.
//
// Phase 2 keeps it to a scalar amplitude s(t) in [0, 1] plus a spatial shape
// h(x, y). The model decides where it enters (see LeniaParams::stim_mode).
// amplitude 0 must reproduce the stimulus-free update exactly.

namespace cave {

enum class StimulusShape {
    Uniform,    // h = 1 everywhere
    GradientX   // h = x / (W - 1): 0 at the left edge, 1 at the right edge
                // (discontinuous across the periodic seam; recorded as such)
};

struct Stimulus {
    float amplitude = 0.0f;                        // 0..1, from the input source
    StimulusShape shape = StimulusShape::Uniform;

    static Stimulus none() { return Stimulus{}; }
    bool active() const { return amplitude != 0.0f; }
};

const char* stimulus_shape_name(StimulusShape s);
StimulusShape stimulus_shape_from_name(const char* s);

// Spatial weight h(x, y) for a W x H grid.
float stimulus_weight(StimulusShape shape, int x, int y, int W, int H);

}  // namespace cave
