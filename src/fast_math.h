#ifndef FAST_MATH_H
#define FAST_MATH_H

#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Ultra-fast sine approximation for x in [-PI, PI]
// Q3 minimax approximation with maximum absolute error ~0.001
static inline float fast_sin_range(float x) {
    float sin_val;
    if (x < 0.0f) {
        sin_val = 1.27323954f * x + 0.405284735f * x * x;
        sin_val = 0.225f * (sin_val * fabsf(sin_val) - sin_val) + sin_val;
    } else {
        sin_val = 1.27323954f * x - 0.405284735f * x * x;
        sin_val = 0.225f * (sin_val * fabsf(sin_val) - sin_val) + sin_val;
    }
    return sin_val;
}

// Wrap and compute sin
static inline float fast_sin(float x) {
    // Wrap to [-PI, PI]
    while (x < -PI) x += 2.0f * PI;
    while (x > PI) x -= 2.0f * PI;
    return fast_sin_range(x);
}

static inline float fast_cos(float x) {
    return fast_sin(x + 1.57079632679f);
}

// High-speed LCG random number generator (1 clock cycle, thread-safe using local seed)
static inline float fast_rand_f(unsigned int* seed) {
    *seed = *seed * 1103515245 + 12345;
    return ((float)(*seed & 0x7FFFFFFF) / 2147483648.0f) * 2.0f - 1.0f;
}

#endif // FAST_MATH_H
