#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Configurable backend allocator
#ifndef ARENA_MALLOC
#define ARENA_MALLOC(sz) malloc(sz)
#endif
#ifndef ARENA_FREE
#define ARENA_FREE(ptr) free(ptr)
#endif

// Configurable assertions
#ifndef ARENA_ASSERT
#include <assert.h>
#define ARENA_ASSERT(cond) assert(cond)
#endif

// Alignment detection/macro
#if defined(__cplusplus)
#define ARENA_ALIGNOF(type) alignof(type)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define ARENA_ALIGNOF(type) _Alignof(type)
#elif defined(__GNUC__) || defined(__clang__)
#define ARENA_ALIGNOF(type) __alignof__(type)
#elif defined(_MSC_VER)
#define ARENA_ALIGNOF(type) __alignof(type)
#else
#define ARENA_ALIGNOF(type) (sizeof(type) > 8 ? 16 : 8)
#endif

// Region structure for linked list of allocation blocks
typedef struct Region {
    struct Region* next;
    size_t count;
    size_t capacity;
    uintptr_t padding; // Ensures the flexible array starts at a 16-byte boundary on 64-bit systems
    uint8_t data[];
} Region;

// MemoryArena structure holding the list of regions
typedef struct {
    Region* begin;
    Region* end;
    size_t default_capacity;
} MemoryArena;

// Snapshot/marker structure
typedef struct {
    Region* region;
    size_t count;
} ArenaMarker;

// Helper to create a new region
static inline Region* new_region(size_t capacity) {
    Region* r = (Region*)ARENA_MALLOC(sizeof(Region) + capacity);
    if (!r) return NULL;
    r->next = NULL;
    r->count = 0;
    r->capacity = capacity;
    return r;
}

// Initialize the memory arena with a default region size
static inline void ArenaInit(MemoryArena* arena, size_t default_capacity) {
    arena->default_capacity = default_capacity;
    arena->begin = new_region(default_capacity);
    arena->end = arena->begin;
    ARENA_ASSERT(arena->begin && "Failed to initialize MemoryArena: out of memory");
}

// Allocate memory from the arena with specified alignment
static inline void* ArenaAlloc(MemoryArena* arena, size_t size, size_t alignment) {
    if (alignment == 0) {
        alignment = sizeof(uintptr_t);
    }
    // Assert alignment is a power of two
    ARENA_ASSERT((alignment & (alignment - 1)) == 0 && "Alignment must be a power of two");

    // Initialize if begin is NULL
    if (!arena->begin) {
        size_t cap = arena->default_capacity;
        if (cap == 0) cap = 1024;
        if (cap < size + alignment) cap = size + alignment;
        arena->begin = new_region(cap);
        arena->end = arena->begin;
        ARENA_ASSERT(arena->begin && "Failed to allocate initial arena region: out of memory");
    }

    Region* curr = arena->end;
    Region* last = curr;
    while (curr) {
        uintptr_t current_addr = (uintptr_t)&curr->data[curr->count];
        uintptr_t aligned_addr = (current_addr + (alignment - 1)) & ~(alignment - 1);
        size_t offset = aligned_addr - (uintptr_t)curr->data;
        if (offset + size <= curr->capacity) {
            curr->count = offset + size;
            arena->end = curr;
            return (void*)aligned_addr;
        }
        last = curr;
        curr = curr->next;
    }

    // Allocate a new region if none of the trailing regions have enough space
    size_t cap = arena->default_capacity;
    if (cap < size + alignment) {
        cap = size + alignment;
    }
    Region* next_r = new_region(cap);
    ARENA_ASSERT(next_r && "Failed to allocate new arena region: out of memory");

    last->next = next_r;
    arena->end = next_r;

    uintptr_t current_addr = (uintptr_t)&next_r->data[0];
    uintptr_t aligned_addr = (current_addr + (alignment - 1)) & ~(alignment - 1);
    size_t offset = aligned_addr - (uintptr_t)next_r->data;
    next_r->count = offset + size;
    return (void*)aligned_addr;
}

