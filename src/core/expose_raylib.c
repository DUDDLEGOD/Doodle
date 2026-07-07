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
#include "profiler.h"
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "engine_shared.h"

DoodleContext ctx = {0};

static PyObject* tick_callback = NULL;

// Camera state
Camera2D camera = { .zoom = 1.0f };

// Collision Helper
static UINode* FindCollisionNode(UINode* current, UINode* target, const char* group, Rectangle target_rec) {
    if (!current || !current->visible) return NULL;

    if (current != target && strlen(current->class_name) > 0) {
        int match = 0;
        if (HasClass(current->class_name, group)) match = 1;
        int len = strlen(group);
        if (len > 1 && group[len-1] == 's') {
            char singular[64];
            snprintf(singular, sizeof(singular), "%.*s", len - 1, group);
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

static UINode* GetNodeOrRaise(const char* id) {
    UINode* node = FindNodeById(ctx.root, id);
    if (!node) {
        PyErr_Format(PyExc_KeyError, "Node with ID '%s' not found in the DOM tree.", id);
    }
    return node;
}

static PyObject* doodle_get_line_number(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;
    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    return PyLong_FromLong(node->line_number);
}

static PyObject* doodle_get_layout_size(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;
    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    return Py_BuildValue("ff", node->layout.width, node->layout.height);
}

static PyObject* doodle_is_visible(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;
    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    return PyBool_FromLong(node->visible);
}

static PyObject* doodle_get_text(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;
    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    return PyUnicode_FromString(node->text_content);
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

    UINode* node_a = GetNodeOrRaise(id_a);
    if (!node_a) return NULL;
    UINode* node_b = GetNodeOrRaise(id_b);
    if (!node_b) return NULL;

    if (!node_a->visible || !node_b->visible) Py_RETURN_FALSE;

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

    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    if (!node->visible) Py_RETURN_NONE;

    Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
    UINode* collision = FindCollisionNode(ctx.root, node, group, rec);
    if (collision) {
        return Py_BuildValue("s", collision->id);
    }
    Py_RETURN_NONE;
}

static PyObject* doodle_set_position(PyObject* self, PyObject* args) {
    const char* id;
    float x, y;
    if (!PyArg_ParseTuple(args, "sff", &id, &x, &y)) return NULL;

    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;

    node->style.left = x;
    node->style.top = y;
    ctx.layout_dirty = 1;
    Py_RETURN_TRUE;
}

static PyObject* doodle_get_position(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;

    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    return Py_BuildValue("ff", node->style.left, node->style.top);
}

static PyObject* doodle_remove_node(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;

    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    RemoveNode(ctx.root, node);
    ctx.layout_dirty = 1;
    Py_RETURN_TRUE;
}

static PyObject* doodle_update_text(PyObject* self, PyObject* args) {
    const char* id;
    const char* text;
    if (!PyArg_ParseTuple(args, "ss", &id, &text)) return NULL;

    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    if (strcmp(node->text_content, text) != 0) {
        node->text_content = DOMStrDup(text);
        ctx.layout_dirty = 1;
    }
    Py_RETURN_TRUE;
}

static PyObject* doodle_show_node(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;

    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    node->visible = 1;
    ctx.layout_dirty = 1;
    Py_RETURN_TRUE;
}

static PyObject* doodle_hide_node(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;

    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    node->visible = 0;
    ctx.layout_dirty = 1;
    Py_RETURN_TRUE;
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

static PyObject* doodle_play_synth(PyObject* self, PyObject* args, PyObject* kwargs) {
    float freq;
    float duration;
    int wave_type = WAVE_SQUARE;
    float attack = 0.01f;
    float decay = 0.05f;
    float sustain = 0.5f;
    float release = 0.05f;
    float frequency_slide = 0.0f;
    float vibrato_speed = 0.0f;
    float vibrato_depth = 0.0f;
    float tremolo_speed = 0.0f;
    float tremolo_depth = 0.0f;
    float filter_cutoff = 0.0f;
    float pan = 0.0f;

    static char* kwlist[] = {
        "frequency", "duration", "wave_type",
        "attack", "decay", "sustain", "release",
        "frequency_slide", "vibrato_speed", "vibrato_depth",
        "tremolo_speed", "tremolo_depth", "filter_cutoff", "pan",
        NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ff|ifffffffffff", kwlist,
        &freq, &duration, &wave_type,
        &attack, &decay, &sustain, &release,
        &frequency_slide, &vibrato_speed, &vibrato_depth,
        &tremolo_speed, &tremolo_depth, &filter_cutoff, &pan)) {
        return NULL;
    }

    ADSREnvelope env = { attack, decay, sustain, release };
    PlaySynthToneEx(
        freq, duration, wave_type, env,
        frequency_slide, vibrato_speed, vibrato_depth,
        tremolo_speed, tremolo_depth, filter_cutoff, pan
    );

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
    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    if (node->visible) {
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
    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    if (node->visible) {
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

    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;

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
            ctx.layout_dirty = 1;
        }
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_get_style(PyObject* self, PyObject* args) {
    const char* id;
    const char* property_name;
    if (!PyArg_ParseTuple(args, "ss", &id, &property_name)) return NULL;

    UINode* node = GetNodeOrRaise(id);
    if (!node) return NULL;
    char val[128] = {0};
    GetStyleProperty(node, property_name, val, sizeof(val));
    return Py_BuildValue("s", val);
}

static PyObject* doodle_set_camera(PyObject* self, PyObject* args) {
    float tx, ty, ox, oy, zoom, rot;
    if (!PyArg_ParseTuple(args, "ffffff", &tx, &ty, &ox, &oy, &zoom, &rot)) return NULL;
    ctx.camera.target = (Vector2){tx, ty};
    ctx.camera.offset = (Vector2){ox, oy};
    ctx.camera.zoom = zoom;
    ctx.camera.rotation = rot;
    Py_RETURN_NONE;
}

static PyObject* doodle_shake_camera(PyObject* self, PyObject* args) {
    float intensity, duration;
    if (!PyArg_ParseTuple(args, "ff", &intensity, &duration)) return NULL;
    ctx.shake_intensity = intensity;
    ctx.shake_duration = duration;
    Py_RETURN_NONE;
}



// Developer Tools Variables and Functions
static PyObject* console_callback = NULL;
#define CONSOLE_MAX_LINES 20
#define CONSOLE_LINE_LENGTH 128
static char console_lines[CONSOLE_MAX_LINES][CONSOLE_LINE_LENGTH];
static int console_lines_count = 0;
static char console_input_buf[CONSOLE_LINE_LENGTH] = {0};
static int console_input_len = 0;

static int CountNodes(UINode* n) {
    if (!n) return 0;
    int count = 1;
    for (int i = 0; i < n->child_count; i++) {
        count += CountNodes(n->children[i]);
    }
    return count;
}

static void ConsolePrint(const char* text) {
    const char* p = text;
    char line[CONSOLE_LINE_LENGTH];
    int li = 0;
    while (*p) {
        if (*p == '\n' || li >= CONSOLE_LINE_LENGTH - 1) {
            line[li] = '\0';
            if (console_lines_count < CONSOLE_MAX_LINES) {
                snprintf(console_lines[console_lines_count++], CONSOLE_LINE_LENGTH, "%s", line);
            } else {
                for (int i = 1; i < CONSOLE_MAX_LINES; i++) {
                    strcpy(console_lines[i-1], console_lines[i]);
                }
                snprintf(console_lines[CONSOLE_MAX_LINES - 1], CONSOLE_LINE_LENGTH, "%s", line);
            }
            li = 0;
            if (*p == '\n') p++;
        } else {
            line[li++] = *p++;
        }
    }
    if (li > 0) {
        line[li] = '\0';
        if (console_lines_count < CONSOLE_MAX_LINES) {
            snprintf(console_lines[console_lines_count++], CONSOLE_LINE_LENGTH, "%s", line);
        } else {
            for (int i = 1; i < CONSOLE_MAX_LINES; i++) {
                strcpy(console_lines[i-1], console_lines[i]);
            }
            snprintf(console_lines[CONSOLE_MAX_LINES - 1], CONSOLE_LINE_LENGTH, "%s", line);
        }
    }
}

static void ExecuteConsoleInput(void) {
    if (console_input_len == 0) return;
    
    char cmd_echo[CONSOLE_LINE_LENGTH + 4];
    sprintf(cmd_echo, ">>> %s", console_input_buf);
    ConsolePrint(cmd_echo);
    
    if (console_callback) {
        PyObject* arglist = Py_BuildValue("(s)", console_input_buf);
        PyObject* result = PyObject_CallObject(console_callback, arglist);
        Py_DECREF(arglist);
        if (result) {
            if (PyUnicode_Check(result)) {
                const char* output = PyUnicode_AsUTF8(result);
                if (output && strlen(output) > 0) {
                    ConsolePrint(output);
                }
            } else if (result != Py_None) {
                PyObject* str_res = PyObject_Str(result);
                if (str_res) {
                    const char* output = PyUnicode_AsUTF8(str_res);
                    if (output && strlen(output) > 0) {
                        ConsolePrint(output);
                    }
                    Py_DECREF(str_res);
                }
            }
            Py_DECREF(result);
        } else {
            PyObject* ptype, *pvalue, *ptraceback;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);
            if (pvalue) {
                PyObject* str_val = PyObject_Str(pvalue);
                if (str_val) {
                    const char* err_str = PyUnicode_AsUTF8(str_val);
                    char err_msg[256];
                    sprintf(err_msg, "Error: %s", err_str);
                    ConsolePrint(err_msg);
                    Py_DECREF(str_val);
                }
                Py_DECREF(pvalue);
            }
            Py_XDECREF(ptype);
            Py_XDECREF(ptraceback);
        }
    } else {
        ConsolePrint("Error: Console callback not registered.");
    }
    
    memset(console_input_buf, 0, sizeof(console_input_buf));
    console_input_len = 0;
}

static void UpdateConsoleInput(void) {
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (console_input_len < CONSOLE_LINE_LENGTH - 1)) {
            console_input_buf[console_input_len++] = (char)key;
            console_input_buf[console_input_len] = '\0';
        }
        key = GetCharPressed();
    }
    
    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (console_input_len > 0) {
            console_input_len--;
            console_input_buf[console_input_len] = '\0';
        }
    }
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        ExecuteConsoleInput();
    }
}

