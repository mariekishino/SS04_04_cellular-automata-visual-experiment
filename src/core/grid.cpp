#include "core/grid.hpp"

#include <utility>

namespace cave {

const char* boundary_name(Boundary b) {
    switch (b) {
        case Boundary::Periodic: return "periodic";
        case Boundary::Fixed: return "fixed";
    }
    return "unknown";
}

Grid::Grid(int width, int height, float fill)
    : w_(width), h_(height), data_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), fill) {}

float Grid::read(int x, int y, Boundary b, float outside) const {
    if (b == Boundary::Periodic) {
        return data_[index(wrap(x, w_), wrap(y, h_))];
    }
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return outside;
    return data_[index(x, y)];
}

void Grid::fill(float v) {
    for (float& f : data_) f = v;
}

void Grid::swap(Grid& other) noexcept {
    std::swap(w_, other.w_);
    std::swap(h_, other.h_);
    data_.swap(other.data_);  // O(1): only pointers move
}

}  // namespace cave