// Reallocate memory block, trying to expand in-place if possible
static inline void* ArenaRealloc(MemoryArena* arena, void* old_ptr, size_t old_size, size_t new_size, size_t alignment) {
    if (!old_ptr) {
        return ArenaAlloc(arena, new_size, alignment);
    }
    if (new_size == 0) {
        return NULL;
    }

    // Search for the region containing old_ptr to try in-place expansion
    Region* r = arena->begin;
    while (r) {
        if ((uint8_t*)old_ptr >= r->data && (uint8_t*)old_ptr < r->data + r->capacity) {
            // Found region containing old_ptr. Check if it was the last allocation
            if ((uint8_t*)old_ptr + old_size == r->data + r->count) {
                size_t offset = (uint8_t*)old_ptr - r->data;
                if (offset + new_size <= r->capacity) {
                    r->count = offset + new_size;
                    return old_ptr;
                }
            }
            break; // Cannot expand in-place
        }
        r = r->next;
    }

    // Fallback: allocate new memory and copy
    void* new_ptr = ArenaAlloc(arena, new_size, alignment);
    if (new_ptr) {
        size_t copy_size = old_size < new_size ? old_size : new_size;
        memcpy(new_ptr, old_ptr, copy_size);
    }
    return new_ptr;
}

// Reset all regions to unused state (counts to 0) without freeing memory
static inline void ArenaReset(MemoryArena* arena) {
    Region* r = arena->begin;
    while (r) {
        r->count = 0;
        r = r->next;
    }
    arena->end = arena->begin;
}

// Duplicate a string in the arena
static inline const char* ArenaStrDup(MemoryArena* arena, const char* str) {
    if (!str) return "";
    size_t len = strlen(str);
    char* copy = (char*)ArenaAlloc(arena, len + 1, 1);
    if (copy) {
        memcpy(copy, str, len + 1);
    }
    return copy;
}

// Free all regions associated with the arena
static inline void ArenaFree(MemoryArena* arena) {
    Region* r = arena->begin;
    while (r) {
        Region* next = r->next;
        ARENA_FREE(r);
        r = next;
    }
    arena->begin = NULL;
    arena->end = NULL;
    arena->default_capacity = 0;
}

// Get marker snapshot of current arena state
static inline ArenaMarker ArenaGetMarker(MemoryArena* arena) {
    ArenaMarker marker;
    marker.region = arena->end;
    marker.count = arena->end ? arena->end->count : 0;
    return marker;
}

// Rewind arena back to a snapshot marker
static inline void ArenaRewind(MemoryArena* arena, ArenaMarker marker) {
    if (!marker.region) {
        ArenaReset(arena);
        return;
    }
    arena->end = marker.region;
    arena->end->count = marker.count;

    Region* r = arena->end->next;
    while (r) {
        r->count = 0;
        r = r->next;
    }
}

// Trim and free any trailing unused regions after a rewind
static inline void ArenaTrim(MemoryArena* arena) {
    if (!arena) return;
    if (!arena->end) {
        Region* r = arena->begin;
        arena->begin = NULL;
        while (r) {
            Region* next = r->next;
            ARENA_FREE(r);
            r = next;
        }
        return;
    }
    Region* r = arena->end->next;
    arena->end->next = NULL;
    while (r) {
        Region* next = r->next;
        ARENA_FREE(r);
        r = next;
    }
}

// Variadic string formatting directly inside the arena
static inline char* ArenaVPrintf(MemoryArena* arena, const char* fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (len < 0) {
        return NULL;
    }

    char* buf = (char*)ArenaAlloc(arena, len + 1, 1);
    if (buf) {
        vsnprintf(buf, len + 1, fmt, args);
    }
    return buf;
}

static inline char* ArenaSprintf(MemoryArena* arena, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char* res = ArenaVPrintf(arena, fmt, args);
    va_end(args);
    return res;
}

// Dynamic Array macros
#define arena_da_append(arena, da, item) \
    do { \
        if ((da)->count >= (da)->capacity) { \
            size_t new_cap = (da)->capacity == 0 ? 8 : (da)->capacity * 2; \
            (da)->items = ArenaRealloc((arena), (da)->items, \
                                       (da)->capacity * sizeof(*(da)->items), \
                                       new_cap * sizeof(*(da)->items), \
                                       ARENA_ALIGNOF(*(da)->items)); \
            ARENA_ASSERT((da)->items && "Dynamic array growth failed"); \
            (da)->capacity = new_cap; \
        } \
        (da)->items[(da)->count++] = (item); \
    } while (0)

#define arena_da_resize(arena, da, new_capacity) \
    do { \
        (da)->items = ArenaRealloc((arena), (da)->items, \
                                   (da)->capacity * sizeof(*(da)->items), \
                                   (new_capacity) * sizeof(*(da)->items), \
                                   ARENA_ALIGNOF(*(da)->items)); \
        (da)->capacity = (new_capacity); \
        if ((da)->count > (new_capacity)) { \
            (da)->count = (new_capacity); \
        } \
    } while (0)

#endif // ARENA_H
