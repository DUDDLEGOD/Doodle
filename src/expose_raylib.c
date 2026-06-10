#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "raylib.h"
#include "mparser.h"
#include <sys/stat.h>

static UINode* root = NULL;

// Mutation flag to ensure we only ComputeLayout when needed
static int layout_dirty = 1;

static PyObject* tick_callback = NULL;

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

    if (!node_a || !node_b) Py_RETURN_FALSE;

    Rectangle rec_a = {node_a->layout.x, node_a->layout.y, node_a->layout.width, node_a->layout.height};
    Rectangle rec_b = {node_b->layout.x, node_b->layout.y, node_b->layout.width, node_b->layout.height};

    if (CheckCollisionRecs(rec_a, rec_b)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* doodle_set_position(PyObject* self, PyObject* args) {
    const char* id;
    float x, y;
    if (!PyArg_ParseTuple(args, "sff", &id, &x, &y)) return NULL;

    UINode* node = FindNodeById(root, id);
    if (node) {
        node->layout.x = x;
        node->layout.y = y;
        // Mutated, so layout might need recalculation if it affects children
        // but for absolute positioning like this, we just change it.
        // If we want a strict "Calculate on mutation", we set layout_dirty = 1;
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

static PyObject* doodle_play_sound(PyObject* self, PyObject* args) {
    const char* sound_path;
    if (!PyArg_ParseTuple(args, "s", &sound_path)) return NULL;
    // Mock play sound to not crash if raylib not initialized properly
    // PlaySound(LoadSound(sound_path));
    Py_RETURN_NONE;
}

void DrawUINode(UINode* node) {
    if (!node) return;

    // Read on render: Use node->layout directly! No heavy layout math here.
    if (node->type == NODE_VIEW) {
        if (node->style.border_radius > 0) {
            Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
            DrawRectangleRounded(rec, 0.2f, 8, node->style.bg_color);
        } else {
            DrawRectangle(node->layout.x, node->layout.y, node->layout.width, node->layout.height, node->style.bg_color);
        }
    }

    // Recursively draw children
    for (int i = 0; i < node->child_count; i++) {
        DrawUINode(node->children[i]);
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
    SetTargetFPS(60);

    // Mock HTML parser step
    root = CreateNode(NODE_VIEW);
    root->style.bg_color = (Color){20, 20, 20, 255};
    root->style.width = width;
    root->style.height = height;

    layout_dirty = 1;

    while (!WindowShouldClose()) {
        if (tick_callback) {
            PyObject* result = PyObject_CallObject(tick_callback, NULL);
            if (!result) {
                PyErr_Print();
            } else {
                Py_DECREF(result);
            }
        }

        // Calculate on mutation
        if (layout_dirty) {
            ComputeLayout(root, 0, 0, width, height);
            layout_dirty = 0;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        
        // Read on render
        DrawUINode(root);
        
        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();
    
    // Bottom-up recursion free
    FreeNode(root);
    root = NULL;

    Py_RETURN_NONE;
}

static PyMethodDef DoodleMethods[] = {
    {"register_tick_callback", doodle_register_tick_callback, METH_VARARGS, "Register update callback"},
    {"check_collision", doodle_check_collision, METH_VARARGS, "Check collision between two IDs"},
    {"set_position", doodle_set_position, METH_VARARGS, "Set absolute layout pos"},
    {"get_position", doodle_get_position, METH_VARARGS, "Get layout pos"},
    {"play_sound", doodle_play_sound, METH_VARARGS, "Play sound"},
    {"run", (PyCFunction)doodle_run, METH_VARARGS | METH_KEYWORDS, "Start the engine loop"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef doodlemodule = {
    PyModuleDef_HEAD_INIT,
    "doodle",
    "Hardware-accelerated DOM-based UI & 2D Game Engine",
    -1,
    DoodleMethods
};

PyMODINIT_FUNC PyInit_doodle(void) {
    return PyModule_Create(&doodlemodule);
}
