#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "raylib.h"
#include "mparser.h"
#include "dutils.h"
#include "daudio.h"
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#ifndef DEG2RAD
#define DEG2RAD (PI/180.0f)
#endif

static UINode* root = NULL;
static int layout_dirty = 1;
static PyObject* tick_callback = NULL;

// Camera state
static Camera2D camera = { .zoom = 1.0f };
static float shake_intensity = 0.0f;
static float shake_duration = 0.0f;

// Particle System
typedef struct {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float size;
    float lifetime;
    float max_lifetime;
} Particle;

#define MAX_PARTICLES 2048
static Particle particle_pool[MAX_PARTICLES];
static int particle_count = 0;
static int particles_drawn_this_frame = 0;

static PyObject* doodle_spawn_particles(PyObject* self, PyObject* args) {
    float x, y;
    int count;
    const char* color_hex;
    float speed;
    float lifetime;
    if (!PyArg_ParseTuple(args, "ffisff", &x, &y, &count, &color_hex, &speed, &lifetime)) return NULL;
    
    Color col = ParseColor(color_hex);
    
    for (int i = 0; i < count; i++) {
        int idx = -1;
        if (particle_count < MAX_PARTICLES) {
            idx = particle_count++;
        } else {
            idx = GetRandomValue(0, MAX_PARTICLES - 1);
        }
        
        if (idx != -1) {
            particle_pool[idx].position = (Vector2){x, y};
            float angle = GetRandomValue(0, 360) * DEG2RAD;
            float current_speed = GetRandomValue(20, 100) * 0.01f * speed;
            particle_pool[idx].velocity = (Vector2){ cosf(angle) * current_speed, sinf(angle) * current_speed };
            particle_pool[idx].color = col;
            particle_pool[idx].size = (float)GetRandomValue(2, 6);
            particle_pool[idx].lifetime = lifetime;
            particle_pool[idx].max_lifetime = lifetime;
        }
    }
    Py_RETURN_NONE;
}

static void UpdateAndDrawParticles(void) {
    if (particles_drawn_this_frame) return;
    particles_drawn_this_frame = 1;
    float dt = GetFrameTime();
    int active_idx = 0;
    for (int i = 0; i < particle_count; i++) {
        particle_pool[i].lifetime -= dt;
        if (particle_pool[i].lifetime > 0.0f) {
            particle_pool[i].position.x += particle_pool[i].velocity.x * dt * 60.0f;
            particle_pool[i].position.y += particle_pool[i].velocity.y * dt * 60.0f;
            
            Color c = particle_pool[i].color;
            c.a = (unsigned char)(255.0f * (particle_pool[i].lifetime / particle_pool[i].max_lifetime));
            DrawRectangleV(particle_pool[i].position, (Vector2){particle_pool[i].size, particle_pool[i].size}, c);
            
            if (active_idx != i) {
                particle_pool[active_idx] = particle_pool[i];
            }
            active_idx++;
        }
    }
    particle_count = active_idx;
}

// Caching Systems
typedef struct {
    char path[256];
    Texture2D texture;
} CachedTexture;

static CachedTexture texture_cache[128];
static int texture_cache_count = 0;

static Texture2D GetCachedTexture(const char* path) {
    for (int i = 0; i < texture_cache_count; i++) {
        if (strcmp(texture_cache[i].path, path) == 0) {
            return texture_cache[i].texture;
        }
    }
    Texture2D tex = LoadTexture(path);
    if (texture_cache_count < 128) {
        strncpy(texture_cache[texture_cache_count].path, path, 255);
        texture_cache[texture_cache_count].texture = tex;
        texture_cache_count++;
    }
    return tex;
}

static void UnloadCachedTextures(void) {
    for (int i = 0; i < texture_cache_count; i++) {
        UnloadTexture(texture_cache[i].texture);
    }
    texture_cache_count = 0;
}

// Shader Cache
typedef struct {
    char path[256];
    Shader shader;
} CachedShader;

static CachedShader shader_cache[32];
static int shader_cache_count = 0;

static Shader GetCachedShader(const char* path) {
    for (int i = 0; i < shader_cache_count; i++) {
        if (strcmp(shader_cache[i].path, path) == 0) {
            return shader_cache[i].shader;
        }
    }
    Shader sh = LoadShader(NULL, path);
    if (shader_cache_count < 32) {
        strncpy(shader_cache[shader_cache_count].path, path, 255);
        shader_cache[shader_cache_count].shader = sh;
        shader_cache_count++;
    }
    return sh;
}

