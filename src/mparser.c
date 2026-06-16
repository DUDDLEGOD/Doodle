#include "mparser.h"
#include "dutils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int g_node_id_counter = 0;

UINode* CreateNode(NodeType type) {
    UINode* node = (UINode*)calloc(1, sizeof(UINode));
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
    
    node->hover_style = node->style;
    node->visible = 1;
    node->thickness = 1.0f;
    node->shape_color = WHITE;
    
    return node;
}

void FreeNode(UINode* node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        FreeNode(node->children[i]);
        node->children[i] = NULL;
    }
    node->child_count = 0;
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

void handle_border_color(UINode* node, const char* value) {
    node->style.border_color = ParseColor(value);
}

void handle_border_width(UINode* node, const char* value) {
    node->style.border_width = ParseUnit(value);
}

void parse_sizing(const char* value, SizingType* type, float* val) {
    if (strcmp(value, "grow") == 0) {
        *type = SIZING_GROW;
        *val = 0.0f;
    } else if (strcmp(value, "fit") == 0 || strcmp(value, "fit-content") == 0) {
        *type = SIZING_FIT;
        *val = 0.0f;
    } else {
        int isPercent = 0;
        float num = ParseUnitExt(value, &isPercent);
        if (isPercent) {
            *type = SIZING_PERCENT;
            *val = num;
        } else {
            *type = SIZING_FIXED;
            *val = num;
        }
    }
}

void handle_width(UINode* node, const char* value) {
    parse_sizing(value, &node->style.width_type, &node->style.width_value);
}

void handle_height(UINode* node, const char* value) {
    parse_sizing(value, &node->style.height_type, &node->style.height_value);
}

void handle_position(UINode* node, const char* value) {
    if (strstr(value, "absolute")) node->style.position_mode = POS_ABSOLUTE;
    else node->style.position_mode = POS_RELATIVE;
}

void handle_left(UINode* node, const char* value) {
    node->style.left = ParseUnit(value);
}

void handle_top(UINode* node, const char* value) {
    node->style.top = ParseUnit(value);
}

void handle_color(UINode* node, const char* value) {
    node->style.text_color = ParseColor(value);
}

void handle_font_size(UINode* node, const char* value) {
    node->style.font_size = ParseUnit(value);
}

void handle_display(UINode* node, const char* value) {
    if (strstr(value, "none")) node->visible = 0;
    else node->visible = 1;
}

void handle_padding(UINode* node, const char* value) {
    float val1 = 0.0f, val2 = 0.0f, val3 = 0.0f, val4 = 0.0f;
    int count = sscanf(value, "%f %f %f %f", &val1, &val2, &val3, &val4);
    if (count == 1) {
        node->style.padding_left = node->style.padding_right = node->style.padding_top = node->style.padding_bottom = val1;
    } else if (count == 2) {
        node->style.padding_top = node->style.padding_bottom = val1;
        node->style.padding_left = node->style.padding_right = val2;
    } else if (count == 4) {
        node->style.padding_top = val1;
        node->style.padding_right = val2;
        node->style.padding_bottom = val3;
        node->style.padding_left = val4;
    }
}

void handle_padding_left(UINode* node, const char* value) { node->style.padding_left = ParseUnit(value); }
void handle_padding_right(UINode* node, const char* value) { node->style.padding_right = ParseUnit(value); }
void handle_padding_top(UINode* node, const char* value) { node->style.padding_top = ParseUnit(value); }
void handle_padding_bottom(UINode* node, const char* value) { node->style.padding_bottom = ParseUnit(value); }

void handle_margin(UINode* node, const char* value) {
    float val1 = 0.0f, val2 = 0.0f, val3 = 0.0f, val4 = 0.0f;
    int count = sscanf(value, "%f %f %f %f", &val1, &val2, &val3, &val4);
    if (count == 1) {
        node->style.margin_left = node->style.margin_right = node->style.margin_top = node->style.margin_bottom = val1;
    } else if (count == 2) {
        node->style.margin_top = node->style.margin_bottom = val1;
        node->style.margin_left = node->style.margin_right = val2;
    } else if (count == 4) {
        node->style.margin_top = val1;
        node->style.margin_right = val2;
        node->style.margin_bottom = val3;
        node->style.margin_left = val4;
    }
}

