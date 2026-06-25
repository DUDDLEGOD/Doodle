#include "dom_utils.h"
#include <string.h>

int HasClass(const char* class_list, const char* cls) {
    if (!class_list || !cls) return 0;
    const char* p = class_list;
    int len = strlen(cls);
    while ((p = strstr(p, cls))) {
        if (p == class_list || *(p - 1) == ' ') {
            char next = *(p + len);
            if (next == '\0' || next == ' ') {
                return 1;
            }
        }
        p += len;
    }
    return 0;
}

uint32_t HashString(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

void ParseClassHashes(const char* class_list, uint32_t* hashes, int max_hashes) {
    for (int i = 0; i < max_hashes; i++) hashes[i] = 0;
    if (!class_list) return;
    
    char buf[256];
    strncpy(buf, class_list, 255);
    buf[255] = '\0';
    
    int count = 0;
    char* token = strtok(buf, " \t\r\n");
    while (token && count < max_hashes) {
        hashes[count++] = HashString(token);
        token = strtok(NULL, " \t\r\n");
    }
}

int HasClassHash(uint32_t* hashes, int max_hashes, uint32_t target_hash) {
    if (target_hash == 0) return 0;
    for (int i = 0; i < max_hashes; i++) {
        if (hashes[i] == target_hash) return 1;
        if (hashes[i] == 0) break; // End of valid hashes
    }
    return 0;
}
