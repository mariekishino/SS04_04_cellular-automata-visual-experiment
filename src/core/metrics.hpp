#pragma once
// Metrics: the minimum numbers we record per snapshot (docs/02).
//   min, max, sum, l1 change from the previous snapshot, count above threshold,
//   and whether every value is finite.
#include <cstddef>
#include <string>
#include <vector>

#include "core/grid.hpp"

namespace cave {

struct ChannelMetrics {
    float min = 0.0f;
    float max = 0.0f;
    double sum = 0.0;
    double diff_l1 = 0.0;        // sum |cur - prev|; 0 on the first snapshot
    std::size_t count_above = 0; // cells with value >= threshold
    bool finite = true;          // false if any NaN or Inf
};

ChannelMetrics compute_metrics(const Grid& cur, const Grid* prev, float threshold);

// Flags derived from a series of snapshots. Thresholds are explicit.
struct HealthFlags {
    bool nan_or_inf = false;
    bool extinct = false;     // count_above == 0 at the last snapshot
    bool saturated = false;   // count_above >= saturation_fraction * cells
    bool static_ = false;     // diff_l1 / cells < static_eps for the last N snapshots
};

struct HealthConfig {
    double saturation_fraction = 0.95;
    double static_eps = 1e-6;
    int static_window = 5;
};

HealthFlags evaluate_health(const std::vector<ChannelMetrics>& series, std::size_t cells, const HealthConfig& cfg);

std::string csv_header(const std::vector<std::string>& channel_names);
std::string csv_row(unsigned long long step, double t, const std::vector<ChannelMetrics>& m);

}  // namespace cave

namespace cave {

// Shape metrics for "is it one blob, and is it moving?" (Phase 1).
// Centroid is mass-weighted. With Boundary::Periodic it uses the circular
// mean per axis, so a blob straddling the seam still gets a centroid on the
// blob, not in the middle of the grid.
struct ShapeMetrics {
    double mass = 0.0;
    double cx = 0.0, cy = 0.0;     // centroid in cell units, [0,W) x [0,H)
    double spread = 0.0;           // RMS distance of mass from centroid (minimal image)
    std::size_t components = 0;    // 4-connected components of cells >= threshold
};

ShapeMetrics compute_shape(const Grid& g, Boundary b, float threshold);

// Mass and centroid only (no spread, no components). Cheap enough to call every step.
ShapeMetrics compute_centroid(const Grid& g, Boundary b);

// Minimal-image displacement from (ax, ay) to (bx, by) on a W x H grid.
void displacement(double ax, double ay, double bx, double by, int W, int H, Boundary b, double& dx, double& dy);

}  // namespace cave
