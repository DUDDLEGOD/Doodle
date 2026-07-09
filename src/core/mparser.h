#ifndef MPARSER_H
#define MPARSER_H

#include "raylib.h"
#include <stdint.h>
#include <stddef.h>

typedef enum { NODE_VIEW, NODE_TEXT, NODE_IMAGE, NODE_BUTTON, NODE_AUDIO, NODE_CIRCLE, NODE_LINE } NodeType;
typedef enum { DIR_ROW, DIR_COLUMN } FlexDirection;
typedef enum { POS_RELATIVE, POS_ABSOLUTE } PositionMode;
typedef enum { SIZING_FIXED, SIZING_PERCENT, SIZING_GROW, SIZING_FIT } SizingType;

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
    const char* font_path;
    float font_size;
    Color text_color;

    SizingType width_type;
    float width_value;
    SizingType height_type;
    float height_value;

    float padding_left;
    float padding_right;
    float padding_top;
    float padding_bottom;

    float margin_left;
    float margin_right;
    float margin_top;
    float margin_bottom;

    const char* justify_content;
    const char* align_items;

    PositionMode position_mode;
    float left;
    float top;

    const char* text_align;
    const char* shader_path;
    float rotation;
    Color tint_color;
    int z_index;
} StyleProps;

typedef struct UINode {
    const char* id;
    const char* class_name;
    NodeType type;
    const char* text_content;
    const char* asset_path;
    int visible;
    int autoplay;
    int loop;
    int position_set;
    uint32_t class_hashes[8]; // Max 8 classes hashed for fast lookup

    StyleProps style;
    StyleProps hover_style;
    int has_hover_style;
    int currently_hovered;

    LayoutBox layout;

    int use_camera;

    float radius;
    float x2;
    float y2;
    float thickness;
    Color shape_color;

    struct UINode* parent;
    struct UINode** children;
    int child_count;
    int child_capacity;
    int line_number;
} UINode;

typedef void (*CSSPropertyHandler)(UINode* node, const char* value);

typedef struct {
    const char* property_name;
    CSSPropertyHandler handler;
    int affects_layout;
} CSSMap;

void* DOMAlloc(size_t size, size_t alignment);
const char* DOMStrDup(const char* str);

void InitDOM(void);
void CleanupDOM(void);

// Memory Management & Tree functions
UINode* CreateNode(NodeType type);
void FreeNode(UINode* node);
void AddChild(UINode* parent, UINode* child);
void AddToHashTable(UINode* node);
void RemoveFromHashTable(const char* id);
UINode* FindNodeById(UINode* root_node, const char* id);
void RemoveNode(UINode* root_node, UINode* node_to_remove);

// Style property reader
void GetStyleProperty(UINode* node, const char* name, char* out_value, int max_len);

#endif // MPARSER_H
