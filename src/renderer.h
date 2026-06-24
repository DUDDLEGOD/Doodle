#ifndef RENDERER_H
#define RENDERER_H

#include "mparser.h"

void DrawUINode(UINode* node);
void GetSortedChildren(UINode* node, UINode** sorted_list);

#endif // RENDERER_H
