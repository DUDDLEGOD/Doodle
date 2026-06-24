#ifndef DUTILS_H
#define DUTILS_H

// Fast math helpers
static inline float DMax(float a, float b) { return (a > b) ? a : b; }
static inline float DMin(float a, float b) { return (a < b) ? a : b; }
static inline float DClamp(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}
static inline float DLerp(float a, float b, float t) { return a + t * (b - a); }

#endif // DUTILS_H
