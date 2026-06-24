#include "layout.h"
#include "engine_shared.h"
#include <string.h>

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

    if (node->type == NODE_CIRCLE) {
        if (s->width_type == SIZING_FIT) {
            node->layout.width = 2.0f * node->radius;
        }
        if (s->height_type == SIZING_FIT) {
            node->layout.height = 2.0f * node->radius;
        }
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
        if (!child->visible) continue;

        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) {
            cs = &child->hover_style;
        }
        if (cs->position_mode == POS_ABSOLUTE) continue;

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
            if (!child->visible) continue;

            StyleProps* cs = &child->style;
            if (child->has_hover_style && child->currently_hovered) {
                cs = &child->hover_style;
            }
            if (cs->position_mode == POS_ABSOLUTE) continue;

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
        if (!child->visible) continue;
        
        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) {
            cs = &child->hover_style;
        }
        if (cs->position_mode == POS_ABSOLUTE) continue;
        
        rel_count++;

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
