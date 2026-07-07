#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string.h>
#include <stdint.h>

static inline uint64_t HashString64(const char* str) {
    uint64_t hash = 14695981039346656037ULL;
    while (str && *str) {
        hash ^= (uint8_t)*str++;
        hash *= 1099511628211ULL;
    }
    return hash;
}

static inline uint32_t HashString32(const char* str) {
    uint32_t hash = 2166136261U;
    while (str && *str) {
        hash ^= (uint8_t)*str++;
        hash *= 16777619U;
    }
    return hash;
}

static inline char* TrimWhitespace(char* str) {
    if (!str) return NULL;
    while (*str && (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;
    end[1] = '\0';
    return str;
}

#endif // STRING_UTILS_H
