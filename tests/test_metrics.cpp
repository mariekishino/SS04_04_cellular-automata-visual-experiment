#include <cmath>
#include <limits>
#include "core/metrics.hpp"
#include "check.hpp"
using namespace cave;
int main() {
    Grid g(4, 1, 0.0f);
    g.at(0, 0) = 0.2f; g.at(1, 0) = 0.5f; g.at(2, 0) = 0.05f;
    ChannelMetrics m = compute_metrics(g, nullptr, 0.1f);
    CHECK(m.min == 0.0f); CHECK(m.max == 0.5f);
    CHECK_NEAR(m.sum, 0.75, 1e-6);
    CHECK(m.count_above == 2);
    CHECK(m.diff_l1 == 0.0);
    CHECK(m.finite);
    Grid h = g; h.at(1, 0) = 0.0f;
    ChannelMetrics m2 = compute_metrics(h, &g, 0.1f);
    CHECK_NEAR(m2.diff_l1, 0.5, 1e-6);
    h.at(3, 0) = std::numeric_limits<float>::quiet_NaN();
    CHECK(!compute_metrics(h, nullptr, 0.1f).finite);

    HealthConfig hc; hc.static_window = 2;
    std::vector<ChannelMetrics> s;
    ChannelMetrics z; z.count_above = 0; z.diff_l1 = 0.0;
    for (int i = 0; i < 4; ++i) s.push_back(z);
    HealthFlags f = evaluate_health(s, 4, hc);
    CHECK(f.extinct); CHECK(f.static_); CHECK(!f.saturated); CHECK(!f.nan_or_inf);
    ChannelMetrics full; full.count_above = 4; full.diff_l1 = 1.0;
    s.push_back(full);
    f = evaluate_health(s, 4, hc);
    CHECK(f.saturated); CHECK(!f.extinct); CHECK(!f.static_);
    std::puts("test_metrics OK");
    return 0;
}
