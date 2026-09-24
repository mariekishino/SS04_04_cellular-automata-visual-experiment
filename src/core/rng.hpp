#pragma once
// Seeded random number generator. std::mt19937 gives the same sequence for
// the same seed on every platform, which is what reproducibility needs.
#include <cstdint>
#include <random>

namespace cave {

class Rng {
public:
    explicit Rng(std::uint32_t seed) : gen_(seed) {}
    float uniform01() { return dist_(gen_); }
    int uniform_int(int lo, int hi) {  // inclusive range
        std::uniform_int_distribution<int> d(lo, hi);
        return d(gen_);
    }
private:
    std::mt19937 gen_;
    std::uniform_real_distribution<float> dist_{0.0f, 1.0f};
};

}  // namespace cave
