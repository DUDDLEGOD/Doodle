#ifndef UNIT_H
#define UNIT_H

#include <stdlib.h>

static inline float ParseUnit(const char* unitStr) {
    if (!unitStr) return 0.0f;
    return (float)atof(unitStr);
}

static inline float ParseUnitExt(const char* unitStr, int* isPercent) {
    if (!unitStr) {
        if (isPercent) *isPercent = 0;
        return 0.0f;
    }
    const char* start = unitStr;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') start++;
    if (*start == '\0') {
        if (isPercent) *isPercent = 0;
        return 0.0f;
    }
    char* endptr = NULL;
    float val = strtof(start, &endptr);
    if (endptr) {
        while (*endptr == ' ' || *endptr == '\t' || *endptr == '\r' || *endptr == '\n') endptr++;
        if (*endptr == '%') {
            if (isPercent) *isPercent = 1;
        } else {
            if (isPercent) *isPercent = 0;
        }
    } else {
        if (isPercent) *isPercent = 0;
    }
    return val;
}

#endif // UNIT_H
