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
