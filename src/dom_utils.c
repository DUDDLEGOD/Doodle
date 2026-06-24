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
