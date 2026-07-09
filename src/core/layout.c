#include "layout.h"
#include "engine_shared.h"
#include <string.h>
#include "cache.h"
#include <stdio.h>

// Pass 1: Bottom-up measurement of intrinsic sizes
void MeasureNode(UINode* node) {
    if (!node || !node->visible) return;

    StyleProps* s = &node->style;
    if (node->has_hover_style && node->currently_hovered) s = &node->hover_style;

    // Measure children first
    for (int i = 0; i < node->child_count; i++) {
        MeasureNode(node->children[i]);
    }

    float intrinsic_w = 0.0f;
    float intrinsic_h = 0.0f;

    if (node->type == NODE_TEXT || node->type == NODE_BUTTON) {
        if (node->text_content[0] != '\0') {
            float font_size = s->font_size;
            Font font = GetFontDefault();
            int has_custom_font = 0;
            if (s->font_path[0] != '\0') {
                font = GetCachedFont(s->font_path);
                if (font.texture.id > 0) has_custom_font = 1;
            }
            if (has_custom_font) {
                Vector2 text_size = MeasureTextEx(font, node->text_content, font_size, 1.0f);
                intrinsic_w = text_size.x;
                intrinsic_h = text_size.y;
            } else {
                intrinsic_w = (float)MeasureText(node->text_content, (int)font_size);
                intrinsic_h = font_size;
            }
            if (node->type == NODE_BUTTON) {
                intrinsic_w += 10.0f; // basic padding for default button
            }
        }
    } else if (node->type == NODE_IMAGE) {
        if (node->asset_path[0] != '\0') {
            Texture2D tex = GetCachedTexture(node->asset_path);
            if (tex.id > 0) {
                intrinsic_w = (float)tex.width;
                intrinsic_h = (float)tex.height;
            }
        }
    } else if (node->type == NODE_CIRCLE) {
        intrinsic_w = 2.0f * node->radius;
        intrinsic_h = 2.0f * node->radius;
    }

    // For FIT sizes, we calculate based on children
    float child_max_w = 0.0f;
    float child_max_h = 0.0f;
    float child_sum_w = 0.0f;
    float child_sum_h = 0.0f;

    for (int i = 0; i < node->child_count; i++) {
        UINode* child = node->children[i];
        if (!child->visible) continue;
        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) cs = &child->hover_style;
        if (cs->position_mode == POS_ABSOLUTE) continue;

        float cw = child->layout.width + cs->margin_left + cs->margin_right;
        float ch = child->layout.height + cs->margin_top + cs->margin_bottom;
        if (cw > child_max_w) child_max_w = cw;
        if (ch > child_max_h) child_max_h = ch;
        child_sum_w += cw;
        child_sum_h += ch;
    }

    if (s->width_type == SIZING_FIXED) {
        node->layout.width = s->width_value;
    } else if (s->width_type == SIZING_FIT) {
        if (s->flex_direction == DIR_ROW) node->layout.width = intrinsic_w > child_sum_w ? intrinsic_w : child_sum_w;
        else node->layout.width = intrinsic_w > child_max_w ? intrinsic_w : child_max_w;
        node->layout.width += s->padding_left + s->padding_right;
    } else {
        node->layout.width = 0.0f; // Percent or grow will be resolved top-down
    }

    if (s->height_type == SIZING_FIXED) {
        node->layout.height = s->height_value;
    } else if (s->height_type == SIZING_FIT) {
        if (s->flex_direction == DIR_ROW) node->layout.height = intrinsic_h > child_max_h ? intrinsic_h : child_max_h;
        else node->layout.height = intrinsic_h > child_sum_h ? intrinsic_h : child_sum_h;
        node->layout.height += s->padding_top + s->padding_bottom;
    } else {
        node->layout.height = 0.0f;
    }
}

