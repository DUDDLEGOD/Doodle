#include "mparser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int g_node_id_counter = 0;

static inline unsigned int hash_id(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

typedef struct {
    char id[64];
    UINode* node;
} HashEntry;

static HashEntry node_hash_table[2048];
int node_hash_table_dirty = 1;

void MarkNodeHashTableDirty(void) {
    node_hash_table_dirty = 1;
}

static void PopulateHashHelper(UINode* n) {
    if (!n) return;
    if (strlen(n->id) > 0) {
        unsigned int h = hash_id(n->id);
        for (int i = 0; i < 2048; i++) {
            unsigned int idx = (h + i) % 2048;
            if (node_hash_table[idx].node == NULL) {
                strncpy(node_hash_table[idx].id, n->id, sizeof(node_hash_table[idx].id) - 1);
                node_hash_table[idx].id[sizeof(node_hash_table[idx].id) - 1] = '\0';
                node_hash_table[idx].node = n;
                break;
            }
        }
    }
    for (int i = 0; i < n->child_count; i++) {
        PopulateHashHelper(n->children[i]);
    }
}

void RebuildNodeHashTable(UINode* root_node) {
    memset(node_hash_table, 0, sizeof(node_hash_table));
    if (root_node) {
        PopulateHashHelper(root_node);
    }
    node_hash_table_dirty = 0;
}

#define NODE_POOL_MAX 1024
static UINode* node_pool[NODE_POOL_MAX];
static int node_pool_count = 0;

UINode* CreateNode(NodeType type) {
    UINode* node = NULL;
    if (node_pool_count > 0) {
        node = node_pool[--node_pool_count];
        memset(node, 0, sizeof(UINode));
    } else {
        node = (UINode*)calloc(1, sizeof(UINode));
    }
    if (!node) return NULL;
    
    node->type = type;
    sprintf(node->id, "__node_%d", ++g_node_id_counter);
    
    // Style defaults
    node->style.bg_color = BLANK;
    node->style.border_color = BLANK;
    node->style.text_color = BLACK;
    node->style.font_size = 16.0f;
    node->style.opacity = 1.0f;
    node->style.flex_direction = DIR_COLUMN;
    
    node->style.width_type = SIZING_FIT;
    node->style.height_type = SIZING_FIT;
    node->style.left = 0.0f;
    node->style.top = 0.0f;
    node->style.rotation = 0.0f;
    node->style.tint_color = BLANK;
    node->style.z_index = 0;
    
    node->hover_style = node->style;
    node->visible = 1;
    node->autoplay = 0;
    node->loop = 0;
    node->use_camera = 0;
    node->thickness = 1.0f;
    node->shape_color = WHITE;
    
    return node;
}

void FreeNode(UINode* node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        FreeNode(node->children[i]);
    }
    node->child_count = 0;
    
    if (node_pool_count < NODE_POOL_MAX) {
        node_pool[node_pool_count++] = node;
    } else {
        free(node);
    }
}

void AddChild(UINode* parent, UINode* child) {
    if (!parent || !child || parent->child_count >= 128) return;
    parent->children[parent->child_count++] = child;
    child->parent = parent;
    node_hash_table_dirty = 1;
}

UINode* FindNodeById(UINode* root_node, const char* id) {
    if (!root_node || !id) return NULL;
    if (node_hash_table_dirty) {
        RebuildNodeHashTable(root_node);
    }
    unsigned int h = hash_id(id);
    for (int i = 0; i < 2048; i++) {
        unsigned int idx = (h + i) % 2048;
        if (node_hash_table[idx].node == NULL) {
            return NULL;
        }
        if (strcmp(node_hash_table[idx].id, id) == 0) {
            return node_hash_table[idx].node;
        }
    }
    return NULL;
}

void RemoveNode(UINode* root_node, UINode* node_to_remove) {
    if (!root_node || !node_to_remove) return;
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
    node_hash_table_dirty = 1;
}

void GetStyleProperty(UINode* node, const char* name, char* out_value, int max_len) {
    if (!node || !name || !out_value || max_len <= 0) return;
    out_value[0] = '\0';
    
    StyleProps* s = &node->style;
    if (node->has_hover_style && node->currently_hovered) {
        s = &node->hover_style;
    }
    
    if (strcmp(name, "rotation") == 0) {
        snprintf(out_value, max_len, "%f", s->rotation);
    } else if (strcmp(name, "z-index") == 0 || strcmp(name, "z_index") == 0) {
        snprintf(out_value, max_len, "%d", s->z_index);
    } else if (strcmp(name, "background-color") == 0 || strcmp(name, "background_color") == 0) {
        snprintf(out_value, max_len, "#%02x%02x%02x%02x", s->bg_color.r, s->bg_color.g, s->bg_color.b, s->bg_color.a);
    } else if (strcmp(name, "border-color") == 0 || strcmp(name, "border_color") == 0) {
        snprintf(out_value, max_len, "#%02x%02x%02x%02x", s->border_color.r, s->border_color.g, s->border_color.b, s->border_color.a);
    } else if (strcmp(name, "color") == 0 || strcmp(name, "text-color") == 0 || strcmp(name, "text_color") == 0) {
        snprintf(out_value, max_len, "#%02x%02x%02x%02x", s->text_color.r, s->text_color.g, s->text_color.b, s->text_color.a);
    } else if (strcmp(name, "tint") == 0 || strcmp(name, "tint-color") == 0 || strcmp(name, "tint_color") == 0) {
        snprintf(out_value, max_len, "#%02x%02x%02x%02x", s->tint_color.r, s->tint_color.g, s->tint_color.b, s->tint_color.a);
    } else if (strcmp(name, "border-width") == 0 || strcmp(name, "border_width") == 0) {
        snprintf(out_value, max_len, "%f", s->border_width);
    } else if (strcmp(name, "border-radius") == 0 || strcmp(name, "border_radius") == 0) {
        snprintf(out_value, max_len, "%f", s->border_radius);
    } else if (strcmp(name, "font-size") == 0 || strcmp(name, "font_size") == 0) {
        snprintf(out_value, max_len, "%f", s->font_size);
    } else if (strcmp(name, "left") == 0) {
        snprintf(out_value, max_len, "%f", s->left);
    } else if (strcmp(name, "top") == 0) {
        snprintf(out_value, max_len, "%f", s->top);
    } else if (strcmp(name, "width") == 0) {
        if (s->width_type == SIZING_FIXED) snprintf(out_value, max_len, "%f", s->width_value);
        else if (s->width_type == SIZING_PERCENT) snprintf(out_value, max_len, "%f%%", s->width_value);
        else if (s->width_type == SIZING_GROW) snprintf(out_value, max_len, "grow");
        else if (s->width_type == SIZING_FIT) snprintf(out_value, max_len, "fit");
    } else if (strcmp(name, "height") == 0) {
        if (s->height_type == SIZING_FIXED) snprintf(out_value, max_len, "%f", s->height_value);
        else if (s->height_type == SIZING_PERCENT) snprintf(out_value, max_len, "%f%%", s->height_value);
        else if (s->height_type == SIZING_GROW) snprintf(out_value, max_len, "grow");
        else if (s->height_type == SIZING_FIT) snprintf(out_value, max_len, "fit");
    }
}
