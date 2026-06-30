#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t* buffer;
    size_t capacity;
    size_t offset;
} MemoryArena;

static inline void ArenaInit(MemoryArena* arena, size_t capacity) {
    arena->buffer = (uint8_t*)malloc(capacity);
    arena->capacity = capacity;
    arena->offset = 0;
}

static inline void* ArenaAlloc(MemoryArena* arena, size_t size, size_t alignment) {
    size_t current_ptr = (size_t)(arena->buffer + arena->offset);
    size_t offset = (current_ptr + (alignment - 1)) & ~(alignment - 1);
    size_t relative_offset = offset - (size_t)arena->buffer;

    if (relative_offset + size <= arena->capacity) {
        void* ptr = &arena->buffer[relative_offset];
        arena->offset = relative_offset + size;
        return ptr;
    }
    return NULL; // Out of memory
}

static inline void ArenaReset(MemoryArena* arena) {
    arena->offset = 0;
}

static inline const char* ArenaStrDup(MemoryArena* arena, const char* str) {
    if (!str) return "";
    size_t len = strlen(str);
    char* copy = (char*)ArenaAlloc(arena, len + 1, 1);
    if (copy) {
        memcpy(copy, str, len + 1);
    }
    return copy;
}

static inline void ArenaFree(MemoryArena* arena) {
    if (arena->buffer) {
        free(arena->buffer);
        arena->buffer = NULL;
    }
    arena->capacity = 0;
    arena->offset = 0;
}

#endif // ARENA_H
