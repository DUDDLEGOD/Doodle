#ifndef DOM_UTILS_H
#define DOM_UTILS_H

#include <stdint.h>

int HasClass(const char* class_list, const char* cls);

uint32_t HashString(const char* str);
void ParseClassHashes(const char* class_list, uint32_t* hashes, int max_hashes);
int HasClassHash(uint32_t* hashes, int max_hashes, uint32_t target_hash);

#endif // DOM_UTILS_H
