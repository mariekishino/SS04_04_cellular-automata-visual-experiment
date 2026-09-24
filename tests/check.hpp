#pragma once
// Tiny assertion helper: no framework, prints and exits non-zero on failure.
#include <cstdio>
#include <cstdlib>
#define CHECK(cond) do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); std::exit(1); } } while (0)
#define CHECK_NEAR(a, b, eps) do { double _a = (a), _b = (b); if (!((_a - _b) < (eps) && (_b - _a) < (eps))) { std::fprintf(stderr, "FAIL %s:%d: %s = %g vs %s = %g\n", __FILE__, __LINE__, #a, _a, #b, _b); std::exit(1); } } while (0)
