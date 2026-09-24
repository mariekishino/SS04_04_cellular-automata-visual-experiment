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

#include <queue>
#include <utility>

namespace cave {

namespace {
constexpr double kPi = 3.14159265358979323846;

double wrap_delta(double d, int n) {  // into [-n/2, n/2)
    const double half = n * 0.5;
    while (d >= half) d -= n;
    while (d < -half) d += n;
    return d;
}
}  // namespace

void displacement(double ax, double ay, double bx, double by, int W, int H, Boundary b, double& dx, double& dy) {
    dx = bx - ax;
    dy = by - ay;
    if (b == Boundary::Periodic) {
        dx = wrap_delta(dx, W);
        dy = wrap_delta(dy, H);
    }
}

ShapeMetrics compute_centroid(const Grid& g, Boundary b) {
    ShapeMetrics s;
    const int W = g.width(), H = g.height();
    const std::vector<float>& a = g.raw();
    double sx = 0, sy = 0, cxs = 0, cxc = 0, cys = 0, cyc = 0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const double v = a[static_cast<std::size_t>(y) * W + x];
            if (!(v > 0.0) || !std::isfinite(v)) continue;
            s.mass += v;
            if (b == Boundary::Periodic) {
                const double tx = 2.0 * kPi * (x + 0.5) / W, ty = 2.0 * kPi * (y + 0.5) / H;
                cxs += v * std::sin(tx); cxc += v * std::cos(tx);
                cys += v * std::sin(ty); cyc += v * std::cos(ty);
            } else {
                sx += v * (x + 0.5);
                sy += v * (y + 0.5);
            }
        }
    }
    if (s.mass <= 0.0) return s;
    if (b == Boundary::Periodic) {
        double ax = std::atan2(cxs, cxc), ay = std::atan2(cys, cyc);
        if (ax < 0) ax += 2.0 * kPi;
        if (ay < 0) ay += 2.0 * kPi;
        s.cx = ax * W / (2.0 * kPi);
        s.cy = ay * H / (2.0 * kPi);
    } else {
        s.cx = sx / s.mass;
        s.cy = sy / s.mass;
    }
    return s;
}

ShapeMetrics compute_shape(const Grid& g, Boundary b, float threshold) {
    ShapeMetrics s = compute_centroid(g, b);
    const int W = g.width(), H = g.height();
    const std::vector<float>& a = g.raw();
    if (s.mass <= 0.0) return s;
    // --- (centroid computed above) ---

    // --- spread: RMS minimal-image distance from centroid ---
    double acc = 0.0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const double v = a[static_cast<std::size_t>(y) * W + x];
            if (!(v > 0.0) || !std::isfinite(v)) continue;
            double dx, dy;
            displacement(s.cx, s.cy, x + 0.5, y + 0.5, W, H, b, dx, dy);
            acc += v * (dx * dx + dy * dy);
        }
    }
    s.spread = std::sqrt(acc / s.mass);

    // --- connected components (4-neighborhood) of cells >= threshold ---
    std::vector<unsigned char> seen(a.size(), 0);
    std::queue<std::pair<int, int>> q;
    for (int y0 = 0; y0 < H; ++y0) {
        for (int x0 = 0; x0 < W; ++x0) {
            const std::size_t i0 = static_cast<std::size_t>(y0) * W + x0;
            if (seen[i0] || !(a[i0] >= threshold)) continue;
            ++s.components;
            seen[i0] = 1;
            q.push({x0, y0});
            while (!q.empty()) {
                auto [x, y] = q.front(); q.pop();
                const int nx[4] = {x + 1, x - 1, x, x};
                const int ny[4] = {y, y, y + 1, y - 1};
                for (int k = 0; k < 4; ++k) {
                    int xx = nx[k], yy = ny[k];
                    if (b == Boundary::Periodic) { xx = Grid::wrap(xx, W); yy = Grid::wrap(yy, H); }
                    else if (xx < 0 || yy < 0 || xx >= W || yy >= H) continue;
                    const std::size_t j = static_cast<std::size_t>(yy) * W + xx;
                    if (seen[j] || !(a[j] >= threshold)) continue;
                    seen[j] = 1;
                    q.push({xx, yy});
                }
            }
        }
    }
    return s;
}

}  // namespace cave
