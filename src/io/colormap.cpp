#include "io/colormap.hpp"

#include <algorithm>

namespace cave {

Colormap colormap_from_name(const std::string& s) {
    if (s == "gray") return Colormap::Gray;
    return Colormap::Viridis;
}

const char* colormap_name(Colormap c) {
    return c == Colormap::Gray ? "gray" : "viridis";
}

void map_color(float v, Colormap cm, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b) {
    v = std::min(1.0f, std::max(0.0f, v));
    if (cm == Colormap::Gray) {
        r = g = b = static_cast<std::uint8_t>(v * 255.0f + 0.5f);
        return;
    }
    // 5 stops sampled from matplotlib viridis.
    static const float stops[5][3] = {
        {68, 1, 84}, {59, 82, 139}, {33, 145, 140}, {94, 201, 98}, {253, 231, 37}};
    const float p = v * 4.0f;
    const int i = std::min(3, static_cast<int>(p));
    const float t = p - static_cast<float>(i);
    r = static_cast<std::uint8_t>(stops[i][0] + (stops[i + 1][0] - stops[i][0]) * t + 0.5f);
    g = static_cast<std::uint8_t>(stops[i][1] + (stops[i + 1][1] - stops[i][1]) * t + 0.5f);
    b = static_cast<std::uint8_t>(stops[i][2] + (stops[i + 1][2] - stops[i][2]) * t + 0.5f);
}

std::vector<std::uint8_t> render_rgb(const Grid& g, int scale, Colormap cm, float vmin, float vmax) {
    const int W = g.width() * scale, H = g.height() * scale;
    std::vector<std::uint8_t> rgb(static_cast<std::size_t>(W) * H * 3);
    const float range = (vmax > vmin) ? (vmax - vmin) : 1.0f;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const float v = (g.at(x / scale, y / scale) - vmin) / range;
            std::uint8_t* px = rgb.data() + (static_cast<std::size_t>(y) * W + x) * 3;
            map_color(v, cm, px[0], px[1], px[2]);
        }
    }
    return rgb;
}

}  // namespace cave
