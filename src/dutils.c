#include "dutils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Color ParseColor(const char* hexOrRgba) {
    if (!hexOrRgba) return BLANK;
    
    if (hexOrRgba[0] == '#') {
        int r = 0, g = 0, b = 0, a = 255;
        int len = strlen(hexOrRgba);
        if (len == 7) {
            sscanf(hexOrRgba, "#%02x%02x%02x", &r, &g, &b);
        } else if (len == 9) {
            sscanf(hexOrRgba, "#%02x%02x%02x%02x", &r, &g, &b, &a);
        } else if (len == 4) {
            sscanf(hexOrRgba, "#%1x%1x%1x", &r, &g, &b);
            r = (r << 4) | r;
            g = (g << 4) | g;
            b = (b << 4) | b;
        }
        return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
    } else if (strncmp(hexOrRgba, "rgba", 4) == 0) {
        int r, g, b;
        float a;
        if (sscanf(hexOrRgba, "rgba(%d,%d,%d,%f)", &r, &g, &b, &a) == 4) {
            return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(a * 255) };
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
    char temp[128];
    strncpy(temp, unitStr, sizeof(temp));
    temp[sizeof(temp)-1] = '\0';
    char* trimmed = TrimWhitespace(temp);
    int len = strlen(trimmed);
    if (len > 0 && trimmed[len - 1] == '%') {
        if (isPercent) *isPercent = 1;
        trimmed[len - 1] = '\0';
        return (float)atof(trimmed);
    }
    if (isPercent) *isPercent = 0;
    return (float)atof(trimmed);
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
