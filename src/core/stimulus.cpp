#include "core/stimulus.hpp"

#include <cmath>
#include <cstring>

namespace cave {

const char* stimulus_shape_name(StimulusShape s) {
    switch (s) {
        case StimulusShape::Uniform: return "uniform";
        case StimulusShape::GradientX: return "gradient_x";
        case StimulusShape::CosX: return "cos_x";
    }
    return "uniform";
}

StimulusShape stimulus_shape_from_name(const char* s) {
    if (std::strcmp(s, "gradient_x") == 0 || std::strcmp(s, "gradient") == 0) return StimulusShape::GradientX;
    if (std::strcmp(s, "cos_x") == 0 || std::strcmp(s, "cos") == 0) return StimulusShape::CosX;
    return StimulusShape::Uniform;
}

float stimulus_weight(StimulusShape shape, int x, int /*y*/, int W, int /*H*/) {
    switch (shape) {
        case StimulusShape::Uniform: return 1.0f;
        case StimulusShape::GradientX: return W > 1 ? static_cast<float>(x) / static_cast<float>(W - 1) : 1.0f;
        case StimulusShape::CosX: return 0.5f * (1.0f - std::cos(2.0f * 3.14159265358979f * static_cast<float>(x) / static_cast<float>(W)));
    }
    return 1.0f;
}

}  // namespace cave
