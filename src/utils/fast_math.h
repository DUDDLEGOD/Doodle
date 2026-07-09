#ifndef FAST_MATH_H
#define FAST_MATH_H

#include <math.h>
#include <stddef.h>

#ifndef TWO_PI
#define TWO_PI 6.28318530717958647693f
#define PI_HALF 1.57079632679489661923f
#endif

#define TRIG_LUT_SIZE 360

static inline float fast_sin(float x) {
    static float trig_sin_lut[TRIG_LUT_SIZE + 1] = {0};
    static int trig_lut_initialized = 0;

    if (!trig_lut_initialized) {
        for (int i = 0; i <= TRIG_LUT_SIZE; i++) {
            float rad = ((float)i / (float)TRIG_LUT_SIZE) * TWO_PI;
            trig_sin_lut[i] = sinf(rad);
        }
        trig_lut_initialized = 1;
    }

    if (!isfinite(x)) return 0.0f;

    float wrapped = fmodf(x, TWO_PI);
    if (wrapped < 0.0f) wrapped += TWO_PI;

    float pos = (wrapped / TWO_PI) * (float)TRIG_LUT_SIZE;
    int idx = (int)pos;
    float frac = pos - (float)idx;

    if (idx >= TRIG_LUT_SIZE) {
        idx = TRIG_LUT_SIZE - 1;
        frac = 0.0f;
    }

    // Linear interpolation (huge accuracy boost for ~1 extra cycle)
    return trig_sin_lut[idx] + frac * (trig_sin_lut[idx + 1] - trig_sin_lut[idx]);
}

static inline float fast_cos(float x) {
    return fast_sin(x + PI_HALF);
}

// LCG is perfect
static inline float fast_rand_f(unsigned int* seed) {
    *seed = *seed * 1103515245 + 12345;
    return ((float)(*seed & 0x7FFFFFFF) / 2147483648.0f) * 2.0f - 1.0f;
}

#endif
