#pragma once
// Colormap and grid-to-pixels. Rendering is separate from the model: it
// reads a Grid and produces RGB bytes. Color changes are not state changes.
#include <cstdint>
#include <string>
#include <vector>

#include "core/grid.hpp"

namespace cave {

enum class Colormap { Gray, Viridis };

Colormap colormap_from_name(const std::string& s);
const char* colormap_name(Colormap c);

// v in [0,1] -> RGB. Out-of-range values are clamped.
void map_color(float v, Colormap cm, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b);

// Nearest-neighbor upscale by `scale`. Values are normalized with (v - vmin) / (vmax - vmin).
std::vector<std::uint8_t> render_rgb(const Grid& g, int scale, Colormap cm, float vmin, float vmax);

}  // namespace cave
