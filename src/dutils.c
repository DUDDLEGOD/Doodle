#include "dutils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static inline int HexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

Color ParseColor(const char* hexOrRgba) {
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

float ParseUnit(const char* unitStr) {
    if (!unitStr) return 0.0f;
    return (float)atof(unitStr);
}

char* TrimWhitespace(char* str) {
    if (!str) return NULL;
    while (*str && (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;
    end[1] = '\0';
    return str;
}

float ParseUnitExt(const char* unitStr, int* isPercent) {
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

int HasClass(const char* class_list, const char* cls) {
    if (!class_list || !cls) return 0;
    const char* p = class_list;
    int len = strlen(cls);
    while ((p = strstr(p, cls))) {
        if (p == class_list || *(p - 1) == ' ') {
            char next = *(p + len);
            if (next == '\0' || next == ' ') {
                return 1;
            }
        }
        p += len;
    }
    return 0;
}
