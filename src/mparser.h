#ifndef MPARSER_H
#define MPARSER_H

#include "raylib.h"

typedef enum { NODE_VIEW, NODE_TEXT, NODE_IMAGE, NODE_BUTTON, NODE_AUDIO } NodeType;
typedef enum { DIR_ROW, DIR_COLUMN } FlexDirection;

typedef struct {
    float x, y, width, height;
} LayoutBox;

typedef struct {
    Color bg_color;
    Color border_color;
    float border_width;
    float border_radius;
    float opacity;
    FlexDirection flex_direction;
    char font_path[256];
    float font_size;
    Color text_color;
    
    // Additional helpful properties
    float width;
    float height;
    float padding;
} StyleProps;

typedef struct UINode {
    char id[64];
    NodeType type;
    char text_content[512];
    char asset_path[256];

    StyleProps style;       // Populated by CSS mapper
    LayoutBox layout;       // Populated by Flexbox calculator

    struct UINode* parent;
    struct UINode* children[128];
    int child_count;
} UINode;

typedef void (*CSSPropertyHandler)(UINode* node, const char* value);

typedef struct {
    const char* property_name;
    CSSPropertyHandler handler;
} CSSMap;

// Memory Management & Tree functions
UINode* CreateNode(NodeType type);
void FreeNode(UINode* node);
void AddChild(UINode* parent, UINode* child);
UINode* FindNodeById(UINode* root, const char* id);
void RemoveNode(UINode* root, UINode* node_to_remove);

// CSS Handlers
void handle_flex_direction(UINode* node, const char* value);
void handle_border_radius(UINode* node, const char* value);
void handle_bg_color(UINode* node, const char* value);
void handle_width(UINode* node, const char* value);
void handle_height(UINode* node, const char* value);

// Layout calculation
void ComputeLayout(UINode* node, float parent_x, float parent_y, float parent_width, float parent_height);

#endif // MPARSER_H