// Pass 2: Top-down layout application
void LayoutNode(UINode* node, float avail_w, float avail_h) {
    if (!node || !node->visible) return;

    StyleProps* s = &node->style;
    if (node->has_hover_style && node->currently_hovered) s = &node->hover_style;

    // Resolve percent sizes based on parent available space
    if (s->width_type == SIZING_PERCENT) {
        node->layout.width = avail_w * (s->width_value / 100.0f);
    }
    if (s->height_type == SIZING_PERCENT) {
        node->layout.height = avail_h * (s->height_value / 100.0f);
    }

    // Calculate internal available space for children
    float inner_w = node->layout.width - (s->padding_left + s->padding_right);
    float inner_h = node->layout.height - (s->padding_top + s->padding_bottom);
    if (inner_w < 0.0f) inner_w = 0.0f;
    if (inner_h < 0.0f) inner_h = 0.0f;

    // First pass on children: evaluate non-grow sizes and count grow items
    int grow_count = 0;
    float non_grow_sum = 0.0f;
    for (int i = 0; i < node->child_count; i++) {
        UINode* child = node->children[i];
        if (!child->visible) continue;
        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) cs = &child->hover_style;
        if (cs->position_mode == POS_ABSOLUTE) continue;

        if (s->flex_direction == DIR_ROW) {
            if (cs->width_type == SIZING_GROW) grow_count++;
            else {
                if (cs->width_type == SIZING_PERCENT) child->layout.width = inner_w * (cs->width_value / 100.0f);
                non_grow_sum += child->layout.width + cs->margin_left + cs->margin_right;
            }
            if (cs->height_type == SIZING_PERCENT) child->layout.height = inner_h * (cs->height_value / 100.0f);
        } else {
            if (cs->height_type == SIZING_GROW) grow_count++;
            else {
                if (cs->height_type == SIZING_PERCENT) child->layout.height = inner_h * (cs->height_value / 100.0f);
                non_grow_sum += child->layout.height + cs->margin_top + cs->margin_bottom;
            }
            if (cs->width_type == SIZING_PERCENT) child->layout.width = inner_w * (cs->width_value / 100.0f);
        }
    }

    // Distribute grow space
    float grow_size = 0.0f;
    if (grow_count > 0) {
        float remaining = (s->flex_direction == DIR_ROW ? inner_w : inner_h) - non_grow_sum;
        if (remaining > 0.0f) grow_size = remaining / grow_count;
    }

    // Set grow sizes
    for (int i = 0; i < node->child_count; i++) {
        UINode* child = node->children[i];
        if (!child->visible) continue;
        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) cs = &child->hover_style;
        if (cs->position_mode == POS_ABSOLUTE) continue;

        if (s->flex_direction == DIR_ROW && cs->width_type == SIZING_GROW) {
            child->layout.width = grow_size - (cs->margin_left + cs->margin_right);
            if (child->layout.width < 0.0f) child->layout.width = 0.0f;
        } else if (s->flex_direction == DIR_COLUMN && cs->height_type == SIZING_GROW) {
            child->layout.height = grow_size - (cs->margin_top + cs->margin_bottom);
            if (child->layout.height < 0.0f) child->layout.height = 0.0f;
        }
    }

    // Calculate positions based on justify-content and align-items
    float child_start_x = node->layout.x + s->padding_left;
    float child_start_y = node->layout.y + s->padding_top;
    float gap = 0.0f;

    int rel_count = 0;
    float rel_sum = non_grow_sum + (grow_size * grow_count);
    for (int i = 0; i < node->child_count; i++) {
        UINode* child = node->children[i];
        if (!child->visible) continue;
        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) cs = &child->hover_style;
        if (cs->position_mode == POS_ABSOLUTE) continue;
        rel_count++;
    }

    float remaining_space = (s->flex_direction == DIR_ROW ? inner_w : inner_h) - rel_sum;
    if (remaining_space > 0.0f && s->justify_content[0] != '\0') {
        char j0 = s->justify_content[0];
        if (j0 == 'f') {
            if (s->justify_content[5] == 'e') {
                if (s->flex_direction == DIR_ROW) child_start_x += remaining_space;
                else child_start_y += remaining_space;
            }
        } else if (j0 == 'c') {
            if (s->flex_direction == DIR_ROW) child_start_x += remaining_space / 2.0f;
            else child_start_y += remaining_space / 2.0f;
        } else if (j0 == 's') {
            if (s->justify_content[6] == 'b' && rel_count > 1) {
                gap = remaining_space / (rel_count - 1);
            } else if (s->justify_content[6] == 'a' && rel_count > 0) {
                gap = remaining_space / rel_count;
                if (s->flex_direction == DIR_ROW) child_start_x += gap / 2.0f;
                else child_start_y += gap / 2.0f;
            }
        }
    }

    float current_x = child_start_x;
    float current_y = child_start_y;

    // Assign final coordinates and recurse
    for (int i = 0; i < node->child_count; i++) {
        UINode* child = node->children[i];
        if (!child->visible) continue;
        StyleProps* cs = &child->style;
        if (child->has_hover_style && child->currently_hovered) cs = &child->hover_style;

        if (cs->position_mode == POS_ABSOLUTE) {
            child->layout.x = node->layout.x + s->padding_left + cs->left + cs->margin_left;
            child->layout.y = node->layout.y + s->padding_top + cs->top + cs->margin_top;
            LayoutNode(child, inner_w, inner_h);
        } else {
            if (s->flex_direction == DIR_ROW) {
                current_x += cs->margin_left;
                float final_y = current_y + cs->margin_top;
                if (s->align_items[0] == 'c') {
                    float avail_h = inner_h - cs->margin_top - cs->margin_bottom;
                    final_y = current_y + cs->margin_top + (avail_h - child->layout.height) / 2.0f;
                }
                child->layout.x = current_x;
                child->layout.y = final_y;
                current_x += child->layout.width + cs->margin_right + gap;
            } else {
                current_y += cs->margin_top;
                float final_x = current_x + cs->margin_left;
                if (s->align_items[0] == 'c') {
                    float avail_w = inner_w - cs->margin_left - cs->margin_right;
                    final_x = current_x + cs->margin_left + (avail_w - child->layout.width) / 2.0f;
                }
                child->layout.x = final_x;
                child->layout.y = current_y;
                current_y += child->layout.height + cs->margin_bottom + gap;
            }
            LayoutNode(child, child->layout.width, child->layout.height);
        }
    }
}
