#include "cache.h"
#include "dom_utils.h"
#include <string.h>
#include <stdio.h>

// Caching Systems
typedef struct {
    char path[256];
    Texture2D texture;
} CachedTexture;

static CachedTexture texture_cache[128];
static int texture_cache_count = 0;

Texture2D GetCachedTexture(const char* path) {
    for (int i = 0; i < texture_cache_count; i++) {
        if (strcmp(texture_cache[i].path, path) == 0) {
            return texture_cache[i].texture;
        }
    }
    Texture2D tex = LoadTexture(path);
    if (texture_cache_count < 128) {
        strncpy(texture_cache[texture_cache_count].path, path, 255);
        texture_cache[texture_cache_count].path[255] = '\0';
        texture_cache[texture_cache_count].texture = tex;
        texture_cache_count++;
    }
    return tex;
}

void UnloadCachedTextures(void) {
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

Shader GetCachedShader(const char* path) {
    for (int i = 0; i < shader_cache_count; i++) {
        if (strcmp(shader_cache[i].path, path) == 0) {
            return shader_cache[i].shader;
        }
    }
    Shader sh = LoadShader(NULL, path);
    if (shader_cache_count < 32) {
        strncpy(shader_cache[shader_cache_count].path, path, 255);
        shader_cache[shader_cache_count].path[255] = '\0';
        shader_cache[shader_cache_count].shader = sh;
        shader_cache_count++;
    }
    return sh;
}

void UnloadCachedShaders(void) {
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

Font GetCachedFont(const char* path) {
    for (int i = 0; i < font_cache_count; i++) {
        if (strcmp(font_cache[i].path, path) == 0) {
            return font_cache[i].font;
        }
    }
    Font fnt = LoadFont(path);
    if (font_cache_count < 32) {
        strncpy(font_cache[font_cache_count].path, path, 255);
        font_cache[font_cache_count].path[255] = '\0';
        font_cache[font_cache_count].font = fnt;
        font_cache_count++;
    }
    return fnt;
}

void UnloadCachedFonts(void) {
    for (int i = 0; i < font_cache_count; i++) {
        UnloadFont(font_cache[i].font);
    }
    font_cache_count = 0;
}

// Sound Cache
typedef struct {
    char path[256];
    Sound sound;
} CachedSound;

static CachedSound sound_cache[128];
static int sound_cache_count = 0;

Sound GetCachedSound(const char* path) {
    for (int i = 0; i < sound_cache_count; i++) {
        if (strcmp(sound_cache[i].path, path) == 0) {
            return sound_cache[i].sound;
        }
    }
    Sound snd = LoadSound(path);
    if (sound_cache_count < 128) {
        strncpy(sound_cache[sound_cache_count].path, path, 255);
        sound_cache[sound_cache_count].path[255] = '\0';
        sound_cache[sound_cache_count].sound = snd;
        sound_cache_count++;
    }
    return snd;
}

void UnloadCachedSounds(void) {
    for (int i = 0; i < sound_cache_count; i++) {
        UnloadSound(sound_cache[i].sound);
    }
    sound_cache_count = 0;
}

// Music Cache
typedef struct {
    char path[256];
    Music music;
    int is_playing;
} ActiveMusic;

static ActiveMusic active_musics[16];
static int active_music_count = 0;

void SetupAudioNodes(UINode* node) {
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
            snprintf(active_musics[active_music_count].path, 256, "%s", node->asset_path);
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

void UpdateMusicStreams(void) {
    for (int i = 0; i < active_music_count; i++) {
        if (active_musics[i].is_playing) {
            UpdateMusicStream(active_musics[i].music);
        }
    }
}

void UnloadActiveMusics(void) {
    for (int i = 0; i < active_music_count; i++) {
        StopMusicStream(active_musics[i].music);
        UnloadMusicStream(active_musics[i].music);
    }
    active_music_count = 0;
}
