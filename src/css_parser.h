#ifndef CSS_PARSER_H
#define CSS_PARSER_H

#include "mparser.h"

void LoadAndApplyCSS(UINode* root, const char* filepath);
void ApplyCSSToNode(UINode* node, const char* selector, const char* prop, const char* val);

#endif // CSS_PARSER_H
