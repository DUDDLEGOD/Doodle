#include "mparser.h"
#include "dutils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

UINode* CreateNode(NodeType type) {
    UINode* node = (UINode*)calloc(1, sizeof(UINode));
    if (!node) return NULL;
    node->type = type;
    node->style.bg_color = BLANK;
    node->style.border_color = BLANK;
    node->style.text_color = BLACK;
    node->style.font_size = 16.0f;
    node->style.opacity = 1.0f;
    node->style.flex_direction = DIR_COLUMN;
    return node;
}

// Memory Management & Tree functions
void FreeNode(UINode* node) {
    if (!node) return;
    
    // Recursively free child nodes from the bottom up
    for (int i = 0; i < node->child_count; i++) {
        FreeNode(node->children[i]);
        node->children[i] = NULL;
    }
    node->child_count = 0;
    
    // Free the parent pointer
    node->parent = NULL;
    free(node);
}

void AddChild(UINode* parent, UINode* child) {
    if (!parent || !child || parent->child_count >= 128) return;
    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

UINode* FindNodeById(UINode* root, const char* id) {
    if (!root || !id) return NULL;
    if (strcmp(root->id, id) == 0) return root;
    for (int i = 0; i < root->child_count; i++) {
        UINode* found = FindNodeById(root->children[i], id);
        if (found) return found;
    }
    return NULL;
}

void RemoveNode(UINode* root, UINode* node_to_remove) {
    if (!root || !node_to_remove) return;
    if (node_to_remove->parent) {
        UINode* parent = node_to_remove->parent;
        int found_idx = -1;
        for (int i = 0; i < parent->child_count; i++) {
            if (parent->children[i] == node_to_remove) {
                found_idx = i;
                break;
            }
        }
        if (found_idx != -1) {
            for (int i = found_idx; i < parent->child_count - 1; i++) {
                parent->children[i] = parent->children[i + 1];
            }
            parent->child_count--;
        }
    }
    FreeNode(node_to_remove);
}

// CSS Handlers
void handle_flex_direction(UINode* node, const char* value) {
    if (strstr(value, "row")) node->style.flex_direction = DIR_ROW;
    else if (strstr(value, "column")) node->style.flex_direction = DIR_COLUMN;
}

void handle_border_radius(UINode* node, const char* value) {
    node->style.border_radius = ParseUnit(value);
}

void handle_bg_color(UINode* node, const char* value) {
    node->style.bg_color = ParseColor(value);
}

void handle_width(UINode* node, const char* value) {
    if (strstr(value, "%")) {
        // Mock percentage handling, assuming it's fully computed later
        node->style.width = ParseUnit(value); 
    } else {
        node->style.width = ParseUnit(value);
    }
}

void handle_height(UINode* node, const char* value) {
    node->style.height = ParseUnit(value);
}

CSSMap css_registry[] = {
    {"flex-direction", handle_flex_direction},
    {"border-radius", handle_border_radius},
    {"background-color", handle_bg_color},
    {"width", handle_width},
    {"height", handle_height}
};

// Calculate on mutation rule: apply layout calculation explicitly
void ComputeLayout(UINode* node, float parent_x, float parent_y, float parent_width, float parent_height) {
    if (!node) return;

    // Default positioning logic
    if (node->style.width == 0) node->layout.width = parent_width;
    else node->layout.width = node->style.width;

    if (node->style.height == 0) node->layout.height = parent_height;
    else node->layout.height = node->style.height;
    
    // Very simple layout that doesn't fully implement flexbox but serves the basic absolute positioning idea
    // The requirement says: Calculate on mutation, read on render.
    
    float current_x = parent_x;
    float current_y = parent_y;

    for (int i = 0; i < node->child_count; i++) {
        UINode* child = node->children[i];
        
        child->layout.x = current_x;
        child->layout.y = current_y;

        ComputeLayout(child, current_x, current_y, child->layout.width, child->layout.height);

        if (node->style.flex_direction == DIR_ROW) {
            current_x += child->layout.width;
        } else {
            current_y += child->layout.height;
        }
    }
}