void handle_margin_left(UINode* node, const char* value) { node->style.margin_left = ParseUnit(value); }
void handle_margin_right(UINode* node, const char* value) { node->style.margin_right = ParseUnit(value); }
void handle_margin_top(UINode* node, const char* value) { node->style.margin_top = ParseUnit(value); }
void handle_margin_bottom(UINode* node, const char* value) { node->style.margin_bottom = ParseUnit(value); }

void handle_justify_content(UINode* node, const char* value) {
    strncpy(node->style.justify_content, value, sizeof(node->style.justify_content) - 1);
}

void handle_align_items(UINode* node, const char* value) {
    strncpy(node->style.align_items, value, sizeof(node->style.align_items) - 1);
}

void handle_text_align(UINode* node, const char* value) {
    strncpy(node->style.text_align, value, sizeof(node->style.text_align) - 1);
}

void handle_shader(UINode* node, const char* value) {
    const char* start = value;
    if (*start == '"' || *start == '\'') start++;
    strncpy(node->style.shader_path, start, sizeof(node->style.shader_path) - 1);
    int len = strlen(node->style.shader_path);
    if (len > 0 && (node->style.shader_path[len-1] == '"' || node->style.shader_path[len-1] == '\'')) {
        node->style.shader_path[len-1] = '\0';
    }
}

void handle_font_family(UINode* node, const char* value) {
    const char* start = value;
    if (*start == '"' || *start == '\'') start++;
    strncpy(node->style.font_path, start, sizeof(node->style.font_path) - 1);
    int len = strlen(node->style.font_path);
    if (len > 0 && (node->style.font_path[len-1] == '"' || node->style.font_path[len-1] == '\'')) {
        node->style.font_path[len-1] = '\0';
    }
}

CSSMap css_registry[] = {
    {"flex-direction", handle_flex_direction},
    {"border-radius", handle_border_radius},
    {"background-color", handle_bg_color},
    {"width", handle_width},
    {"height", handle_height},
    {"position", handle_position},
    {"left", handle_left},
    {"top", handle_top},
    {"color", handle_color},
    {"text-color", handle_color},
    {"font-size", handle_font_size},
    {"display", handle_display},
    {"padding", handle_padding},
    {"padding-left", handle_padding_left},
    {"padding-right", handle_padding_right},
    {"padding-top", handle_padding_top},
    {"padding-bottom", handle_padding_bottom},
    {"margin", handle_margin},
    {"margin-left", handle_margin_left},
    {"margin-right", handle_margin_right},
    {"margin-top", handle_margin_top},
    {"margin-bottom", handle_margin_bottom},
    {"justify-content", handle_justify_content},
    {"align-items", handle_align_items},
    {"text-align", handle_text_align},
    {"border-color", handle_border_color},
    {"border-width", handle_border_width},
    {"shader", handle_shader},
    {"font-family", handle_font_family}
};
int css_registry_count = sizeof(css_registry) / sizeof(CSSMap);

