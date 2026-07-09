#include "html_parser.h"
#include "color.h"
#include "unit.h"
#include "string_utils.h"
#include "dom_utils.h"
#include "css_parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int GetLineNumber(const char* buf, const char* p) {
    int line = 1;
    for (const char* c = buf; c < p; c++) {
        if (*c == '\n') line++;
    }
    return line;
}

static void SkipWhitespace(char** p) {
    while (**p && (**p == ' ' || **p == '\t' || **p == '\r' || **p == '\n')) (*p)++;
}

static void ReadIdentifier(char** p, char* out, int max_len) {
    int i = 0;
    while (**p && **p != '=' && **p != '>' && **p != '/' && **p != ' ' && **p != '\t' && **p != '\r' && **p != '\n') {
        if (i < max_len - 1) out[i++] = **p;
        (*p)++;
    }
    out[i] = '\0';
}

static void ReadStringLiteral(char** p, char* out, int max_len) {
    int i = 0;
    char quote = **p;
    if (quote == '"' || quote == '\'') {
        (*p)++;
        while (**p && **p != quote) {
            if (i < max_len - 1) out[i++] = **p;
            (*p)++;
        }
        if (**p == quote) (*p)++;
    } else {
        while (**p && **p != ' ' && **p != '\t' && **p != '>' && **p != '/') {
            if (i < max_len - 1) out[i++] = **p;
            (*p)++;
        }
    }
    out[i] = '\0';
}

UINode* ParseHTML(const char* filepath) {
    InitDOM();
    char* buf = LoadFileContent(filepath);
    if (!buf) return NULL;

    UINode* root = NULL;
    UINode* stack[256];
    int stack_top = -1;

    char* p = buf;
    while (*p) {
        SkipWhitespace(&p);
        if (!*p) break;

        if (*p == '<') {
            p++;
            if (*p == '!') {
                p++;
                if (strncmp(p, "--", 2) == 0) {
                    p += 2;
                    char* end_comment = strstr(p, "-->");
                    if (end_comment) p = end_comment + 3;
                }
                continue;
            }
            if (*p == '/') {
                p++;
                while (*p && *p != '>') p++;
                if (*p == '>') p++;
                if (stack_top >= 0) stack_top--;
                continue;
            }

            char tagname[64];
            ReadIdentifier(&p, tagname, 64);

            NodeType type = NODE_VIEW;
            switch (tagname[0]) {
                case 'v': if (strcmp(tagname, "view") == 0) type = NODE_VIEW; break;
                case 't': if (strcmp(tagname, "text") == 0) type = NODE_TEXT; break;
                case 'i': if (strcmp(tagname, "image") == 0) type = NODE_IMAGE; break;
                case 'b': if (strcmp(tagname, "button") == 0) type = NODE_BUTTON; break;
                case 'a': if (strcmp(tagname, "audio") == 0) type = NODE_AUDIO; break;
                case 'c': if (strcmp(tagname, "circle") == 0) type = NODE_CIRCLE; break;
                case 'l': if (strcmp(tagname, "line") == 0) type = NODE_LINE; break;
            }

            UINode* node = CreateNode(type);
            node->visible = 1;
            node->line_number = GetLineNumber(buf, p);

            int self_closing = 0;
            while (*p) {
                SkipWhitespace(&p);
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
                ReadIdentifier(&p, attr_name, 64);

                char attr_val[512] = {0};
                SkipWhitespace(&p);
                if (*p == '=') {
                    p++;
                    SkipWhitespace(&p);
                    ReadStringLiteral(&p, attr_val, 512);
                }

                switch (attr_name[0]) {
                    case 'i':
                        if (strcmp(attr_name, "id") == 0) {
                            RemoveFromHashTable(node->id);
                            node->id = DOMStrDup(attr_val);
                            AddToHashTable(node);
                        }
                        break;
                    case 'c':
                        if (strcmp(attr_name, "class") == 0) {
                            node->class_name = DOMStrDup(attr_val);
                            ParseClassHashes(attr_val, node->class_hashes, 8);
                        } else if (strcmp(attr_name, "camera") == 0) {
                            if (strcmp(attr_val, "true") == 0) node->use_camera = 1;
                        } else if (strcmp(attr_name, "color") == 0) {
                            node->shape_color = ParseColor(attr_val);
                        }
                        break;
                    case 's':
                        if (strcmp(attr_name, "src") == 0) {
                            node->asset_path = DOMStrDup(attr_val);
                        } else if (strcmp(attr_name, "style") == 0) {
                            // Inline styles bypass global stylesheet and apply directly
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
                                    extern CSSMap css_registry[];
                                    extern int css_registry_count;
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
                        break;
                    case 'a':
                        if (strcmp(attr_name, "autoplay") == 0) {
                            node->autoplay = 1;
                        }
                        break;
                    case 'l':
                        if (strcmp(attr_name, "loop") == 0) {
                            node->loop = 1;
                        }
                        break;
                    case 'r':
                        if (strcmp(attr_name, "radius") == 0) {
                            node->radius = ParseUnit(attr_val);
                        }
                        break;
                    case 'x':
                        if (strcmp(attr_name, "x2") == 0) {
                            node->x2 = ParseUnit(attr_val);
                        }
                        break;
                    case 'y':
                        if (strcmp(attr_name, "y2") == 0) {
                            node->y2 = ParseUnit(attr_val);
                        }
                        break;
                    case 't':
                        if (strcmp(attr_name, "thickness") == 0) {
                            node->thickness = ParseUnit(attr_val);
                        }
                        break;
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
                node->text_content = DOMStrDup(trimmed);
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