static void UnloadCachedShaders(void) {
    for (int i = 0; i < shader_cache_count; i++) {
        UnloadShader(shader_cache[i].shader);
    }
    shader_cache_count = 0;
}

// Font Cache
typedef struct {
    char path[256];
    Font font;
} CachedFont;

static CachedFont font_cache[32];
static int font_cache_count = 0;

static Font GetCachedFont(const char* path) {
    for (int i = 0; i < font_cache_count; i++) {
        if (strcmp(font_cache[i].path, path) == 0) {
            return font_cache[i].font;
        }
    }
    Font fnt = LoadFont(path);
    if (font_cache_count < 32) {
        strncpy(font_cache[font_cache_count].path, path, 255);
        font_cache[font_cache_count].font = fnt;
        font_cache_count++;
    }
    return fnt;
}

static void UnloadCachedFonts(void) {
    for (int i = 0; i < font_cache_count; i++) {
        UnloadFont(font_cache[i].font);
    }
    font_cache_count = 0;
}

typedef struct {
    char path[256];
    Sound sound;
} CachedSound;

static CachedSound sound_cache[128];
static int sound_cache_count = 0;

static Sound GetCachedSound(const char* path) {
    for (int i = 0; i < sound_cache_count; i++) {
        if (strcmp(sound_cache[i].path, path) == 0) {
            return sound_cache[i].sound;
        }
    }
    Sound snd = LoadSound(path);
    if (sound_cache_count < 128) {
        strncpy(sound_cache[sound_cache_count].path, path, 255);
        sound_cache[sound_cache_count].sound = snd;
        sound_cache_count++;
    }
    return snd;
}

static void UnloadCachedSounds(void) {
    for (int i = 0; i < sound_cache_count; i++) {
        UnloadSound(sound_cache[i].sound);
    }
    sound_cache_count = 0;
}

typedef struct {
    char path[256];
    Music music;
    int is_playing;
} ActiveMusic;

static ActiveMusic active_musics[16];
static int active_music_count = 0;

static void SetupAudioNodes(UINode* node) {
    if (!node) return;
    if (node->type == NODE_AUDIO && strlen(node->asset_path) > 0) {
        int found = -1;
        for (int i = 0; i < active_music_count; i++) {
            if (strcmp(active_musics[i].path, node->asset_path) == 0) {
                found = i;
                break;
            }
        }
        if (found == -1 && active_music_count < 16) {
            Music m = LoadMusicStream(node->asset_path);
            m.looping = node->loop;
            strncpy(active_musics[active_music_count].path, node->asset_path, 255);
            active_musics[active_music_count].music = m;
            active_musics[active_music_count].is_playing = 0;
            found = active_music_count;
            active_music_count++;
        }
        if (found != -1 && node->autoplay && !active_musics[found].is_playing) {
            PlayMusicStream(active_musics[found].music);
            active_musics[found].is_playing = 1;
        }
    }
    for (int i = 0; i < node->child_count; i++) {
        SetupAudioNodes(node->children[i]);
    }
}

static void UpdateMusicStreams(void) {
    for (int i = 0; i < active_music_count; i++) {
        if (active_musics[i].is_playing) {
            UpdateMusicStream(active_musics[i].music);
        }
    }
}

