#ifndef FAST_MATH_H
#define FAST_MATH_H

#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Fast pre-calculated lookup table for trig calculations
#define TRIG_LUT_SIZE 360

static inline float fast_sin(float x) {
    static float trig_sin_lut[TRIG_LUT_SIZE];
    static int trig_lut_initialized = 0;
    if (!trig_lut_initialized) {
        for (int i = 0; i < TRIG_LUT_SIZE; i++) {
            float rad = ((float)i / (float)TRIG_LUT_SIZE) * 2.0f * PI;
            trig_sin_lut[i] = sinf(rad);
        }
        trig_lut_initialized = 1;
    }
    // Wrap x to [0, 2*PI)
    float wrapped = fmodf(x, 2.0f * PI);
    if (wrapped < 0.0f) wrapped += 2.0f * PI;
    // Map to table index
    int idx = (int)((wrapped / (2.0f * PI)) * (float)TRIG_LUT_SIZE);
    if (idx < 0) idx = 0;
    if (idx >= TRIG_LUT_SIZE) idx = TRIG_LUT_SIZE - 1;
    return trig_sin_lut[idx];
}

static inline float fast_cos(float x) {
    return fast_sin(x + PI / 2.0f);
}

// High-speed LCG random number generator (1 clock cycle, thread-safe using local seed)
static inline float fast_rand_f(unsigned int* seed) {
    *seed = *seed * 1103515245 + 12345;
    return ((float)(*seed & 0x7FFFFFFF) / 2147483648.0f) * 2.0f - 1.0f;
}

#endif // FAST_MATH_H
