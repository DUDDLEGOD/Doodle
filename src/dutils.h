#ifndef DUTILS_H
#define DUTILS_H

#include "raylib.h"

// Color parsing
Color ParseColor(const char* hexOrRgba);

// Unit normalization
float ParseUnit(const char* unitStr);

#endif // DUTILS_H
