#ifndef COLOR_H
#define COLOR_H

#include "raylib.h"
#include <string.h>
#include <stdio.h>

static inline int HexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static inline Color ParseColor(const char* hexOrRgba) {
    if (!hexOrRgba) return BLANK;
    
    if (hexOrRgba[0] == '#') {
        int len = strlen(hexOrRgba);
        int r = 0, g = 0, b = 0, a = 255;
        if (len == 7) {
            int r1 = HexVal(hexOrRgba[1]);
            int r2 = HexVal(hexOrRgba[2]);
            int g1 = HexVal(hexOrRgba[3]);
            int g2 = HexVal(hexOrRgba[4]);
            int b1 = HexVal(hexOrRgba[5]);
            int b2 = HexVal(hexOrRgba[6]);
            if (r1 >= 0 && r2 >= 0 && g1 >= 0 && g2 >= 0 && b1 >= 0 && b2 >= 0) {
                r = (r1 << 4) | r2;
                g = (g1 << 4) | g2;
                b = (b1 << 4) | b2;
            }
        } else if (len == 9) {
            int r1 = HexVal(hexOrRgba[1]);
            int r2 = HexVal(hexOrRgba[2]);
            int g1 = HexVal(hexOrRgba[3]);
            int g2 = HexVal(hexOrRgba[4]);
            int b1 = HexVal(hexOrRgba[5]);
            int b2 = HexVal(hexOrRgba[6]);
            int a1 = HexVal(hexOrRgba[7]);
            int a2 = HexVal(hexOrRgba[8]);
            if (r1 >= 0 && r2 >= 0 && g1 >= 0 && g2 >= 0 && b1 >= 0 && b2 >= 0 && a1 >= 0 && a2 >= 0) {
                r = (r1 << 4) | r2;
                g = (g1 << 4) | g2;
                b = (b1 << 4) | b2;
                a = (a1 << 4) | a2;
            }
        } else if (len == 4) {
            int r1 = HexVal(hexOrRgba[1]);
            int g1 = HexVal(hexOrRgba[2]);
            int b1 = HexVal(hexOrRgba[3]);
            if (r1 >= 0 && g1 >= 0 && b1 >= 0) {
                r = (r1 << 4) | r1;
                g = (g1 << 4) | g1;
                b = (b1 << 4) | b1;
            }
        }
        return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
    } else if (strncmp(hexOrRgba, "rgba", 4) == 0) {
        int r, g, b;
        float a;
        if (sscanf(hexOrRgba, "rgba(%d,%d,%d,%f)", &r, &g, &b, &a) == 4) {
            return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(a * 255.0f) };
        }
    } else if (strncmp(hexOrRgba, "rgb", 3) == 0) {
        int r, g, b;
        if (sscanf(hexOrRgba, "rgb(%d,%d,%d)", &r, &g, &b) == 3) {
            return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
        }
    }
    return BLANK;
}

#endif // COLOR_H
