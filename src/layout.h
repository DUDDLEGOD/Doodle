#ifndef LAYOUT_H
#define LAYOUT_H

#include "mparser.h"

void MeasureNode(UINode* node);
void LayoutNode(UINode* node, float available_width, float available_height);

#endif // LAYOUT_H
