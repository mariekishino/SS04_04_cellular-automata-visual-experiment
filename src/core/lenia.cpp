#include "core/lenia.hpp"

#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>
#include <sstream>

namespace cave {

Lenia::Lenia(int width, int height, const LeniaParams& p)
    : p_(p), a_(width, height, 0.0f), next_(width, height, 0.0f) {
    build_kernel();
    build_index_tables();
}

float Lenia::kernel_core(float r) {
    if (r <= 0.0f || r >= 1.0f) return 0.0f;
    return std::exp(4.0f - 1.0f / (r * (1.0f - r)));
}

float Lenia::growth(float u, float mu, float sigma) {
    const float d = (u - mu) / sigma;
    return 2.0f * std::exp(-0.5f * d * d) - 1.0f;
}

void Lenia::build_kernel() {
    const int R = p_.R;
    const int K = 2 * R + 1;
    k_.assign(static_cast<std::size_t>(K) * K, 0.0f);
    double sum = 0.0;
    for (int dy = -R; dy <= R; ++dy) {
        for (int dx = -R; dx <= R; ++dx) {
            const float r = std::sqrt(static_cast<float>(dx * dx + dy * dy)) / static_cast<float>(R);
            const float w = kernel_core(r);
            k_[static_cast<std::size_t>(dy + R) * K + (dx + R)] = w;
            sum += w;
        }
    }
    for (float& w : k_) w = static_cast<float>(w / sum);
}

void Lenia::build_index_tables() {
    const int R = p_.R;
    const int W = a_.width(), H = a_.height();
    xi_.assign(static_cast<std::size_t>(W + 2 * R), -1);
    yi_.assign(static_cast<std::size_t>(H + 2 * R), -1);
    for (int i = 0; i < W + 2 * R; ++i) {
        const int x = i - R;
        xi_[i] = (p_.boundary == Boundary::Periodic) ? Grid::wrap(x, W) : ((x >= 0 && x < W) ? x : -1);
    }
    for (int i = 0; i < H + 2 * R; ++i) {
        const int y = i - R;
        yi_[i] = (p_.boundary == Boundary::Periodic) ? Grid::wrap(y, H) : ((y >= 0 && y < H) ? y : -1);
    }
}

void Lenia::step() {
    const int R = p_.R;
    const int K = 2 * R + 1;
    const int W = a_.width(), H = a_.height();
    const float* a = a_.raw().data();
    float* out = next_.raw().data();

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // Potential U = K * A. Reads only from a_ (current), writes only to next_.
            double u = 0.0;
            for (int dy = -R; dy <= R; ++dy) {
                const int yy = yi_[y + dy + R];
                if (yy < 0) continue;  // Fixed boundary: outside reads 0, contributes nothing
                const float* row = a + static_cast<std::size_t>(yy) * W;
                const float* krow = k_.data() + static_cast<std::size_t>(dy + R) * K;
                for (int dx = -R; dx <= R; ++dx) {
                    const int xx = xi_[x + dx + R];
                    if (xx < 0) continue;
                    u += static_cast<double>(krow[dx + R]) * row[xx];
                }
            }
            const float g = growth(static_cast<float>(u), p_.mu, p_.sigma);
            const float v = a[static_cast<std::size_t>(y) * W + x] + p_.dt * g;
            out[static_cast<std::size_t>(y) * W + x] = std::min(1.0f, std::max(0.0f, v));
        }
    }
    a_.swap(next_);
    ++steps_;
}

std::vector<Channel> Lenia::channels() const {
    return {{"A", &a_}};
}

std::vector<std::pair<std::string, std::string>> Lenia::parameters() const {
    auto f = [](float v) { std::ostringstream os; os.precision(6); os << v; return os.str(); };
    return {
        {"R", std::to_string(p_.R)},
        {"mu", f(p_.mu)},
        {"sigma", f(p_.sigma)},
        {"dt", f(p_.dt)},
        {"kernel_core", "exp(4 - 1/(r(1-r)))"},
        {"growth", "2 exp(-(u-mu)^2 / (2 sigma^2)) - 1"},
    };
}

std::string Lenia::formula() const {
    return "Lenia-type (Chan 2019, kn=1, gn=1, b=[1]): A' = clip(A + dt * G(K * A), 0, 1)";
}

void Lenia::save_state(std::ostream& os) const {
    os.write(reinterpret_cast<const char*>(&steps_), sizeof(steps_));
    write_grid(os, a_);
}

void Lenia::load_state(std::istream& is) {
    is.read(reinterpret_cast<char*>(&steps_), sizeof(steps_));
    read_grid(is, a_);
    next_ = Grid(a_.width(), a_.height(), 0.0f);
    build_index_tables();
}

}  // namespace cave
