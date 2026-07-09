#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

static inline char* LoadFileContent(const char* filepath) {
    if (!filepath) return NULL;
    FILE* f = fopen(filepath, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);
    return buf;
}

#endif // STRING_UTILS_H
