#pragma once
// Grid: a 2D field of float stored in one contiguous std::vector.
//
// Index rule (same as the 42 Life exercise): data[y * width + x].
// The Grid owns its memory. Models hold two Grids (current / next) and
// swap them after every step so that reads never touch the buffer
// being written.

#include <cstddef>
#include <vector>

namespace cave {

enum class Boundary {
    Periodic,  // torus: x = -1 reads x = width - 1
    Fixed      // outside the grid reads a constant (per channel, chosen by the model)
};

const char* boundary_name(Boundary b);

class Grid {
public:
    Grid() = default;
    Grid(int width, int height, float fill = 0.0f);

    int width() const { return w_; }
    int height() const { return h_; }
    std::size_t size() const { return data_.size(); }

    // In-range access. No boundary handling.
    float& at(int x, int y) { return data_[index(x, y)]; }
    float at(int x, int y) const { return data_[index(x, y)]; }

    // Out-of-range access with boundary handling. Used by neighborhood sums.
    float read(int x, int y, Boundary b, float outside = 0.0f) const;

    std::vector<float>& raw() { return data_; }
    const std::vector<float>& raw() const { return data_; }

    void fill(float v);
    void swap(Grid& other) noexcept;

    // Wrap i into [0, n). Works for negative i.
    static int wrap(int i, int n) {
        i %= n;
        return i < 0 ? i + n : i;
    }

private:
    std::size_t index(int x, int y) const {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(w_) + static_cast<std::size_t>(x);
    }
    int w_ = 0;
    int h_ = 0;
    std::vector<float> data_;
};

}  // namespace cave
