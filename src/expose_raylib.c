#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "raylib.h"
#include "mparser.h"
#include "color.h"
#include "unit.h"
#include "string_utils.h"
#include "dom_utils.h"
#include "daudio.h"
#include "html_parser.h"
#include "css_parser.h"
#include "layout.h"
#include "renderer.h"
#include "particles.h"
#include "cache.h"
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <math.h>

UINode* root = NULL;
int layout_dirty = 1;
static PyObject* tick_callback = NULL;

// Camera state
Camera2D camera = { .zoom = 1.0f };
float shake_intensity = 0.0f;
float shake_duration = 0.0f;

// Collision Helper
static UINode* FindCollisionNode(UINode* current, UINode* target, const char* group, Rectangle target_rec) {
    if (!current || !current->visible) return NULL;

    if (current != target && strlen(current->class_name) > 0) {
        int match = 0;
        if (HasClass(current->class_name, group)) match = 1;
        int len = strlen(group);
        if (len > 1 && group[len-1] == 's') {
            char singular[64];
            strncpy(singular, group, len - 1);
            singular[len - 1] = '\0';
            if (HasClass(current->class_name, singular)) match = 1;
        }

        if (match) {
            int col = 0;
            if (target->type == NODE_CIRCLE && current->type == NODE_CIRCLE) {
                float r_t = target->radius > 0 ? target->radius : target->layout.width / 2.0f;
                float r_c = current->radius > 0 ? current->radius : current->layout.width / 2.0f;
                Vector2 center_t = {target->layout.x + r_t, target->layout.y + r_t};
                Vector2 center_c = {current->layout.x + r_c, current->layout.y + r_c};
                col = CheckCollisionCircles(center_t, r_t, center_c, r_c);
            } else if (target->type == NODE_CIRCLE) {
                float r = target->radius > 0 ? target->radius : target->layout.width / 2.0f;
                Vector2 center = {target->layout.x + r, target->layout.y + r};
                Rectangle rec = {current->layout.x, current->layout.y, current->layout.width, current->layout.height};
                col = CheckCollisionCircleRec(center, r, rec);
            } else if (current->type == NODE_CIRCLE) {
                float r = current->radius > 0 ? current->radius : current->layout.width / 2.0f;
                Vector2 center = {current->layout.x + r, current->layout.y + r};
                Rectangle rec = {target->layout.x, target->layout.y, target->layout.width, target->layout.height};
                col = CheckCollisionCircleRec(center, r, rec);
            } else {
                Rectangle rec = {current->layout.x, current->layout.y, current->layout.width, current->layout.height};
                col = CheckCollisionRecs(target_rec, rec);
            }

            if (col) {
                return current;
            }
        }
    }

    for (int i = 0; i < current->child_count; i++) {
        UINode* collision = FindCollisionNode(current->children[i], target, group, target_rec);
        if (collision) return collision;
    }
    return NULL;
}

