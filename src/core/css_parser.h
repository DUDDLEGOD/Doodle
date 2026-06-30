#ifndef CSS_PARSER_H
#define CSS_PARSER_H

#include "mparser.h"

typedef struct {
    char selector[64];
    uint32_t selector_hash; // for classes and ids
    int is_class;
    int is_id;
    int is_hover;
    int specificity;
    
    char property[64];
    char value[128];
} CSSRule;

#define MAX_CSS_RULES 1024
typedef struct {
    CSSRule rules[MAX_CSS_RULES];
    int count;
} StyleSheet;

extern StyleSheet global_stylesheet;

void LoadCSS(const char* filepath);
void ApplyStyleSheetToTree(UINode* root);
void ApplyCSSToNode(UINode* node, const char* selector, const char* prop, const char* val);

#endif // CSS_PARSER_H