static void DrawDeveloperTools(int width, int height, double cpu_time_ms) {
    if (IsKeyPressed(KEY_F3)) {
        ctx.dev_tools_active = !ctx.dev_tools_active;
    }
    if (IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_GRAVE)) {
        ctx.console_active = !ctx.console_active;
        if (ctx.console_active) ctx.dev_tools_active = 1;
    }

    if (ctx.console_active) {
        UpdateConsoleInput();
    }

    if (ctx.dev_tools_active) {
        int p_width = 220;
        int p_height = 135;
        int p_x = width - p_width - 15;
        int p_y = 15;

        DrawRectangleRounded((Rectangle){ (float)p_x, (float)p_y, (float)p_width, (float)p_height }, 0.08f, 8, (Color){ 15, 23, 42, 220 });
        DrawRectangleRoundedLines((Rectangle){ (float)p_x, (float)p_y, (float)p_width, (float)p_height }, 0.08f, 8, 1.5f, (Color){ 56, 189, 248, 100 });

        DrawText("DOODLE PROFILER", p_x + 12, p_y + 10, 12, (Color){ 56, 189, 248, 255 });
        DrawLine(p_x + 12, p_y + 25, p_x + p_width - 12, p_y + 25, (Color){ 56, 189, 248, 50 });

        char buf[64];
        int fps = GetFPS();
        float frame_time = GetFrameTime() * 1000.0f;
        sprintf(buf, "FPS: %d (%.1f ms)", fps, frame_time);
        DrawText(buf, p_x + 12, p_y + 32, 11, WHITE);

        sprintf(buf, "CPU: %.2f ms", cpu_time_ms);
        DrawText(buf, p_x + 12, p_y + 48, 11, (Color){ 0, 255, 204, 255 });

        sprintf(buf, "Draw Calls: %d", ctx.g_draw_calls);
        DrawText(buf, p_x + 12, p_y + 64, 11, WHITE);

        double ram = GetProcessMemoryUsage();
        if (ram > 0.0) {
            sprintf(buf, "RAM: %.1f MB", ram);
        } else {
            sprintf(buf, "RAM: N/A");
        }
        DrawText(buf, p_x + 12, p_y + 80, 11, WHITE);

        int node_count = CountNodes(ctx.root);
        int p_count = GetActiveParticleCount();
        sprintf(buf, "Nodes: %d | Part: %d", node_count, p_count);
        DrawText(buf, p_x + 12, p_y + 96, 11, WHITE);

        static float frame_times[60] = {0};
        static int ft_index = 0;
        frame_times[ft_index] = frame_time;
        ft_index = (ft_index + 1) % 60;

        int graph_x = p_x + 12;
        int graph_y = p_y + 125;
        int graph_w = p_width - 24;
        int graph_h = 12;

        DrawRectangle(graph_x, graph_y - graph_h, graph_w, graph_h, (Color){ 10, 15, 28, 100 });
        for (int i = 0; i < 59; i++) {
            int idx1 = (ft_index + i) % 60;
            int idx2 = (ft_index + i + 1) % 60;
            
            float val1 = frame_times[idx1];
            float val2 = frame_times[idx2];
            if (val1 > 33.3f) val1 = 33.3f;
            if (val2 > 33.3f) val2 = 33.3f;

            float h1 = (val1 / 33.3f) * (float)graph_h;
            float h2 = (val2 / 33.3f) * (float)graph_h;

            float x1 = (float)graph_x + ((float)i / 59.0f) * (float)graph_w;
            float x2 = (float)graph_x + ((float)(i + 1) / 59.0f) * (float)graph_w;

            DrawLineEx(
                (Vector2){ x1, (float)graph_y - h1 },
                (Vector2){ x2, (float)graph_y - h2 },
                1.0f,
                (val1 > 18.0f) ? RED : (Color){ 0, 255, 204, 180 }
            );
        }
    }

    if (ctx.console_active) {
        int c_height = 180;
        int c_y = height - c_height;

        DrawRectangle(0, c_y, width, c_height, (Color){ 10, 15, 28, 240 });
        DrawLineEx((Vector2){ 0, (float)c_y }, (Vector2){ (float)width, (float)c_y }, 2.0f, (Color){ 0, 255, 204, 255 });

        DrawText("DOODLE PYTHON CONSOLE (Press F1 or ` to close)", 15, c_y + 8, 10, (Color){ 56, 189, 248, 150 });

        int start_idx = console_lines_count - 7;
        if (start_idx < 0) start_idx = 0;
        int draw_y = c_y + 24;
        for (int i = start_idx; i < console_lines_count; i++) {
            Color c = WHITE;
            if (strncmp(console_lines[i], ">>>", 3) == 0) {
                c = (Color){ 0, 255, 204, 255 };
            } else if (strncmp(console_lines[i], "Error", 5) == 0) {
                c = RED;
            } else {
                c = (Color){ 200, 200, 200, 255 };
            }
            DrawText(console_lines[i], 15, draw_y, 12, c);
            draw_y += 18;
        }

        draw_y = height - 25;
        DrawRectangle(10, draw_y - 2, width - 20, 20, (Color){ 15, 23, 42, 255 });
        DrawRectangleLines(10, draw_y - 2, width - 20, 20, (Color){ 56, 189, 248, 100 });

        DrawText(">>> ", 18, draw_y + 2, 13, (Color){ 0, 255, 204, 255 });
        DrawText(console_input_buf, 48, draw_y + 2, 13, WHITE);

        if (((int)(GetTime() * 2.0) % 2) == 0) {
            int text_w = MeasureText(console_input_buf, 13);
            DrawText("|", 48 + text_w, draw_y + 1, 13, (Color){ 0, 255, 204, 255 });
        }
    }
}

