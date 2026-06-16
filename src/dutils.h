#ifndef DUTILS_H
#define DUTILS_H

#include "raylib.h"

// Color parsing
Color ParseColor(const char* hexOrRgba);

// Unit normalization
float ParseUnit(const char* unitStr);
float ParseUnitExt(const char* unitStr, int* isPercent);

// Trim helper
char* TrimWhitespace(char* str);

// Class list presence check
int HasClass(const char* class_list, const char* cls);

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

