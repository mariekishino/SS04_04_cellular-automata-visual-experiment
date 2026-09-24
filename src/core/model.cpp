#include "core/model.hpp"

#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

namespace cave {

void write_grid(std::ostream& os, const Grid& g) {
    const std::int32_t w = g.width(), h = g.height();
    os.write(reinterpret_cast<const char*>(&w), sizeof(w));
    os.write(reinterpret_cast<const char*>(&h), sizeof(h));
    os.write(reinterpret_cast<const char*>(g.raw().data()), static_cast<std::streamsize>(g.size() * sizeof(float)));
}

void read_grid(std::istream& is, Grid& g) {
    std::int32_t w = 0, h = 0;
    is.read(reinterpret_cast<char*>(&w), sizeof(w));
    is.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (!is || w <= 0 || h <= 0) throw std::runtime_error("read_grid: bad header");
    g = Grid(w, h, 0.0f);
    is.read(reinterpret_cast<char*>(g.raw().data()), static_cast<std::streamsize>(g.size() * sizeof(float)));
    if (!is) throw std::runtime_error("read_grid: truncated data");
}

}  // namespace cave