static void UnloadActiveMusics(void) {
    for (int i = 0; i < active_music_count; i++) {
        StopMusicStream(active_musics[i].music);
        UnloadMusicStream(active_musics[i].music);
    }
    active_music_count = 0;
}

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
            Rectangle rec = {current->layout.x, current->layout.y, current->layout.width, current->layout.height};
            if (CheckCollisionRecs(target_rec, rec)) {
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

    Rectangle rec_a = {node_a->layout.x, node_a->layout.y, node_a->layout.width, node_a->layout.height};
    Rectangle rec_b = {node_b->layout.x, node_b->layout.y, node_b->layout.width, node_b->layout.height};

    if (CheckCollisionRecs(rec_a, rec_b)) {
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
        strncpy(node->text_content, text, sizeof(node->text_content) - 1);
        node->text_content[sizeof(node->text_content) - 1] = '\0';
        layout_dirty = 1;
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

static PyObject* doodle_is_node_hovered(PyObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) return NULL;
    UINode* node = FindNodeById(root, id);
    if (node && node->visible) {
        Vector2 mouse = GetMousePosition();
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
        for (int i = 0; i < css_registry_count; i++) {
            if (strcmp(css_registry[i].property_name, property_name) == 0) {
                css_registry[i].handler(node, property_value);
                StyleProps temp = node->style;
                node->style = node->hover_style;
                css_registry[i].handler(node, property_value);
                node->hover_style = node->style;
                node->style = temp;
                applied = 1;
                break;
            }
        }
        if (applied) {
            layout_dirty = 1;
            Py_RETURN_TRUE;
        }
    }
    Py_RETURN_FALSE;
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

// Drawing Traversal
void DrawUINode(UINode* node) {
    if (!node || !node->visible) return;

    StyleProps* active_style = &node->style;
    if (node->has_hover_style && node->currently_hovered) {
        active_style = &node->hover_style;
    }

    if (node->use_camera) {
        static RenderTexture2D arena_target;
        static int arena_target_created = 0;
        if (!arena_target_created) {
            arena_target = LoadRenderTexture(800, 600);
            arena_target_created = 1;
        }

        BeginTextureMode(arena_target);
        ClearBackground(BLACK);

        if (active_style->bg_color.a > 0) {
            DrawRectangle(0, 0, arena_target.texture.width, arena_target.texture.height, active_style->bg_color);
        }

        Camera2D current_cam = camera;
        if (shake_duration > 0.0f) {
            current_cam.offset.x += GetRandomValue(-shake_intensity, shake_intensity);
            current_cam.offset.y += GetRandomValue(-shake_intensity, shake_intensity);
            shake_duration -= GetFrameTime();
        }
        BeginMode2D(current_cam);

        for (int i = 0; i < node->child_count; i++) {
            DrawUINode(node->children[i]);
        }

        UpdateAndDrawParticles();

        EndMode2D();
        EndTextureMode();

        int has_shader = (strlen(active_style->shader_path) > 0);
        if (has_shader) {
            Shader sh = GetCachedShader(active_style->shader_path);
            if (sh.id > 0) {
                BeginShaderMode(sh);
            }
        }

        Rectangle src = { node->layout.x, arena_target.texture.height - node->layout.y - node->layout.height, node->layout.width, -node->layout.height };
        Rectangle dest = { node->layout.x, node->layout.y, node->layout.width, node->layout.height };
        DrawTexturePro(arena_target.texture, src, dest, (Vector2){0,0}, 0.0f, WHITE);

        if (has_shader) {
            EndShaderMode();
        }

        if (active_style->border_width > 0 && active_style->border_color.a > 0) {
            Rectangle border_rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
            DrawRectangleLinesEx(border_rec, active_style->border_width, active_style->border_color);
        }

        return;
    }

    int has_shader = (strlen(active_style->shader_path) > 0);
    if (has_shader) {
        Shader sh = GetCachedShader(active_style->shader_path);
        if (sh.id > 0) {
            BeginShaderMode(sh);
        }
    }

    if (node->type == NODE_VIEW) {
        if (active_style->bg_color.a > 0) {
            if (active_style->border_radius > 0) {
                Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
                float min_dim = node->layout.width > node->layout.height ? node->layout.height : node->layout.width;
                float roundness = min_dim > 0 ? (active_style->border_radius / min_dim) : 0.0f;
                if (roundness > 1.0f) roundness = 1.0f;
                DrawRectangleRounded(rec, roundness, 8, active_style->bg_color);
            } else {
                DrawRectangle(node->layout.x, node->layout.y, node->layout.width, node->layout.height, active_style->bg_color);
            }
        }
    } else if (node->type == NODE_TEXT || node->type == NODE_BUTTON) {
        if (node->type == NODE_BUTTON && active_style->bg_color.a > 0) {
            if (active_style->border_radius > 0) {
                Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
                float min_dim = node->layout.width > node->layout.height ? node->layout.height : node->layout.width;
                float roundness = min_dim > 0 ? (active_style->border_radius / min_dim) : 0.0f;
                if (roundness > 1.0f) roundness = 1.0f;
                DrawRectangleRounded(rec, roundness, 8, active_style->bg_color);
            } else {
                DrawRectangle(node->layout.x, node->layout.y, node->layout.width, node->layout.height, active_style->bg_color);
            }
        }
        
        const char* text = node->text_content;
        if (strlen(text) > 0) {
            float font_size = active_style->font_size;
            Color text_color = active_style->text_color;
            Font font = GetFontDefault();
            int has_custom_font = 0;
            if (strlen(active_style->font_path) > 0) {
                font = GetCachedFont(active_style->font_path);
                if (font.texture.id > 0) has_custom_font = 1;
            }

            Vector2 text_size;
            if (has_custom_font) {
                text_size = MeasureTextEx(font, text, font_size, 1.0f);
            } else {
                text_size = (Vector2){ (float)MeasureText(text, (int)font_size), font_size };
            }

            float draw_x = node->layout.x;
            float draw_y = node->layout.y;

            if (strcmp(active_style->text_align, "center") == 0) {
                draw_x = node->layout.x + (node->layout.width - text_size.x) / 2.0f;
                draw_y = node->layout.y + (node->layout.height - text_size.y) / 2.0f;
            } else if (node->type == NODE_BUTTON) {
                draw_x = node->layout.x + 5;
                draw_y = node->layout.y + (node->layout.height - text_size.y) / 2.0f;
            }

            if (has_custom_font) {
                DrawTextEx(font, text, (Vector2){draw_x, draw_y}, font_size, 1.0f, text_color);
            } else {
                DrawText(text, (int)draw_x, (int)draw_y, (int)font_size, text_color);
            }
        }
    } else if (node->type == NODE_IMAGE) {
        if (strlen(node->asset_path) > 0) {
            Texture2D tex = GetCachedTexture(node->asset_path);
            if (tex.id > 0) {
                Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
                Rectangle dest = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
                DrawTexturePro(tex, src, dest, (Vector2){0,0}, 0.0f, WHITE);
            }
        }
    } else if (node->type == NODE_CIRCLE) {
        float r = node->radius;
        if (r <= 0) r = node->layout.width / 2.0f;
        DrawCircle(node->layout.x + r, node->layout.y + r, r, node->shape_color);
    } else if (node->type == NODE_LINE) {
        DrawLineEx((Vector2){node->layout.x, node->layout.y}, (Vector2){node->layout.x + node->x2, node->layout.y + node->y2}, node->thickness, node->shape_color);
    }

    // Border drawing
    if (active_style->border_width > 0 && active_style->border_color.a > 0) {
        Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
        if (active_style->border_radius > 0) {
            float min_dim = node->layout.width > node->layout.height ? node->layout.height : node->layout.width;
            float roundness = min_dim > 0 ? (active_style->border_radius / min_dim) : 0.0f;
            if (roundness > 1.0f) roundness = 1.0f;
            DrawRectangleRoundedLines(rec, roundness, 8, active_style->border_color);
        } else {
            DrawRectangleLinesEx(rec, active_style->border_width, active_style->border_color);
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        DrawUINode(node->children[i]);
    }


    if (has_shader) {
        EndShaderMode();
    }
}

static int UpdateHoverStates(UINode* node) {
    if (!node || !node->visible) return 0;
    int changed = 0;

    Vector2 mouse = GetMousePosition();
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
            typedef struct {
                char id[64];
                float x, y;
            } SavedPos;
            static SavedPos saved[512];
            int saved_count = 0;
            
            void SavePositions(UINode* n) {
                if (!n) return;
                if (n->position_set && saved_count < 512) {
                    strncpy(saved[saved_count].id, n->id, 63);
                    saved[saved_count].x = n->layout.x;
                    saved[saved_count].y = n->layout.y;
                    saved_count++;
                }
                for (int i = 0; i < n->child_count; i++) SavePositions(n->children[i]);
            }
            SavePositions(root);

            FreeNode(root);
            UnloadActiveMusics();

            root = ParseHTML(layout);
            if (root) {
                LoadAndApplyCSS(root, style);
                
                void RestorePositions(UINode* n) {
                    if (!n) return;
                    for (int i = 0; i < saved_count; i++) {
                        if (strcmp(n->id, saved[i].id) == 0) {
                            n->layout.x = saved[i].x;
                            n->layout.y = saved[i].y;
                            n->position_set = 1;
                            break;
                        }
                    }
                    for (int i = 0; i < n->child_count; i++) RestorePositions(n->children[i]);
                }
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
