#ifndef ENGINE_SHARED_H
#define ENGINE_SHARED_H

#include "raylib.h"
#include "mparser.h"

// Shared Globals
extern UINode* root;
extern int layout_dirty;
extern Camera2D camera;
extern float shake_intensity;
extern float shake_duration;

extern int g_draw_calls;
extern int dev_tools_active;
extern int console_active;

#endif // ENGINE_SHARED_H