static PyObject* doodle_register_console_callback(PyObject* self, PyObject* args) {
    PyObject* temp = NULL;
    if (!PyArg_ParseTuple(args, "O", &temp)) return NULL;
    if (!PyCallable_Check(temp)) {
        PyErr_SetString(PyExc_TypeError, "Parameter must be callable");
        return NULL;
    }
    Py_XINCREF(temp);
    Py_XDECREF(console_callback);
    console_callback = temp;
    Py_RETURN_NONE;
}

static PyObject* doodle_is_key_down(PyObject* self, PyObject* args) {
    int key;
    if (!PyArg_ParseTuple(args, "i", &key)) return NULL;
    if (ctx.console_active) {
        Py_RETURN_FALSE;
    }
    if (IsKeyDown(key)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_is_key_pressed(PyObject* self, PyObject* args) {
    int key;
    if (!PyArg_ParseTuple(args, "i", &key)) return NULL;
    if (ctx.console_active) {
        Py_RETURN_FALSE;
    }
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
        snprintf(saved_positions[saved_positions_count].id, 64, "%s", n->id);
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

static PyObject* doodle_batch_process(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* positions_dict = NULL;
    PyObject* text_dict = NULL;
    PyObject* visibility_dict = NULL;
    PyObject* collisions_list = NULL;

    static char* kwlist[] = {"positions", "text_updates", "visibility_updates", "collisions", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOOO", kwlist,
                                     &positions_dict, &text_dict, &visibility_dict, &collisions_list)) {
        return NULL;
    }

    // 1. Process positions
    if (positions_dict && positions_dict != Py_None && PyDict_Check(positions_dict)) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(positions_dict, &pos, &key, &value)) {
            const char* id = PyUnicode_AsUTF8(key);
            if (id && (PyTuple_Check(value) || PyList_Check(value)) && PySequence_Size(value) >= 2) {
                PyObject* x_obj = PySequence_GetItem(value, 0);
                PyObject* y_obj = PySequence_GetItem(value, 1);
                float x = (float)PyFloat_AsDouble(x_obj);
                float y = (float)PyFloat_AsDouble(y_obj);
                Py_XDECREF(x_obj);
                Py_XDECREF(y_obj);
                UINode* node = GetNodeOrRaise(id);
                if (!node) return NULL;
                node->style.left = x;
                node->style.top = y;
                ctx.layout_dirty = 1;
            }
        }
    }

    // 2. Process text updates
    if (text_dict && text_dict != Py_None && PyDict_Check(text_dict)) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(text_dict, &pos, &key, &value)) {
            const char* id = PyUnicode_AsUTF8(key);
            const char* text = PyUnicode_AsUTF8(value);
            if (id && text) {
                UINode* node = GetNodeOrRaise(id);
                if (!node) return NULL;
                if (strcmp(node->text_content, text) != 0) {
                    node->text_content = DOMStrDup(text);
                    ctx.layout_dirty = 1;
                }
            }
        }
    }

    // 3. Process visibility updates
    if (visibility_dict && visibility_dict != Py_None && PyDict_Check(visibility_dict)) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(visibility_dict, &pos, &key, &value)) {
            const char* id = PyUnicode_AsUTF8(key);
            if (id) {
                int visible = PyObject_IsTrue(value);
                UINode* node = GetNodeOrRaise(id);
                if (!node) return NULL;
                if (node->visible != visible) {
                    node->visible = visible;
                    ctx.layout_dirty = 1;
                }
            }
        }
    }

    if (ctx.layout_dirty) {
        MeasureNode(ctx.root);
        ctx.root->layout.width = (float)GetRenderWidth();
        ctx.root->layout.height = (float)GetRenderHeight();
        ctx.root->layout.x = 0;
        ctx.root->layout.y = 0;
        LayoutNode(ctx.root, (float)GetRenderWidth(), (float)GetRenderHeight());
        ctx.layout_dirty = 0;
    }

    // 4. Process collisions
    PyObject* collision_results = PyDict_New();
    if (collisions_list && collisions_list != Py_None && PyList_Check(collisions_list)) {
        Py_ssize_t len = PyList_Size(collisions_list);
        for (Py_ssize_t i = 0; i < len; i++) {
            PyObject* item = PyList_GetItem(collisions_list, i);
            if (PyTuple_Check(item) && PyTuple_Size(item) >= 2) {
                PyObject* key_a = PyTuple_GetItem(item, 0);
                PyObject* key_b = PyTuple_GetItem(item, 1);
                const char* id_a = PyUnicode_AsUTF8(key_a);
                const char* id_b_or_group = PyUnicode_AsUTF8(key_b);

                if (id_a && id_b_or_group) {
                    UINode* node_a = GetNodeOrRaise(id_a);
                    if (!node_a) {
                        Py_DECREF(collision_results);
                        return NULL;
                    }
                    if (node_a->visible) {
                        UINode* node_b = FindNodeById(ctx.root, id_b_or_group);
                        if (node_b) {
                            if (node_b->visible) {
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
                                PyDict_SetItem(collision_results, item, col ? Py_True : Py_False);
                            } else {
                                PyDict_SetItem(collision_results, item, Py_False);
                            }
                        } else {
                            Rectangle rec = {node_a->layout.x, node_a->layout.y, node_a->layout.width, node_a->layout.height};
                            UINode* collision = FindCollisionNode(ctx.root, node_a, id_b_or_group, rec);
                            if (collision) {
                                PyObject* col_id = PyUnicode_FromString(collision->id);
                                PyDict_SetItem(collision_results, item, col_id);
                                Py_DECREF(col_id);
                            } else {
                                PyDict_SetItem(collision_results, item, Py_None);
                            }
                        }
                    } else {
                        PyDict_SetItem(collision_results, item, Py_False);
                    }
                }
            }
        }
    }

    return collision_results;
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
    
    int target_fps = 60;
    PyObject* sys_module = PyImport_ImportModule("sys");
    if (sys_module) {
        PyObject* argv = PyObject_GetAttrString(sys_module, "argv");
        if (argv && PyList_Check(argv)) {
            Py_ssize_t size = PyList_Size(argv);
            for (Py_ssize_t i = 0; i < size; i++) {
                PyObject* item = PyList_GetItem(argv, i);
                if (item && PyUnicode_Check(item)) {
                    const char* arg = PyUnicode_AsUTF8(item);
                    if (arg && strcmp(arg, "--benchmark") == 0) {
                        target_fps = 0;
                        break;
                    }
                }
            }
        }
        Py_XDECREF(argv);
        Py_DECREF(sys_module);
    }
    SetTargetFPS(target_fps);

    ctx.root = ParseHTML(layout);
    if (ctx.root) {
        LoadCSS(style);
        ApplyStyleSheetToTree(ctx.root);
        SetupAudioNodes(ctx.root);
        MeasureNode(ctx.root);
        ctx.root->layout.width = (float)width;
        ctx.root->layout.height = (float)height;
        ctx.root->layout.x = 0;
        ctx.root->layout.y = 0;
        LayoutNode(ctx.root, (float)width, (float)height);
    }
    ctx.layout_dirty = 0;

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
            SavePositions(ctx.root);

            FreeNode(ctx.root);
            UnloadActiveMusics();
            UnloadCachedTextures();
            UnloadCachedSounds();
            UnloadCachedShaders();
            UnloadCachedFonts();

            ctx.root = ParseHTML(layout);
            if (ctx.root) {
                LoadCSS(style);
        ApplyStyleSheetToTree(ctx.root);
                RestorePositions(ctx.root);
                SetupAudioNodes(ctx.root);
                MeasureNode(ctx.root);
        ctx.root->layout.width = (float)width;
        ctx.root->layout.height = (float)height;
        ctx.root->layout.x = 0;
        ctx.root->layout.y = 0;
        LayoutNode(ctx.root, (float)width, (float)height);
            }
            ctx.layout_dirty = 0;
        }

        if (ctx.root && UpdateHoverStates(ctx.root)) {
            ctx.layout_dirty = 1;
        }

        double tick_start = GetTime();
        if (tick_callback) {
            PyObject* result = PyObject_CallObject(tick_callback, NULL);
            if (!result) {
                PyErr_Print();
            } else {
                Py_DECREF(result);
            }
        }
        double tick_end = GetTime();
        double cpu_time_ms = (tick_end - tick_start) * 1000.0;

        if (ctx.layout_dirty && ctx.root) {
            MeasureNode(ctx.root);
        ctx.root->layout.width = (float)width;
        ctx.root->layout.height = (float)height;
        ctx.root->layout.x = 0;
        ctx.root->layout.y = 0;
        LayoutNode(ctx.root, (float)width, (float)height);
            ctx.layout_dirty = 0;
        }

        UpdateMusicStreams();
        CheckShaderUpdates();

        BeginDrawing();
        ClearBackground(BLACK);
        
        if (ctx.root) {
            ctx.g_draw_calls = 0;
            DrawUINode(ctx.root);
        }
        
        UpdateAndDrawParticles();
        
        DrawDeveloperTools(width, height, cpu_time_ms);
        
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
    
    if (ctx.root) {
        FreeNode(ctx.root);
        ctx.root = NULL;
    }
    CleanupDOM();

    Py_RETURN_NONE;
}

