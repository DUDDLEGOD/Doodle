#include "css_parser.h"
#include "color.h"
#include "unit.h"
#include "string_utils.h"
#include "dom_utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    node->style.justify_content[sizeof(node->style.justify_content) - 1] = '\0';
}

void handle_align_items(UINode* node, const char* value) {
    strncpy(node->style.align_items, value, sizeof(node->style.align_items) - 1);
    node->style.align_items[sizeof(node->style.align_items) - 1] = '\0';
}

void handle_text_align(UINode* node, const char* value) {
    strncpy(node->style.text_align, value, sizeof(node->style.text_align) - 1);
    node->style.text_align[sizeof(node->style.text_align) - 1] = '\0';
}

void handle_shader(UINode* node, const char* value) {
    const char* start = value;
    if (*start == '"' || *start == '\'') start++;
    strncpy(node->style.shader_path, start, sizeof(node->style.shader_path) - 1);
    node->style.shader_path[sizeof(node->style.shader_path) - 1] = '\0';
    int len = strlen(node->style.shader_path);
    if (len > 0 && (node->style.shader_path[len-1] == '"' || node->style.shader_path[len-1] == '\'')) {
        node->style.shader_path[len-1] = '\0';
    }
}

void handle_font_family(UINode* node, const char* value) {
    const char* start = value;
    if (*start == '"' || *start == '\'') start++;
    strncpy(node->style.font_path, start, sizeof(node->style.font_path) - 1);
    node->style.font_path[sizeof(node->style.font_path) - 1] = '\0';
    int len = strlen(node->style.font_path);
    if (len > 0 && (node->style.font_path[len-1] == '"' || node->style.font_path[len-1] == '\'')) {
        node->style.font_path[len-1] = '\0';
    }
}

void handle_z_index(UINode* node, const char* value) {
    node->style.z_index = (int)atoi(value);
}

void handle_rotation(UINode* node, const char* value) {
    node->style.rotation = ParseUnit(value);
}

void handle_tint(UINode* node, const char* value) {
    node->style.tint_color = ParseColor(value);
}

CSSMap css_registry[] = {
    {"flex-direction", handle_flex_direction, 1},
    {"border-radius", handle_border_radius, 0},
    {"background-color", handle_bg_color, 0},
    {"width", handle_width, 1},
    {"height", handle_height, 1},
    {"position", handle_position, 1},
    {"left", handle_left, 1},
    {"top", handle_top, 1},
    {"color", handle_color, 0},
    {"text-color", handle_color, 0},
    {"font-size", handle_font_size, 1},
    {"display", handle_display, 1},
    {"padding", handle_padding, 1},
    {"padding-left", handle_padding_left, 1},
    {"padding-right", handle_padding_right, 1},
    {"padding-top", handle_padding_top, 1},
    {"padding-bottom", handle_padding_bottom, 1},
    {"margin", handle_margin, 1},
    {"margin-left", handle_margin_left, 1},
    {"margin-right", handle_margin_right, 1},
    {"margin-top", handle_margin_top, 1},
    {"margin-bottom", handle_margin_bottom, 1},
    {"justify-content", handle_justify_content, 1},
    {"align-items", handle_align_items, 1},
    {"text-align", handle_text_align, 0},
    {"border-color", handle_border_color, 0},
    {"border-width", handle_border_width, 0},
    {"shader", handle_shader, 0},
    {"font-family", handle_font_family, 1},
    {"z-index", handle_z_index, 0},
    {"rotation", handle_rotation, 0},
    {"tint", handle_tint, 0}
};
int css_registry_count = sizeof(css_registry) / sizeof(CSSMap);

void ApplyCSSToNode(UINode* node, const char* selector, const char* prop, const char* val) {
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

            if (*p == '/' && *(p+1) == '*') {
                p += 2;
                char* end_comment = strstr(p, "*/");
                if (end_comment) p = end_comment + 2;
                continue;
            }

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
