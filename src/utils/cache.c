#include "cache.h"
#include "dom_utils.h"
#include "arena.h"
#include "string_utils.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>

// Static memory arena for all cache allocations
static MemoryArena cache_arena = {0};

static inline void EnsureCacheArenaInit(void) {
    if (!cache_arena.begin) {
        ArenaInit(&cache_arena, 1024 * 1024); // 1 MB region capacity
    }
}

// Forward declaration of TryFreeCacheArena
static inline void TryFreeCacheArena(void);

// Generic Cache Generator Macro
#define DEFINE_CACHE(type, name_suffix, plural_name, load_fn, unload_fn) \
    typedef struct { \
        const char* key; \
        uint64_t hash; \
        type resource; \
    } CacheEntry_##type; \
    typedef struct { \
        CacheEntry_##type** slots; \
        size_t capacity; \
        size_t count; \
    } HashTable_##type; \
    static HashTable_##type cache_##type = {0}; \
    type GetCached##name_suffix(const char* path) { \
        if (!path) { \
            static type empty_res = {0}; \
            return empty_res; \
        } \
        EnsureCacheArenaInit(); \
        uint64_t h = HashString64(path); \
        CacheEntry_##type* entry = NULL; \
        if (cache_##type.capacity > 0) { \
            size_t mask = cache_##type.capacity - 1; \
            size_t idx = h & mask; \
            while (cache_##type.slots[idx]) { \
                if (cache_##type.slots[idx]->hash == h && strcmp(cache_##type.slots[idx]->key, path) == 0) { \
                    entry = cache_##type.slots[idx]; \
                    break; \
                } \
                idx = (idx + 1) & mask; \
            } \
        } \
        if (entry) return entry->resource; \
        type res = load_fn(path); \
        entry = (CacheEntry_##type*)ArenaAlloc(&cache_arena, sizeof(CacheEntry_##type), ARENA_ALIGNOF(CacheEntry_##type)); \
        entry->key = ArenaStrDup(&cache_arena, path); \
        entry->hash = h; \
        entry->resource = res; \
        if (cache_##type.capacity == 0 || cache_##type.count * 10 >= cache_##type.capacity * 7) { \
            size_t new_cap = cache_##type.capacity == 0 ? 128 : cache_##type.capacity * 2; \
            CacheEntry_##type** new_slots = (CacheEntry_##type**)ArenaAlloc(&cache_arena, new_cap * sizeof(CacheEntry_##type*), 8); \
            memset(new_slots, 0, new_cap * sizeof(CacheEntry_##type*)); \
            if (cache_##type.capacity > 0) { \
                size_t mask = new_cap - 1; \
                for (size_t i = 0; i < cache_##type.capacity; i++) { \
                    if (cache_##type.slots[i]) { \
                        size_t idx = cache_##type.slots[i]->hash & mask; \
                        while (new_slots[idx]) { \
                            idx = (idx + 1) & mask; \
                        } \
                        new_slots[idx] = cache_##type.slots[i]; \
                    } \
                } \
            } \
            cache_##type.slots = new_slots; \
            cache_##type.capacity = new_cap; \
        } \
        size_t mask = cache_##type.capacity - 1; \
        size_t idx = h & mask; \
        while (cache_##type.slots[idx]) { \
            idx = (idx + 1) & mask; \
        } \
        cache_##type.slots[idx] = entry; \
        cache_##type.count++; \
        return res; \
    } \
    void UnloadCached##plural_name(void) { \
        if (cache_##type.capacity > 0) { \
            for (size_t i = 0; i < cache_##type.capacity; i++) { \
                if (cache_##type.slots[i]) { \
                    unload_fn(cache_##type.slots[i]->resource); \
                } \
            } \
        } \
        cache_##type.slots = NULL; \
        cache_##type.capacity = 0; \
        cache_##type.count = 0; \
        TryFreeCacheArena(); \
    }

// Instantiate cache systems for Texture2D, Font, and Sound
DEFINE_CACHE(Texture2D, Texture, Textures, LoadTexture, UnloadTexture)
DEFINE_CACHE(Font, Font, Fonts, LoadFont, UnloadFont)
DEFINE_CACHE(Sound, Sound, Sounds, LoadSound, UnloadSound)

// --- Shader Cache ---
typedef struct {
    const char* key;
    uint64_t hash;
    Shader shader;
    long last_mod_time;
} CachedShader;

typedef struct {
    CachedShader** slots;
    size_t capacity;
    size_t count;
} HashTable_Shader;

static HashTable_Shader cache_Shader = {0};

Shader GetCachedShader(const char* path) {
    if (!path) {
        return (Shader){0};
    }
    EnsureCacheArenaInit();
    uint64_t h = HashString64(path);
    CachedShader* entry = NULL;
    if (cache_Shader.capacity > 0) {
        size_t mask = cache_Shader.capacity - 1;
        size_t idx = h & mask;
        while (cache_Shader.slots[idx]) {
            if (cache_Shader.slots[idx]->hash == h && strcmp(cache_Shader.slots[idx]->key, path) == 0) {
                entry = cache_Shader.slots[idx];
                break;
            }
            idx = (idx + 1) & mask;
        }
    }
    if (entry) return entry->shader;

    Shader sh = LoadShader(NULL, path);
    long current_mod_time = GetFileModTime(path);

    entry = (CachedShader*)ArenaAlloc(&cache_arena, sizeof(CachedShader), ARENA_ALIGNOF(CachedShader));
    entry->key = ArenaStrDup(&cache_arena, path);
    entry->hash = h;
    entry->shader = sh;
    entry->last_mod_time = current_mod_time;

    if (cache_Shader.capacity == 0 || cache_Shader.count * 10 >= cache_Shader.capacity * 7) {
        size_t new_cap = cache_Shader.capacity == 0 ? 32 : cache_Shader.capacity * 2;
        CachedShader** new_slots = (CachedShader**)ArenaAlloc(&cache_arena, new_cap * sizeof(CachedShader*), 8);
        memset(new_slots, 0, new_cap * sizeof(CachedShader*));
        if (cache_Shader.capacity > 0) {
            size_t mask = new_cap - 1;
            for (size_t i = 0; i < cache_Shader.capacity; i++) {
                if (cache_Shader.slots[i]) {
                    size_t idx = cache_Shader.slots[i]->hash & mask;
                    while (new_slots[idx]) {
                        idx = (idx + 1) & mask;
                    }
                    new_slots[idx] = cache_Shader.slots[i];
                }
            }
        }
        cache_Shader.slots = new_slots;
        cache_Shader.capacity = new_cap;
    }
    size_t mask = cache_Shader.capacity - 1;
    size_t idx = h & mask;
    while (cache_Shader.slots[idx]) {
        idx = (idx + 1) & mask;
    }
    cache_Shader.slots[idx] = entry;
    cache_Shader.count++;
    return sh;
}

void UnloadCachedShaders(void) {
    if (cache_Shader.capacity > 0) {
        for (size_t i = 0; i < cache_Shader.capacity; i++) {
            if (cache_Shader.slots[i]) {
                UnloadShader(cache_Shader.slots[i]->shader);
            }
        }
    }
    cache_Shader.slots = NULL;
    cache_Shader.capacity = 0;
    cache_Shader.count = 0;
    TryFreeCacheArena();
}

void CheckShaderUpdates(void) {
    if (cache_Shader.capacity == 0) return;
    for (size_t i = 0; i < cache_Shader.capacity; i++) {
        CachedShader* entry = cache_Shader.slots[i];
        if (entry) {
            long current_mod_time = GetFileModTime(entry->key);
            if (current_mod_time != entry->last_mod_time) {
                Shader new_sh = LoadShader(NULL, entry->key);
                if (new_sh.id > 0) {
                    UnloadShader(entry->shader);
                    entry->shader = new_sh;
                    entry->last_mod_time = current_mod_time;
                }
            }
        }
    }
}

// --- Music Cache ---
typedef struct {
    const char* key;
    uint64_t hash;
    Music music;
    int is_playing;
} ActiveMusic;

typedef struct {
    ActiveMusic** slots;
    size_t capacity;
    size_t count;
} HashTable_Music;

static HashTable_Music active_musics = {0};

void SetupAudioNodes(UINode* node) {
    if (!node) return;
    if (node->type == NODE_AUDIO && node->asset_path[0] != '\0') {
        EnsureCacheArenaInit();
        const char* path = node->asset_path;
        uint64_t h = HashString64(path);
        ActiveMusic* entry = NULL;
        if (active_musics.capacity > 0) {
            size_t mask = active_musics.capacity - 1;
            size_t idx = h & mask;
            while (active_musics.slots[idx]) {
                if (active_musics.slots[idx]->hash == h && strcmp(active_musics.slots[idx]->key, path) == 0) {
                    entry = active_musics.slots[idx];
                    break;
                }
                idx = (idx + 1) & mask;
            }
        }

        if (!entry) {
            Music m = LoadMusicStream(path);
            m.looping = node->loop;

            entry = (ActiveMusic*)ArenaAlloc(&cache_arena, sizeof(ActiveMusic), ARENA_ALIGNOF(ActiveMusic));
            entry->key = ArenaStrDup(&cache_arena, path);
            entry->hash = h;
            entry->music = m;
            entry->is_playing = 0;

            if (active_musics.capacity == 0 || active_musics.count * 10 >= active_musics.capacity * 7) {
                size_t new_cap = active_musics.capacity == 0 ? 16 : active_musics.capacity * 2;
                ActiveMusic** new_slots = (ActiveMusic**)ArenaAlloc(&cache_arena, new_cap * sizeof(ActiveMusic*), 8);
                memset(new_slots, 0, new_cap * sizeof(ActiveMusic*));
                if (active_musics.capacity > 0) {
                    size_t mask = new_cap - 1;
                    for (size_t i = 0; i < active_musics.capacity; i++) {
                        if (active_musics.slots[i]) {
                            size_t idx = active_musics.slots[i]->hash & mask;
                            while (new_slots[idx]) {
                                idx = (idx + 1) & mask;
                            }
                            new_slots[idx] = active_musics.slots[i];
                        }
                    }
                }
                active_musics.slots = new_slots;
                active_musics.capacity = new_cap;
            }
            size_t mask = active_musics.capacity - 1;
            size_t idx = h & mask;
            while (active_musics.slots[idx]) {
                idx = (idx + 1) & mask;
            }
            active_musics.slots[idx] = entry;
            active_musics.count++;
        }

        if (node->autoplay && !entry->is_playing) {
            PlayMusicStream(entry->music);
            entry->is_playing = 1;
        }
    }
    for (int i = 0; i < node->child_count; i++) {
        SetupAudioNodes(node->children[i]);
    }
}

void UpdateMusicStreams(void) {
    if (active_musics.capacity > 0) {
        for (size_t i = 0; i < active_musics.capacity; i++) {
            if (active_musics.slots[i] && active_musics.slots[i]->is_playing) {
                UpdateMusicStream(active_musics.slots[i]->music);
            }
        }
    }
}

void UnloadActiveMusics(void) {
    if (active_musics.capacity > 0) {
        for (size_t i = 0; i < active_musics.capacity; i++) {
            if (active_musics.slots[i]) {
                StopMusicStream(active_musics.slots[i]->music);
                UnloadMusicStream(active_musics.slots[i]->music);
            }
        }
    }
    active_musics.slots = NULL;
    active_musics.capacity = 0;
    active_musics.count = 0;
    TryFreeCacheArena();
}

// Cleanup helper to free the entire cache arena when all individual caches are empty
static inline void TryFreeCacheArena(void) {
    if (cache_Texture2D.count == 0 &&
        cache_Font.count == 0 &&
        cache_Sound.count == 0 &&
        cache_Shader.count == 0 &&
        active_musics.count == 0) {
        ArenaFree(&cache_arena);
    }
}