// CPython Bindings
static PyObject* doodle_register_tick_callback(PyObject* self, PyObject* args) {
    PyObject* temp;
    if (PyArg_ParseTuple(args, "O:register_tick_callback", &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        Py_XINCREF(temp);
        Py_XDECREF(tick_callback);
        tick_callback = temp;
    }
    Py_RETURN_NONE;
}

static PyObject* doodle_check_collision(PyObject* self, PyObject* args) {
    const char* id_a;
    const char* id_b;
    if (!PyArg_ParseTuple(args, "ss", &id_a, &id_b)) return NULL;

    UINode* node_a = FindNodeById(root, id_a);
    UINode* node_b = FindNodeById(root, id_b);

    if (!node_a || !node_b || !node_a->visible || !node_b->visible) Py_RETURN_FALSE;

    int col = 0;
    if (node_a->type == NODE_CIRCLE && node_b->type == NODE_CIRCLE) {
        float r_a = node_a->radius > 0 ? node_a->radius : node_a->layout.width / 2.0f;
        float r_b = node_b->radius > 0 ? node_b->radius : node_b->layout.width / 2.0f;
        Vector2 center_a = {node_a->layout.x + r_a, node_a->layout.y + r_a};
        Vector2 center_b = {node_b->layout.x + r_b, node_b->layout.y + r_b};
        col = CheckCollisionCircles(center_a, r_a, center_b, r_b);
    } else if (node_a->type == NODE_CIRCLE) {
        float r = node_a->radius > 0 ? node_a->radius : node_a->layout.width / 2.0f;
        Vector2 center = {node_a->layout.x + r, node_a->layout.y + r};
        Rectangle rec = {node_b->layout.x, node_b->layout.y, node_b->layout.width, node_b->layout.height};
        col = CheckCollisionCircleRec(center, r, rec);
    } else if (node_b->type == NODE_CIRCLE) {
        float r = node_b->radius > 0 ? node_b->radius : node_b->layout.width / 2.0f;
        Vector2 center = {node_b->layout.x + r, node_b->layout.y + r};
        Rectangle rec = {node_a->layout.x, node_a->layout.y, node_a->layout.width, node_a->layout.height};
        col = CheckCollisionCircleRec(center, r, rec);
    } else {
        Rectangle rec_a = {node_a->layout.x, node_a->layout.y, node_a->layout.width, node_a->layout.height};
        Rectangle rec_b = {node_b->layout.x, node_b->layout.y, node_b->layout.width, node_b->layout.height};
        col = CheckCollisionRecs(rec_a, rec_b);
    }

    if (col) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_get_first_collision(PyObject* self, PyObject* args, PyObject* kwargs) {
    const char* id;
    const char* group;
    static char* kwlist[] = {"id", "group", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss", kwlist, &id, &group)) return NULL;

    UINode* node = FindNodeById(root, id);
    if (!node || !node->visible) Py_RETURN_NONE;

    Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
    UINode* collision = FindCollisionNode(root, node, group, rec);
    if (collision) {
        return Py_BuildValue("s", collision->id);
    }
    Py_RETURN_NONE;
}

static PyObject* doodle_set_position(PyObject* self, PyObject* args) {
    const char* id;
    float x, y;
    if (!PyArg_ParseTuple(args, "sff", &id, &x, &y)) return NULL;

    UINode* node = FindNodeById(root, id);
    if (node) {
        node->layout.x = x;
        node->layout.y = y;
        node->position_set = 1;
        layout_dirty = 1;
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_get_position(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;

    UINode* node = FindNodeById(root, id);
    if (node) {
        return Py_BuildValue("ff", node->layout.x, node->layout.y);
    }
    return Py_BuildValue("ff", 0.0f, 0.0f);
}

static PyObject* doodle_remove_node(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;

    UINode* node = FindNodeById(root, id);
    if (node) {
        RemoveNode(root, node);
        layout_dirty = 1;
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_update_text(PyObject* self, PyObject* args) {
    const char* id;
    const char* text;
    if (!PyArg_ParseTuple(args, "ss", &id, &text)) return NULL;

    UINode* node = FindNodeById(root, id);
    if (node) {
        if (strcmp(node->text_content, text) != 0) {
            strncpy(node->text_content, text, sizeof(node->text_content) - 1);
            node->text_content[sizeof(node->text_content) - 1] = '\0';
            layout_dirty = 1;
        }
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_show_node(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;

    UINode* node = FindNodeById(root, id);
    if (node) {
        node->visible = 1;
        layout_dirty = 1;
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_hide_node(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;

    UINode* node = FindNodeById(root, id);
    if (node) {
        node->visible = 0;
        layout_dirty = 1;
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_play_sound(PyObject* self, PyObject* args) {
    const char* sound_path;
    if (!PyArg_ParseTuple(args, "s", &sound_path)) return NULL;
    Sound snd = GetCachedSound(sound_path);
    if (snd.frameCount > 0) PlaySound(snd);
    Py_RETURN_NONE;
}

static PyObject* doodle_spawn_particles(PyObject* self, PyObject* args) {
    float x, y;
    int count;
    const char* color_hex;
    float speed;
    float lifetime;
    if (!PyArg_ParseTuple(args, "ffisff", &x, &y, &count, &color_hex, &speed, &lifetime)) return NULL;
    
    SpawnParticles(x, y, count, color_hex, speed, lifetime);
    
    Py_RETURN_NONE;
}

static PyObject* doodle_play_synth(PyObject* self, PyObject* args) {
    float freq;
    float duration;
    int wave_type = WAVE_SQUARE;
    float attack = 0.01f;
    float decay = 0.05f;
    float sustain = 0.5f;
    float release = 0.05f;

    if (!PyArg_ParseTuple(args, "ff|iffff", &freq, &duration, &wave_type, &attack, &decay, &sustain, &release)) {
        return NULL;
    }

    ADSREnvelope env = { attack, decay, sustain, release };
    PlaySynthTone(freq, duration, wave_type, env);

    Py_RETURN_NONE;
}

static int IsNodeInCamera(UINode* node) {
    if (!node) return 0;
    UINode* p = node->parent;
    while (p) {
        if (p->use_camera) return 1;
        p = p->parent;
    }
    return 0;
}

static PyObject* doodle_is_node_hovered(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;
    UINode* node = FindNodeById(root, id);
    if (node && node->visible) {
        Vector2 mouse = GetMousePosition();
        if (IsNodeInCamera(node)) {
            mouse = GetScreenToWorld2D(mouse, camera);
        }
        Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
        if (CheckCollisionPointRec(mouse, rec)) {
            Py_RETURN_TRUE;
        }
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_is_node_clicked(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;
    UINode* node = FindNodeById(root, id);
    if (node && node->visible) {
        Vector2 mouse = GetMousePosition();
        if (IsNodeInCamera(node)) {
            mouse = GetScreenToWorld2D(mouse, camera);
        }
        Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
        if (CheckCollisionPointRec(mouse, rec) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Py_RETURN_TRUE;
        }
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_set_style(PyObject* self, PyObject* args) {
    const char* id;
    const char* property_name;
    const char* property_value;
    if (!PyArg_ParseTuple(args, "sss", &id, &property_name, &property_value)) return NULL;

    UINode* node = FindNodeById(root, id);
    if (node) {
        extern CSSMap css_registry[];
        extern int css_registry_count;
        int applied = 0;
        int affects_layout = 0;
        for (int i = 0; i < css_registry_count; i++) {
            if (strcmp(css_registry[i].property_name, property_name) == 0) {
                css_registry[i].handler(node, property_value);
                StyleProps temp = node->style;
                node->style = node->hover_style;
                css_registry[i].handler(node, property_value);
                node->hover_style = node->style;
                node->style = temp;
                applied = 1;
                affects_layout = css_registry[i].affects_layout;
                break;
            }
        }
        if (applied) {
            if (affects_layout) {
                layout_dirty = 1;
            }
            Py_RETURN_TRUE;
        }
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_get_style(PyObject* self, PyObject* args) {
    const char* id;
    const char* property_name;
    if (!PyArg_ParseTuple(args, "ss", &id, &property_name)) return NULL;

    UINode* node = FindNodeById(root, id);
    if (node) {
        char val[128] = {0};
        GetStyleProperty(node, property_name, val, sizeof(val));
        return Py_BuildValue("s", val);
    }
    return Py_BuildValue("s", "");
}

static PyObject* doodle_set_camera(PyObject* self, PyObject* args) {
    float tx, ty, ox, oy, zoom, rot;
    if (!PyArg_ParseTuple(args, "ffffff", &tx, &ty, &ox, &oy, &zoom, &rot)) return NULL;
    camera.target = (Vector2){tx, ty};
    camera.offset = (Vector2){ox, oy};
    camera.zoom = zoom;
    camera.rotation = rot;
    Py_RETURN_NONE;
}

static PyObject* doodle_shake_camera(PyObject* self, PyObject* args) {
    float intensity, duration;
    if (!PyArg_ParseTuple(args, "ff", &intensity, &duration)) return NULL;
    shake_intensity = intensity;
    shake_duration = duration;
    Py_RETURN_NONE;
}

static PyObject* doodle_is_key_down(PyObject* self, PyObject* args) {
    int key;
    if (!PyArg_ParseTuple(args, "i", &key)) return NULL;
    if (IsKeyDown(key)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_is_key_pressed(PyObject* self, PyObject* args) {
    int key;
    if (!PyArg_ParseTuple(args, "i", &key)) return NULL;
    if (IsKeyPressed(key)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_get_mouse_x(PyObject* self, PyObject* args) {
    return Py_BuildValue("i", GetMouseX());
}

static PyObject* doodle_get_mouse_y(PyObject* self, PyObject* args) {
    return Py_BuildValue("i", GetMouseY());
}

static PyObject* doodle_get_mouse_position(PyObject* self, PyObject* args) {
    Vector2 m = GetMousePosition();
    return Py_BuildValue("ff", m.x, m.y);
}

static PyObject* doodle_is_mouse_button_down(PyObject* self, PyObject* args) {
    int button;
    if (!PyArg_ParseTuple(args, "i", &button)) return NULL;
    if (IsMouseButtonDown(button)) Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* doodle_is_mouse_button_pressed(PyObject* self, PyObject* args) {
    int button;
    if (!PyArg_ParseTuple(args, "i", &button)) return NULL;
    if (IsMouseButtonPressed(button)) Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* doodle_get_mouse_wheel_move(PyObject* self, PyObject* args) {
    return Py_BuildValue("f", GetMouseWheelMove());
}

static PyObject* doodle_set_mouse_cursor(PyObject* self, PyObject* args) {
    int cursor;
    if (!PyArg_ParseTuple(args, "i", &cursor)) return NULL;
    SetMouseCursor(cursor);
    Py_RETURN_NONE;
}

static PyObject* doodle_set_window_title(PyObject* self, PyObject* args) {
    const char* title;
    if (!PyArg_ParseTuple(args, "s", &title)) return NULL;
    SetWindowTitle(title);
    Py_RETURN_NONE;
}

static PyObject* doodle_toggle_fullscreen(PyObject* self, PyObject* args) {
    ToggleFullscreen();
    Py_RETURN_NONE;
}

static PyObject* doodle_get_screen_size(PyObject* self, PyObject* args) {
    return Py_BuildValue("ii", GetScreenWidth(), GetScreenHeight());
}

static int UpdateHoverStates(UINode* node) {
    if (!node || !node->visible) return 0;
    int changed = 0;

    Vector2 mouse = GetMousePosition();
    if (IsNodeInCamera(node)) {
        mouse = GetScreenToWorld2D(mouse, camera);
    }

    Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
    int is_hovered = CheckCollisionPointRec(mouse, rec);

    if (is_hovered != node->currently_hovered) {
        node->currently_hovered = is_hovered;
        if (node->has_hover_style) {
            changed = 1;
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        if (UpdateHoverStates(node->children[i])) {
            changed = 1;
        }
    }
    return changed;
}

typedef struct {
    char id[64];
    float x, y;
} SavedPos;

static SavedPos saved_positions[512];
static int saved_positions_count = 0;

static void SavePositions(UINode* n) {
    if (!n) return;
    if (n->position_set && saved_positions_count < 512) {
        strncpy(saved_positions[saved_positions_count].id, n->id, 63);
        saved_positions[saved_positions_count].id[63] = '\0';
        saved_positions[saved_positions_count].x = n->layout.x;
        saved_positions[saved_positions_count].y = n->layout.y;
        saved_positions_count++;
    }
    for (int i = 0; i < n->child_count; i++) {
        SavePositions(n->children[i]);
    }
}

static void RestorePositions(UINode* n) {
    if (!n) return;
    for (int i = 0; i < saved_positions_count; i++) {
        if (strcmp(n->id, saved_positions[i].id) == 0) {
            n->layout.x = saved_positions[i].x;
            n->layout.y = saved_positions[i].y;
            n->position_set = 1;
            break;
        }
    }
    for (int i = 0; i < n->child_count; i++) {
        RestorePositions(n->children[i]);
    }
}

static PyObject* doodle_run(PyObject* self, PyObject* args, PyObject* kwargs) {
    char* layout = "layout.html";
    char* style = "styles.css";
    int width = 800;
    int height = 600;
    char* title = "Doodle Engine";

    static char* kwlist[] = {"layout", "style", "width", "height", "title", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ssiis", kwlist, &layout, &style, &width, &height, &title)) {
        return NULL;
    }

    InitWindow(width, height, title);
    InitAudioDevice();
    InitSynth();
    SetTargetFPS(60);

    root = ParseHTML(layout);
    if (root) {
        LoadAndApplyCSS(root, style);
        SetupAudioNodes(root);
        ComputeLayout(root, 0, 0, (float)width, (float)height);
    }
    layout_dirty = 0;

    time_t layout_mtime = 0;
    time_t style_mtime = 0;
    struct stat st;
    if (stat(layout, &st) == 0) layout_mtime = st.st_mtime;
    if (stat(style, &st) == 0) style_mtime = st.st_mtime;

    while (!WindowShouldClose()) {
        particles_drawn_this_frame = 0;
        struct stat st_now;
        int reload_needed = 0;
        if (stat(layout, &st_now) == 0 && st_now.st_mtime != layout_mtime) {
            layout_mtime = st_now.st_mtime;
            reload_needed = 1;
        }
        if (stat(style, &st_now) == 0 && st_now.st_mtime != style_mtime) {
            style_mtime = st_now.st_mtime;
            reload_needed = 1;
        }

        if (reload_needed) {
            saved_positions_count = 0;
            SavePositions(root);

            FreeNode(root);
            UnloadActiveMusics();

            root = ParseHTML(layout);
            if (root) {
                LoadAndApplyCSS(root, style);
                RestorePositions(root);
                SetupAudioNodes(root);
                ComputeLayout(root, 0, 0, (float)width, (float)height);
            }
            layout_dirty = 0;
        }

        if (root && UpdateHoverStates(root)) {
            layout_dirty = 1;
        }

        if (tick_callback) {
            PyObject* result = PyObject_CallObject(tick_callback, NULL);
            if (!result) {
                PyErr_Print();
            } else {
                Py_DECREF(result);
            }
        }

        if (layout_dirty && root) {
            ComputeLayout(root, 0, 0, (float)width, (float)height);
            layout_dirty = 0;
        }

        UpdateMusicStreams();

        BeginDrawing();
        ClearBackground(BLACK);
        
        if (root) {
            DrawUINode(root);
        }
        
        UpdateAndDrawParticles();
        
        EndDrawing();
    }

    UnloadActiveMusics();
    UnloadCachedSounds();
    UnloadCachedTextures();
    UnloadCachedShaders();
    UnloadCachedFonts();

    CloseSynth();
    CloseAudioDevice();
    CloseWindow();
    
    if (root) {
        FreeNode(root);
        root = NULL;
    }

    Py_RETURN_NONE;
}

static PyMethodDef DoodleMethods[] = {
    {"register_tick_callback", doodle_register_tick_callback, METH_VARARGS, "Register update callback"},
    {"check_collision", doodle_check_collision, METH_VARARGS, "Check collision between two IDs"},
    {"get_first_collision", (PyCFunction)doodle_get_first_collision, METH_VARARGS | METH_KEYWORDS, "Get first collision in group"},
    {"set_position", doodle_set_position, METH_VARARGS, "Set absolute layout pos"},
    {"get_position", doodle_get_position, METH_VARARGS, "Get layout pos"},
    {"remove_node", doodle_remove_node, METH_VARARGS, "Remove node from DOM tree"},
    {"update_text", doodle_update_text, METH_VARARGS, "Update text content of node"},
    {"show_node", doodle_show_node, METH_VARARGS, "Show hidden node"},
    {"hide_node", doodle_hide_node, METH_VARARGS, "Hide node"},
    {"play_sound", doodle_play_sound, METH_VARARGS, "Play sound"},
    {"is_key_down", doodle_is_key_down, METH_VARARGS, "Check if key is down"},
    {"is_key_pressed", doodle_is_key_pressed, METH_VARARGS, "Check if key is pressed"},
    {"get_mouse_x", doodle_get_mouse_x, METH_NOARGS, "Get mouse X coordinate"},
    {"get_mouse_y", doodle_get_mouse_y, METH_NOARGS, "Get mouse Y coordinate"},
    {"get_mouse_position", doodle_get_mouse_position, METH_NOARGS, "Get mouse position"},
    {"is_mouse_button_down", doodle_is_mouse_button_down, METH_VARARGS, "Check if mouse button is down"},
    {"is_mouse_button_pressed", doodle_is_mouse_button_pressed, METH_VARARGS, "Check if mouse button is pressed"},
    {"get_mouse_wheel_move", doodle_get_mouse_wheel_move, METH_NOARGS, "Get mouse wheel movement"},
    {"set_mouse_cursor", doodle_set_mouse_cursor, METH_VARARGS, "Set mouse cursor type"},
    {"set_window_title", doodle_set_window_title, METH_VARARGS, "Set window title"},
    {"toggle_fullscreen", doodle_toggle_fullscreen, METH_NOARGS, "Toggle fullscreen mode"},
    {"get_screen_size", doodle_get_screen_size, METH_NOARGS, "Get current screen size"},
    {"is_node_hovered", doodle_is_node_hovered, METH_VARARGS, "Check if node is hovered"},
    {"is_node_clicked", doodle_is_node_clicked, METH_VARARGS, "Check if node is clicked"},
    {"set_style", doodle_set_style, METH_VARARGS, "Set a style property dynamically"},
    {"get_style", doodle_get_style, METH_VARARGS, "Get a style property dynamically"},
    {"set_camera", doodle_set_camera, METH_VARARGS, "Set 2D camera parameters"},
    {"shake_camera", doodle_shake_camera, METH_VARARGS, "Trigger camera screen shake"},
    {"spawn_particles", doodle_spawn_particles, METH_VARARGS, "Spawn particle explosion burst"},
    {"play_synth", doodle_play_synth, METH_VARARGS, "Play procedurally synthesized tone"},
    {"run", (PyCFunction)doodle_run, METH_VARARGS | METH_KEYWORDS, "Start the engine loop"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef doodlemodule = {
    PyModuleDef_HEAD_INIT,
    "_doodle",
    "Hardware-accelerated DOM-based UI & 2D Game Engine",
    -1,
    DoodleMethods
};

PyMODINIT_FUNC PyInit__doodle(void) {
    return PyModule_Create(&doodlemodule);
}
