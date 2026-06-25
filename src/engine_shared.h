#ifndef ENGINE_SHARED_H
#define ENGINE_SHARED_H

#include "raylib.h"
#include "mparser.h"

typedef struct {
    UINode* root;
    int layout_dirty;
    Camera2D camera;
    float shake_intensity;
    float shake_duration;
    
    int g_draw_calls;
    int dev_tools_active;
    int console_active;
} DoodleContext;

extern DoodleContext ctx;

#endif // ENGINE_SHARED_H
