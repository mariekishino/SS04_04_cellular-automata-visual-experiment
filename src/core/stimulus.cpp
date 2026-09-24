#include "core/stimulus.hpp"

#include <cstring>

namespace cave {

const char* stimulus_shape_name(StimulusShape s) {
    return s == StimulusShape::GradientX ? "gradient_x" : "uniform";
}

StimulusShape stimulus_shape_from_name(const char* s) {
    if (std::strcmp(s, "gradient_x") == 0 || std::strcmp(s, "gradient") == 0) return StimulusShape::GradientX;
    return StimulusShape::Uniform;
}

float stimulus_weight(StimulusShape shape, int x, int /*y*/, int W, int /*H*/) {
    switch (shape) {
        case StimulusShape::Uniform: return 1.0f;
        case StimulusShape::GradientX: return W > 1 ? static_cast<float>(x) / static_cast<float>(W - 1) : 1.0f;
    }
    return 1.0f;
}

}  // namespace cave