static PyMethodDef DoodleMethods[] = {
    {"register_tick_callback", doodle_register_tick_callback, METH_VARARGS, "Register update callback"},
    {"registerTickCallback", doodle_register_tick_callback, METH_VARARGS, "Register update callback"},
    
    {"check_collision", doodle_check_collision, METH_VARARGS, "Check collision between two IDs"},
    {"checkCollision", doodle_check_collision, METH_VARARGS, "Check collision between two IDs"},
    
    {"get_first_collision", (PyCFunction)doodle_get_first_collision, METH_VARARGS | METH_KEYWORDS, "Get first collision in group"},
    {"getFirstCollision", (PyCFunction)doodle_get_first_collision, METH_VARARGS | METH_KEYWORDS, "Get first collision in group"},
    
    {"set_position", doodle_set_position, METH_VARARGS, "Set absolute layout pos"},
    {"setPosition", doodle_set_position, METH_VARARGS, "Set absolute layout pos"},
    
    {"get_position", doodle_get_position, METH_VARARGS, "Get layout pos"},
    {"getPosition", doodle_get_position, METH_VARARGS, "Get layout pos"},
    
    {"remove_node", doodle_remove_node, METH_VARARGS, "Remove node from DOM tree"},
    {"removeNode", doodle_remove_node, METH_VARARGS, "Remove node from DOM tree"},
    
    {"update_text", doodle_update_text, METH_VARARGS, "Update text content of node"},
    {"updateText", doodle_update_text, METH_VARARGS, "Update text content of node"},
    
    {"show_node", doodle_show_node, METH_VARARGS, "Show hidden node"},
    {"showNode", doodle_show_node, METH_VARARGS, "Show hidden node"},
    
    {"hide_node", doodle_hide_node, METH_VARARGS, "Hide node"},
    {"hideNode", doodle_hide_node, METH_VARARGS, "Hide node"},
    
    {"play_sound", doodle_play_sound, METH_VARARGS, "Play sound"},
    {"playSound", doodle_play_sound, METH_VARARGS, "Play sound"},
    
    {"is_key_down", doodle_is_key_down, METH_VARARGS, "Check if key is down"},
    {"isKeyDown", doodle_is_key_down, METH_VARARGS, "Check if key is down"},
    
    {"is_key_pressed", doodle_is_key_pressed, METH_VARARGS, "Check if key is pressed"},
    {"isKeyPressed", doodle_is_key_pressed, METH_VARARGS, "Check if key is pressed"},
    
    {"get_mouse_x", doodle_get_mouse_x, METH_NOARGS, "Get mouse X coordinate"},
    {"getMouseX", doodle_get_mouse_x, METH_NOARGS, "Get mouse X coordinate"},
    
    {"get_mouse_y", doodle_get_mouse_y, METH_NOARGS, "Get mouse Y coordinate"},
    {"getMouseY", doodle_get_mouse_y, METH_NOARGS, "Get mouse Y coordinate"},
    
    {"get_mouse_position", doodle_get_mouse_position, METH_NOARGS, "Get mouse position"},
    {"getMousePosition", doodle_get_mouse_position, METH_NOARGS, "Get mouse position"},
    
    {"is_mouse_button_down", doodle_is_mouse_button_down, METH_VARARGS, "Check if mouse button is down"},
    {"isMouseButtonDown", doodle_is_mouse_button_down, METH_VARARGS, "Check if mouse button is down"},
    
    {"is_mouse_button_pressed", doodle_is_mouse_button_pressed, METH_VARARGS, "Check if mouse button is pressed"},
    {"isMouseButtonPressed", doodle_is_mouse_button_pressed, METH_VARARGS, "Check if mouse button is pressed"},
    
    {"get_mouse_wheel_move", doodle_get_mouse_wheel_move, METH_NOARGS, "Get mouse wheel movement"},
    {"getMouseWheelMove", doodle_get_mouse_wheel_move, METH_NOARGS, "Get mouse wheel movement"},
    
    {"set_mouse_cursor", doodle_set_mouse_cursor, METH_VARARGS, "Set mouse cursor type"},
    {"setMouseCursor", doodle_set_mouse_cursor, METH_VARARGS, "Set mouse cursor type"},
    
    {"set_window_title", doodle_set_window_title, METH_VARARGS, "Set window title"},
    {"setWindowTitle", doodle_set_window_title, METH_VARARGS, "Set window title"},
    
    {"toggle_fullscreen", doodle_toggle_fullscreen, METH_NOARGS, "Toggle fullscreen mode"},
    {"toggleFullscreen", doodle_toggle_fullscreen, METH_NOARGS, "Toggle fullscreen mode"},
    
    {"get_screen_size", doodle_get_screen_size, METH_NOARGS, "Get current screen size"},
    {"getScreenSize", doodle_get_screen_size, METH_NOARGS, "Get current screen size"},
    
    {"is_node_hovered", doodle_is_node_hovered, METH_VARARGS, "Check if node is hovered"},
    {"isNodeHovered", doodle_is_node_hovered, METH_VARARGS, "Check if node is hovered"},
    
    {"is_node_clicked", doodle_is_node_clicked, METH_VARARGS, "Check if node is clicked"},
    {"isNodeClicked", doodle_is_node_clicked, METH_VARARGS, "Check if node is clicked"},
    
    {"set_style", doodle_set_style, METH_VARARGS, "Set a style property dynamically"},
    {"setStyle", doodle_set_style, METH_VARARGS, "Set a style property dynamically"},
    
    {"get_style", doodle_get_style, METH_VARARGS, "Get a style property dynamically"},
    {"getStyle", doodle_get_style, METH_VARARGS, "Get a style property dynamically"},
    
    {"set_camera", doodle_set_camera, METH_VARARGS, "Set 2D camera parameters"},
    {"setCamera", doodle_set_camera, METH_VARARGS, "Set 2D camera parameters"},
    
    {"shake_camera", doodle_shake_camera, METH_VARARGS, "Trigger camera screen shake"},
    {"shakeCamera", doodle_shake_camera, METH_VARARGS, "Trigger camera screen shake"},
    
    {"spawn_particles", doodle_spawn_particles, METH_VARARGS, "Spawn particle explosion burst"},
    {"spawnParticles", doodle_spawn_particles, METH_VARARGS, "Spawn particle explosion burst"},
    
    {"play_synth", (PyCFunction)doodle_play_synth, METH_VARARGS | METH_KEYWORDS, "Play procedurally synthesized tone"},
    {"playSynth", (PyCFunction)doodle_play_synth, METH_VARARGS | METH_KEYWORDS, "Play procedurally synthesized tone"},
    
    {"register_console_callback", doodle_register_console_callback, METH_VARARGS, "Register console command callback"},
    {"registerConsoleCallback", doodle_register_console_callback, METH_VARARGS, "Register console command callback"},
    
    {"batch_process", (PyCFunction)doodle_batch_process, METH_VARARGS | METH_KEYWORDS, "Batch process updates and collisions"},
    {"batchProcess", (PyCFunction)doodle_batch_process, METH_VARARGS | METH_KEYWORDS, "Batch process updates and collisions"},
    
    {"get_line_number", doodle_get_line_number, METH_VARARGS, "Get HTML line number of node"},
    {"getLineNumber", doodle_get_line_number, METH_VARARGS, "Get HTML line number of node"},
    
    {"get_layout_size", doodle_get_layout_size, METH_VARARGS, "Get calculated width and height of node"},
    {"getLayoutSize", doodle_get_layout_size, METH_VARARGS, "Get calculated width and height of node"},
    
    {"is_visible", doodle_is_visible, METH_VARARGS, "Check if node is visible"},
    {"isVisible", doodle_is_visible, METH_VARARGS, "Check if node is visible"},
    
    {"get_text", doodle_get_text, METH_VARARGS, "Get text content of node"},
    {"getText", doodle_get_text, METH_VARARGS, "Get text content of node"},
    
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
