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
