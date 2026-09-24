#include "core/gray_scott.hpp"

#include <istream>
#include <ostream>
#include <sstream>

namespace cave {

GrayScott::GrayScott(int width, int height, const GrayScottParams& p)
    : p_(p), u_(width, height, 1.0f), v_(width, height, 0.0f), un_(width, height, 0.0f), vn_(width, height, 0.0f) {}

float GrayScott::laplacian(const Grid& g, int x, int y, Boundary b, float outside) {
    const float c = g.at(x, y);
    const float e = g.read(x + 1, y, b, outside) + g.read(x - 1, y, b, outside)
                  + g.read(x, y + 1, b, outside) + g.read(x, y - 1, b, outside);
    const float d = g.read(x + 1, y + 1, b, outside) + g.read(x - 1, y + 1, b, outside)
                  + g.read(x + 1, y - 1, b, outside) + g.read(x - 1, y - 1, b, outside);
    return -c + 0.2f * e + 0.05f * d;
}

void GrayScott::step() {
    const int W = u_.width(), H = u_.height();
    const Boundary b = p_.boundary;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const float u = u_.at(x, y);
            const float v = v_.at(x, y);
            const float lu = laplacian(u_, x, y, b, 1.0f);  // rest state u = 1 outside
            const float lv = laplacian(v_, x, y, b, 0.0f);  // rest state v = 0 outside
            const float uvv = u * v * v;
            un_.at(x, y) = u + p_.dt * (p_.Du * lu - uvv + p_.F * (1.0f - u));
            vn_.at(x, y) = v + p_.dt * (p_.Dv * lv + uvv - (p_.F + p_.k) * v);
        }
    }
    u_.swap(un_);
    v_.swap(vn_);
    ++steps_;
}

std::vector<Channel> GrayScott::channels() const {
    return {{"v", &v_}, {"u", &u_}};  // v is drawn: it carries the pattern
}

std::vector<std::pair<std::string, std::string>> GrayScott::parameters() const {
    auto f = [](float v) { std::ostringstream os; os.precision(6); os << v; return os.str(); };
    return {
        {"Du", f(p_.Du)}, {"Dv", f(p_.Dv)}, {"F", f(p_.F)}, {"k", f(p_.k)}, {"dt", f(p_.dt)},
        {"laplacian", "9-point: center -1, edge 0.2, corner 0.05"},
    };
}

std::string GrayScott::formula() const {
    return "Gray-Scott (Pearson 1993, Sims discretization): u' = u + dt(Du lap u - u v^2 + F(1-u)); v' = v + dt(Dv lap v + u v^2 - (F+k) v)";
}

void GrayScott::save_state(std::ostream& os) const {
    os.write(reinterpret_cast<const char*>(&steps_), sizeof(steps_));
    write_grid(os, u_);
    write_grid(os, v_);
}

void GrayScott::load_state(std::istream& is) {
    is.read(reinterpret_cast<char*>(&steps_), sizeof(steps_));
    read_grid(is, u_);
    read_grid(is, v_);
    un_ = Grid(u_.width(), u_.height(), 0.0f);
    vn_ = Grid(v_.width(), v_.height(), 0.0f);
}

}  // namespace cave
