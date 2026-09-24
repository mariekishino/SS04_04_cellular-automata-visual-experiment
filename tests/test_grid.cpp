#include "core/grid.hpp"
#include "check.hpp"
using namespace cave;
int main() {
    CHECK(Grid::wrap(-1, 8) == 7);
    CHECK(Grid::wrap(8, 8) == 0);
    CHECK(Grid::wrap(-9, 8) == 7);
    CHECK(Grid::wrap(3, 8) == 3);

    Grid g(4, 3, 0.0f);
    CHECK(g.size() == 12);
    g.at(3, 2) = 5.0f;
    CHECK(g.raw()[2 * 4 + 3] == 5.0f);  // index = y * width + x
    CHECK(g.read(-1, -1, Boundary::Periodic) == 5.0f);
    CHECK(g.read(-1, -1, Boundary::Fixed) == 0.0f);
    CHECK(g.read(-1, -1, Boundary::Fixed, 9.0f) == 9.0f);
    CHECK(g.read(3, 2, Boundary::Fixed) == 5.0f);

    Grid h(4, 3, 1.0f);
    g.swap(h);
    CHECK(g.at(3, 2) == 1.0f);
    CHECK(h.at(3, 2) == 5.0f);
    std::puts("test_grid OK");
    return 0;
}
