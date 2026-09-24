#include "core/metrics.hpp"

#include <cmath>
#include <limits>
#include <sstream>

namespace cave {

ChannelMetrics compute_metrics(const Grid& cur, const Grid* prev, float threshold) {
    ChannelMetrics m;
    m.min = std::numeric_limits<float>::infinity();
    m.max = -std::numeric_limits<float>::infinity();
    const std::vector<float>& a = cur.raw();
    const std::vector<float>* p = prev ? &prev->raw() : nullptr;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const float v = a[i];
        if (!std::isfinite(v)) { m.finite = false; continue; }
        if (v < m.min) m.min = v;
        if (v > m.max) m.max = v;
        m.sum += v;
        if (v >= threshold) ++m.count_above;
        if (p) m.diff_l1 += std::fabs(static_cast<double>(v) - static_cast<double>((*p)[i]));
    }
    if (!std::isfinite(m.min)) { m.min = 0.0f; m.max = 0.0f; }
    return m;
}

HealthFlags evaluate_health(const std::vector<ChannelMetrics>& series, std::size_t cells, const HealthConfig& cfg) {
    HealthFlags f;
    if (series.empty() || cells == 0) return f;
    for (const ChannelMetrics& m : series) if (!m.finite) f.nan_or_inf = true;
    const ChannelMetrics& last = series.back();
    f.extinct = (last.count_above == 0);
    f.saturated = (static_cast<double>(last.count_above) >= cfg.saturation_fraction * static_cast<double>(cells));
    if (static_cast<int>(series.size()) > cfg.static_window) {
        bool all_static = true;
        for (std::size_t i = series.size() - static_cast<std::size_t>(cfg.static_window); i < series.size(); ++i) {
            if (series[i].diff_l1 / static_cast<double>(cells) >= cfg.static_eps) { all_static = false; break; }
        }
        f.static_ = all_static;
    }
    return f;
}

std::string csv_header(const std::vector<std::string>& names) {
    std::ostringstream os;
    os << "step,t";
    for (const std::string& n : names) {
        os << ',' << n << "_min," << n << "_max," << n << "_sum," << n << "_diff_l1," << n << "_count_above," << n << "_finite";
    }
    return os.str();
}

std::string csv_row(unsigned long long step, double t, const std::vector<ChannelMetrics>& ms) {
    std::ostringstream os;
    os.precision(9);
    os << step << ',' << t;
    for (const ChannelMetrics& m : ms) {
        os << ',' << m.min << ',' << m.max << ',' << m.sum << ',' << m.diff_l1 << ',' << m.count_above << ',' << (m.finite ? 1 : 0);
    }
    return os.str();
}

}  // namespace cave