// HTML Parser
UINode* ParseHTML(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);

    UINode* root = NULL;
    UINode* stack[256];
    int stack_top = -1;

    char* p = buf;
    while (*p) {
        while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
        if (!*p) break;

        if (*p == '<') {
            p++;
            if (*p == '!') {
                p++;
                if (strncmp(p, "--", 2) == 0) {
                    p += 2;
                    char* end_comment = strstr(p, "-->");
                    if (end_comment) {
                        p = end_comment + 3;
                    }
                }
                continue;
            }
            if (*p == '/') {
                p++;
                char tagname[64];
                int i = 0;
                while (*p && *p != '>' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
                    if (i < 63) tagname[i++] = *p;
                    p++;
                }
                tagname[i] = '\0';
                while (*p && *p != '>') p++;
                if (*p == '>') p++;

                if (stack_top >= 0) {
                    stack_top--;
                }
                continue;
            }

            char tagname[64];
            int i = 0;
            while (*p && *p != '>' && *p != '/' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
                if (i < 63) tagname[i++] = *p;
                p++;
            }
            tagname[i] = '\0';

            NodeType type = NODE_VIEW;
            if (strcmp(tagname, "view") == 0) type = NODE_VIEW;
            else if (strcmp(tagname, "text") == 0) type = NODE_TEXT;
            else if (strcmp(tagname, "image") == 0) type = NODE_IMAGE;
            else if (strcmp(tagname, "button") == 0) type = NODE_BUTTON;
            else if (strcmp(tagname, "audio") == 0) type = NODE_AUDIO;
            else if (strcmp(tagname, "circle") == 0) type = NODE_CIRCLE;
            else if (strcmp(tagname, "line") == 0) type = NODE_LINE;

            UINode* node = CreateNode(type);
            node->visible = 1;

            int self_closing = 0;
            while (*p) {
                while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
                if (*p == '>') {
                    p++;
                    break;
                }
                if (*p == '/' && *(p+1) == '>') {
                    self_closing = 1;
                    p += 2;
                    break;
                }
                if (!*p) break;

                char attr_name[64];
                int ai = 0;
                while (*p && *p != '=' && *p != '>' && *p != '/' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
                    if (ai < 63) attr_name[ai++] = *p;
                    p++;
                }
                attr_name[ai] = '\0';

                char attr_val[512] = {0};
                while (*p && (*p == ' ' || *p == '\t')) p++;
                if (*p == '=') {
                    p++;
                    while (*p && (*p == ' ' || *p == '\t' || *p == '\"' || *p == '\'')) p++;
                    int vi = 0;
                    while (*p && *p != '\"' && *p != '\'') {
                        if (vi < 511) attr_val[vi++] = *p;
                        p++;
                    }
                    attr_val[vi] = '\0';
                    if (*p == '\"' || *p == '\'') p++;
                }

                if (strcmp(attr_name, "id") == 0) {
                    strncpy(node->id, attr_val, sizeof(node->id) - 1);
                } else if (strcmp(attr_name, "class") == 0) {
                    strncpy(node->class_name, attr_val, sizeof(node->class_name) - 1);
                } else if (strcmp(attr_name, "src") == 0) {
                    strncpy(node->asset_path, attr_val, sizeof(node->asset_path) - 1);
                } else if (strcmp(attr_name, "autoplay") == 0) {
                    node->autoplay = 1;
                } else if (strcmp(attr_name, "loop") == 0) {
                    node->loop = 1;
                } else if (strcmp(attr_name, "camera") == 0) {
                    if (strcmp(attr_val, "true") == 0) {
                        node->use_camera = 1;
                    }
                } else if (strcmp(attr_name, "radius") == 0) {
                    node->radius = ParseUnit(attr_val);
                } else if (strcmp(attr_name, "x2") == 0) {
                    node->x2 = ParseUnit(attr_val);
                } else if (strcmp(attr_name, "y2") == 0) {
                    node->y2 = ParseUnit(attr_val);
                } else if (strcmp(attr_name, "thickness") == 0) {
                    node->thickness = ParseUnit(attr_val);
                } else if (strcmp(attr_name, "color") == 0) {
                    node->shape_color = ParseColor(attr_val);
                } else if (strcmp(attr_name, "style") == 0) {
                    char* s_ptr = attr_val;
                    while (*s_ptr) {
                        while (*s_ptr && (*s_ptr == ' ' || *s_ptr == '\t' || *s_ptr == ';')) s_ptr++;
                        if (!*s_ptr) break;
                        char p_name[64] = {0};
                        int p_idx = 0;
                        while (*s_ptr && *s_ptr != ':' && *s_ptr != ';') {
                            if (p_idx < 63) p_name[p_idx++] = *s_ptr;
                            s_ptr++;
                        }
                        if (*s_ptr == ':') s_ptr++;
                        char p_val[128] = {0};
                        int v_idx = 0;
                        while (*s_ptr && *s_ptr != ';') {
                            if (v_idx < 127) p_val[v_idx++] = *s_ptr;
                            s_ptr++;
                        }
                        char* trimmed_name = TrimWhitespace(p_name);
                        char* trimmed_val = TrimWhitespace(p_val);
                        if (strlen(trimmed_name) > 0 && strlen(trimmed_val) > 0) {
                            for (int k = 0; k < css_registry_count; k++) {
                                if (strcmp(css_registry[k].property_name, trimmed_name) == 0) {
                                    css_registry[k].handler(node, trimmed_val);
                                    
                                    StyleProps temp = node->style;
                                    node->style = node->hover_style;
                                    css_registry[k].handler(node, trimmed_val);
                                    node->hover_style = node->style;
                                    node->style = temp;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if (stack_top == -1) {
                root = node;
            } else {
                AddChild(stack[stack_top], node);
            }

            char inner_text[512] = {0};
            int ti = 0;
            while (*p && *p != '<') {
                if (ti < 511) inner_text[ti++] = *p;
                p++;
            }
            inner_text[ti] = '\0';
            char* trimmed = TrimWhitespace(inner_text);
            if (strlen(trimmed) > 0) {
                strncpy(node->text_content, trimmed, sizeof(node->text_content) - 1);
            }

            if (!self_closing) {
                stack[++stack_top] = node;
            }
        } else {
            p++;
        }
    }

    free(buf);
    return root;
}
// CSS Helper
static void ApplyCSSToNode(UINode* node, const char* selector, const char* prop, const char* val) {
    if (!node) return;
    int matches = 0;

    int is_hover_selector = 0;
    char base_selector[64];
    strncpy(base_selector, selector, sizeof(base_selector) - 1);
    base_selector[sizeof(base_selector) - 1] = '\0';
    char* colon = strchr(base_selector, ':');
    if (colon && strcmp(colon, ":hover") == 0) {
        *colon = '\0';
        is_hover_selector = 1;
    }

    if (base_selector[0] == '.') {
        if (HasClass(node->class_name, base_selector + 1)) matches = 1;
    } else if (base_selector[0] == '#') {
        if (strcmp(node->id, base_selector + 1) == 0) matches = 1;
    } else {
        const char* tag = "view";
        if (node->type == NODE_VIEW) tag = "view";
        else if (node->type == NODE_TEXT) tag = "text";
        else if (node->type == NODE_IMAGE) tag = "image";
        else if (node->type == NODE_BUTTON) tag = "button";
        else if (node->type == NODE_AUDIO) tag = "audio";
        else if (node->type == NODE_CIRCLE) tag = "circle";
        else if (node->type == NODE_LINE) tag = "line";
        if (strcmp(tag, base_selector) == 0) matches = 1;
    }

    if (matches) {
        for (int i = 0; i < css_registry_count; i++) {
            if (strcmp(css_registry[i].property_name, prop) == 0) {
                if (is_hover_selector) {
                    StyleProps temp = node->style;
                    node->style = node->hover_style;
                    css_registry[i].handler(node, val);
                    node->hover_style = node->style;
                    node->style = temp;
                    node->has_hover_style = 1;
                } else {
                    css_registry[i].handler(node, val);
                    
                    StyleProps temp = node->style;
                    node->style = node->hover_style;
                    css_registry[i].handler(node, val);
                    node->hover_style = node->style;
                    node->style = temp;
                }
                break;
            }
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        ApplyCSSToNode(node->children[i], selector, prop, val);
    }
}

// CSS Loader
void LoadAndApplyCSS(UINode* root, const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(size + 1);
    if (!buf) {
        fclose(f);
        return;
    }
    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);

    char* p = buf;
    while (*p) {
        while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
        if (!*p) break;

        if (*p == '/' && *(p+1) == '*') {
            p += 2;
            char* end_comment = strstr(p, "*/");
            if (end_comment) p = end_comment + 2;
            continue;
        }

        char selector[64];
        int si = 0;
        while (*p && *p != '{' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
            if (si < 63) selector[si++] = *p;
            p++;
        }
        selector[si] = '\0';
        while (*p && *p != '{') p++;
        if (*p == '{') p++;

        while (*p && *p != '}') {
            while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ';')) p++;
            if (*p == '}') break;

            char prop[64];
            int pi = 0;
            while (*p && *p != ':' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
                if (pi < 63) prop[pi++] = *p;
                p++;
            }
            prop[pi] = '\0';
            while (*p && *p != ':') p++;
            if (*p == ':') p++;

            char val[128];
            int vi = 0;
            while (*p && (*p == ' ' || *p == '\t')) p++;
            while (*p && *p != ';' && *p != '}') {
                if (vi < 127) val[vi++] = *p;
                p++;
            }
            val[vi] = '\0';
            char* trimmed_val = TrimWhitespace(val);

            if (strlen(selector) > 0 && strlen(prop) > 0 && strlen(trimmed_val) > 0) {
                ApplyCSSToNode(root, selector, prop, trimmed_val);
            }
        }
        if (*p == '}') p++;
    }

    free(buf);
}

// Dual-pass Sizing and Position calculation (Clay-inspired)
static void ResolveSizes(UINode* node, float parent_content_w, float parent_content_h) {
    if (!node) return;

    StyleProps* s = &node->style;
    if (node->has_hover_style && node->currently_hovered) {
        s = &node->hover_style;
    }

    // Resolve self width
    if (s->width_type == SIZING_FIXED) {
        node->layout.width = s->width_value;
    } else if (s->width_type == SIZING_PERCENT) {
        node->layout.width = parent_content_w * (s->width_value / 100.0f);
    }
    
    // Resolve self height
    if (s->height_type == SIZING_FIXED) {
        node->layout.height = s->height_value;
    } else if (s->height_type == SIZING_PERCENT) {
        node->layout.height = parent_content_h * (s->height_value / 100.0f);
    }

    float inner_w = node->layout.width - (s->padding_left + s->padding_right);
    float inner_h = node->layout.height - (s->padding_top + s->padding_bottom);
    if (inner_w < 0.0f) inner_w = 0.0f;
    if (inner_h < 0.0f) inner_h = 0.0f;

    int grow_count = 0;
    float non_grow_sum = 0.0f;

    for (int i = 0; i < node->child_count; i++) {
        UINode* child = node->children[i];
        if (!child->visible) continue;

        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) {
            cs = &child->hover_style;
        }

        if (cs->position_mode == POS_ABSOLUTE) {
            ResolveSizes(child, inner_w, inner_h);
            continue;
        }

        if (s->flex_direction == DIR_ROW) {
            if (cs->width_type == SIZING_GROW) {
                grow_count++;
            } else {
                ResolveSizes(child, inner_w, inner_h);
                non_grow_sum += child->layout.width + cs->margin_left + cs->margin_right;
            }
        } else {
            if (cs->height_type == SIZING_GROW) {
                grow_count++;
            } else {
                ResolveSizes(child, inner_w, inner_h);
                non_grow_sum += child->layout.height + cs->margin_top + cs->margin_bottom;
            }
        }
    }

    float grow_size = 0.0f;
    if (grow_count > 0) {
        float total_avail = (s->flex_direction == DIR_ROW) ? inner_w : inner_h;
        float remaining = total_avail - non_grow_sum;
        if (remaining < 0.0f) remaining = 0.0f;
        grow_size = remaining / grow_count;
    }

    for (int i = 0; i < node->child_count; i++) {
        UINode* child = node->children[i];
        if (!child->visible || child->style.position_mode == POS_ABSOLUTE) continue;

        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) {
            cs = &child->hover_style;
        }

        if (s->flex_direction == DIR_ROW && cs->width_type == SIZING_GROW) {
            child->layout.width = grow_size - (cs->margin_left + cs->margin_right);
            if (child->layout.width < 0.0f) child->layout.width = 0.0f;
            ResolveSizes(child, inner_w, inner_h);
        } else if (s->flex_direction == DIR_COLUMN && cs->height_type == SIZING_GROW) {
            child->layout.height = grow_size - (cs->margin_top + cs->margin_bottom);
            if (child->layout.height < 0.0f) child->layout.height = 0.0f;
            ResolveSizes(child, inner_w, inner_h);
        }
    }

    if (s->width_type == SIZING_FIT || s->height_type == SIZING_FIT) {
        float child_max_w = 0.0f;
        float child_max_h = 0.0f;
        float child_sum_w = 0.0f;
        float child_sum_h = 0.0f;

        for (int i = 0; i < node->child_count; i++) {
            UINode* child = node->children[i];
            if (!child->visible || child->style.position_mode == POS_ABSOLUTE) continue;

            StyleProps* cs = &child->style;
            if (child->has_hover_style && child->currently_hovered) {
                cs = &child->hover_style;
            }

            float cw = child->layout.width + cs->margin_left + cs->margin_right;
            float ch = child->layout.height + cs->margin_top + cs->margin_bottom;

            if (cw > child_max_w) child_max_w = cw;
            if (ch > child_max_h) child_max_h = ch;
            child_sum_w += cw;
            child_sum_h += ch;
        }

        if (s->width_type == SIZING_FIT) {
            if (s->flex_direction == DIR_ROW) {
                node->layout.width = child_sum_w + s->padding_left + s->padding_right;
            } else {
                node->layout.width = child_max_w + s->padding_left + s->padding_right;
            }
        }
        if (s->height_type == SIZING_FIT) {
            if (s->flex_direction == DIR_ROW) {
                node->layout.height = child_max_h + s->padding_top + s->padding_bottom;
            } else {
                node->layout.height = child_sum_h + s->padding_top + s->padding_bottom;
            }
        }
    }
}

static void ResolvePositions(UINode* node, float start_x, float start_y, float parent_content_w, float parent_content_h) {
    if (!node) return;

    StyleProps* s = &node->style;
    if (node->has_hover_style && node->currently_hovered) {
        s = &node->hover_style;
    }

    if (!node->position_set) {
        node->layout.x = start_x;
        node->layout.y = start_y;
    }

    float inner_w = node->layout.width - (s->padding_left + s->padding_right);
    float inner_h = node->layout.height - (s->padding_top + s->padding_bottom);
    if (inner_w < 0.0f) inner_w = 0.0f;
    if (inner_h < 0.0f) inner_h = 0.0f;

    float child_start_x = node->layout.x + s->padding_left;
    float child_start_y = node->layout.y + s->padding_top;
    float gap = 0.0f;

    int rel_count = 0;
    float rel_sum = 0.0f;
    for (int i = 0; i < node->child_count; i++) {
        UINode* child = node->children[i];
        if (!child->visible || child->style.position_mode == POS_ABSOLUTE) continue;
        rel_count++;
        
        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) {
            cs = &child->hover_style;
        }

        if (s->flex_direction == DIR_ROW) {
            rel_sum += child->layout.width + cs->margin_left + cs->margin_right;
        } else {
            rel_sum += child->layout.height + cs->margin_top + cs->margin_bottom;
        }
    }

    if (s->flex_direction == DIR_ROW) {
        float remaining = inner_w - rel_sum;
        if (remaining > 0.0f) {
            if (strcmp(s->justify_content, "flex-end") == 0) {
                child_start_x += remaining;
            } else if (strcmp(s->justify_content, "center") == 0) {
                child_start_x += remaining / 2.0f;
            } else if (strcmp(s->justify_content, "space-between") == 0 && rel_count > 1) {
                gap = remaining / (rel_count - 1);
            } else if (strcmp(s->justify_content, "space-around") == 0 && rel_count > 0) {
                gap = remaining / rel_count;
                child_start_x += gap / 2.0f;
            }
        }
    } else {
        float remaining = inner_h - rel_sum;
        if (remaining > 0.0f) {
            if (strcmp(s->justify_content, "flex-end") == 0) {
                child_start_y += remaining;
            } else if (strcmp(s->justify_content, "center") == 0) {
                child_start_y += remaining / 2.0f;
            } else if (strcmp(s->justify_content, "space-between") == 0 && rel_count > 1) {
                gap = remaining / (rel_count - 1);
            } else if (strcmp(s->justify_content, "space-around") == 0 && rel_count > 0) {
                gap = remaining / rel_count;
                child_start_y += gap / 2.0f;
            }
        }
    }

    float current_x = child_start_x;
    float current_y = child_start_y;

    for (int i = 0; i < node->child_count; i++) {
        UINode* child = node->children[i];
        if (!child->visible) continue;

        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) {
            cs = &child->hover_style;
        }

        if (cs->position_mode == POS_ABSOLUTE) {
            float abs_x = node->layout.x + s->padding_left + cs->left + cs->margin_left;
            float abs_y = node->layout.y + s->padding_top + cs->top + cs->margin_top;
            ResolvePositions(child, abs_x, abs_y, inner_w, inner_h);
        } else {
            if (s->flex_direction == DIR_ROW) {
                current_x += cs->margin_left;
                
                float final_y = current_y + cs->margin_top;
                if (strcmp(s->align_items, "center") == 0) {
                    float avail_h = inner_h - cs->margin_top - cs->margin_bottom;
                    final_y = current_y + cs->margin_top + (avail_h - child->layout.height) / 2.0f;
                }
                
                ResolvePositions(child, current_x, final_y, inner_w, inner_h);
                current_x += child->layout.width + cs->margin_right + gap;
            } else {
                current_y += cs->margin_top;
                
                float final_x = current_x + cs->margin_left;
                if (strcmp(s->align_items, "center") == 0) {
                    float avail_w = inner_w - cs->margin_left - cs->margin_right;
                    final_x = current_x + cs->margin_left + (avail_w - child->layout.width) / 2.0f;
                }
                
                ResolvePositions(child, final_x, current_y, inner_w, inner_h);
                current_y += child->layout.height + cs->margin_bottom + gap;
            }
        }
    }
}

void ComputeLayout(UINode* node, float parent_x, float parent_y, float parent_width, float parent_height) {
    if (!node) return;
    ResolveSizes(node, parent_width, parent_height);
    ResolvePositions(node, parent_x, parent_y, parent_width, parent_height);
}
