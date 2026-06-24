#include "html_parser.h"
#include "color.h"
#include "unit.h"
#include "string_utils.h"
#include "dom_utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern CSSMap css_registry[];
extern int css_registry_count;

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
                    node->id[sizeof(node->id) - 1] = '\0';
                } else if (strcmp(attr_name, "class") == 0) {
                    strncpy(node->class_name, attr_val, sizeof(node->class_name) - 1);
                    node->class_name[sizeof(node->class_name) - 1] = '\0';
                } else if (strcmp(attr_name, "src") == 0) {
                    strncpy(node->asset_path, attr_val, sizeof(node->asset_path) - 1);
                    node->asset_path[sizeof(node->asset_path) - 1] = '\0';
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
                node->text_content[sizeof(node->text_content) - 1] = '\0';
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
