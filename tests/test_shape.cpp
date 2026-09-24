#include <cmath>
#include "core/metrics.hpp"
#include "check.hpp"
using namespace cave;
int main() {
    // single bright cell: centroid at its center, spread 0, one component
    Grid g(16, 8, 0.0f);
    g.at(5, 2) = 1.0f;
    ShapeMetrics s = compute_shape(g, Boundary::Periodic, 0.5f);
    CHECK_NEAR(s.cx, 5.5, 1e-6); CHECK_NEAR(s.cy, 2.5, 1e-6);
    CHECK_NEAR(s.spread, 0.0, 1e-6); CHECK(s.components == 1); CHECK_NEAR(s.mass, 1.0, 1e-9);

    // blob straddling the x seam: cells x=15 and x=0 -> centroid at the seam (x=0 or 16), not x=8
    Grid h(16, 8, 0.0f);
    h.at(15, 3) = 1.0f; h.at(0, 3) = 1.0f;
    ShapeMetrics t = compute_shape(h, Boundary::Periodic, 0.5f);
    const double dseam = std::min(t.cx, 16.0 - t.cx);
    CHECK(dseam < 1e-6);
    CHECK_NEAR(t.spread, 0.5, 1e-6);      // each cell 0.5 from the seam
    CHECK(t.components == 1);             // connected across the seam
    ShapeMetrics tf = compute_shape(h, Boundary::Fixed, 0.5f);
    CHECK_NEAR(tf.cx, 8.0, 1e-6);         // plain mean without wrap
    CHECK(tf.components == 2);            // not connected without wrap

    // two separate blobs -> 2 components; sub-threshold cells do not connect them
    Grid k(16, 8, 0.0f);
    k.at(2, 2) = 1.0f; k.at(3, 2) = 1.0f; k.at(3, 3) = 0.2f; k.at(10, 6) = 0.9f;
    CHECK(compute_shape(k, Boundary::Periodic, 0.5f).components == 2);
    CHECK(compute_shape(k, Boundary::Periodic, 0.1f).components == 2);

    // displacement across the seam is short, not long
    double dx, dy;
    displacement(15.5, 1.0, 0.5, 1.0, 16, 8, Boundary::Periodic, dx, dy);
    CHECK_NEAR(dx, 1.0, 1e-9); CHECK_NEAR(dy, 0.0, 1e-9);
    displacement(15.5, 1.0, 0.5, 1.0, 16, 8, Boundary::Fixed, dx, dy);
    CHECK_NEAR(dx, -15.0, 1e-9);

    // empty grid: mass 0, no components, no NaN
    ShapeMetrics e = compute_shape(Grid(4, 4, 0.0f), Boundary::Periodic, 0.5f);
    CHECK(e.mass == 0.0 && e.components == 0 && std::isfinite(e.cx));
    std::puts("test_shape OK");
    return 0;
}
