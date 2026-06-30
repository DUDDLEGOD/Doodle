#include "renderer.h"
#include "engine_shared.h"
#include "cache.h"
#include "particles.h"
#include <string.h>

static Shader current_active_shader = {0};

#define MAX_SORT_NODES 1024

void SetActiveShader(Shader sh) {
    if (sh.id != current_active_shader.id) {
        if (current_active_shader.id > 0) {
            EndShaderMode();
        }
        if (sh.id > 0) {
            BeginShaderMode(sh);
        }
        current_active_shader = sh;
    }
}

static inline int GetActiveZIndex(UINode* n) {
    if (!n) return 0;
    if (n->has_hover_style && n->currently_hovered) {
        return n->hover_style.z_index;
    }
    return n->style.z_index;
}

void GetSortedChildren(UINode* node, UINode** sorted_list) {
    for (int i = 0; i < node->child_count; i++) {
        sorted_list[i] = node->children[i];
    }
    for (int i = 1; i < node->child_count; i++) {
        UINode* key = sorted_list[i];
        int key_z = GetActiveZIndex(key);
        int j = i - 1;
        while (j >= 0 && GetActiveZIndex(sorted_list[j]) > key_z) {
            sorted_list[j + 1] = sorted_list[j];
            j = j - 1;
        }
        sorted_list[j + 1] = key;
    }
}

// Drawing Traversal
void DrawUINode(UINode* node) {
    if (!node || !node->visible) return;

    if (node->parent == NULL) {
        current_active_shader = (Shader){0};
    }

    StyleProps* active_style = &node->style;
    if (node->has_hover_style && node->currently_hovered) {
        active_style = &node->hover_style;
    }

    if (node->use_camera) {
        int has_shader = (active_style->shader_path && active_style->shader_path[0] != '\0');
        if (has_shader) {
            static RenderTexture2D arena_target;
            static int arena_target_created = 0;
            static int arena_tex_w = 0, arena_tex_h = 0;

            int cur_w = (int)node->layout.width;
            int cur_h = (int)node->layout.height;
            if (cur_w < 1) cur_w = 800;
            if (cur_h < 1) cur_h = 600;

            if (!arena_target_created || cur_w != arena_tex_w || cur_h != arena_tex_h) {
                if (arena_target_created) UnloadRenderTexture(arena_target);
                arena_target = LoadRenderTexture(cur_w, cur_h);
                arena_tex_w = cur_w;
                arena_tex_h = cur_h;
                arena_target_created = 1;
            }

            Shader prev_shader = current_active_shader;
            SetActiveShader((Shader){0});

            BeginTextureMode(arena_target);
            ClearBackground(BLACK);

            if (active_style->bg_color.a > 0) {
                DrawRectangle(0, 0, cur_w, cur_h, active_style->bg_color);
            }

            Camera2D current_cam = ctx.camera;
            current_cam.target = (Vector2){ node->layout.x, node->layout.y };
            current_cam.offset = (Vector2){ 0.0f, 0.0f };
            current_cam.zoom = 1.0f;
            if (ctx.shake_duration > 0.0f) {
                current_cam.offset.x += GetRandomValue(-ctx.shake_intensity, ctx.shake_intensity);
                current_cam.offset.y += GetRandomValue(-ctx.shake_intensity, ctx.shake_intensity);
                ctx.shake_duration -= GetFrameTime();
            }
            BeginMode2D(current_cam);

            if (node->child_count > 0) {
                int N = node->child_count;
                if (N > MAX_SORT_NODES) N = MAX_SORT_NODES;
                UINode* sorted_children[MAX_SORT_NODES];
                for (int i = 0; i < N; i++) sorted_children[i] = node->children[i];
                for (int i = 1; i < N; i++) {
                    UINode* key = sorted_children[i];
                    int key_z = GetActiveZIndex(key);
                    int j = i - 1;
                    while (j >= 0 && GetActiveZIndex(sorted_children[j]) > key_z) {
                        sorted_children[j + 1] = sorted_children[j];
                        j = j - 1;
                    }
                    sorted_children[j + 1] = key;
                }
                for (int i = 0; i < N; i++) DrawUINode(sorted_children[i]);
            }

            UpdateAndDrawParticles();

            EndMode2D();
            EndTextureMode();

            Shader sh = GetCachedShader(active_style->shader_path);
            if (sh.id > 0) {
                SetActiveShader(sh);
            }

            Rectangle src = { 0, 0, (float)arena_target.texture.width, -(float)arena_target.texture.height };
            Rectangle dest = { node->layout.x, node->layout.y, node->layout.width, node->layout.height };
            DrawTexturePro(arena_target.texture, src, dest, (Vector2){0,0}, 0.0f, WHITE); ctx.g_draw_calls++;

            SetActiveShader(prev_shader);

            if (active_style->border_width > 0 && active_style->border_color.a > 0) {
                Rectangle border_rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
                DrawRectangleLinesEx(border_rec, active_style->border_width, active_style->border_color); ctx.g_draw_calls++;
            }

            if (node->parent == NULL) {
                SetActiveShader((Shader){0});
            }
        } else {
            Camera2D current_cam = ctx.camera;
            current_cam.target = (Vector2){ node->layout.x, node->layout.y };
            current_cam.offset = (Vector2){ node->layout.x, node->layout.y };
            current_cam.zoom = 1.0f;
            if (ctx.shake_duration > 0.0f) {
                current_cam.offset.x += GetRandomValue(-ctx.shake_intensity, ctx.shake_intensity);
                current_cam.offset.y += GetRandomValue(-ctx.shake_intensity, ctx.shake_intensity);
                ctx.shake_duration -= GetFrameTime();
            }
            BeginMode2D(current_cam);

            if (active_style->bg_color.a > 0) {
                DrawRectangle(node->layout.x, node->layout.y, node->layout.width, node->layout.height, active_style->bg_color);
            }

            if (node->child_count > 0) {
                int N = node->child_count;
                if (N > MAX_SORT_NODES) N = MAX_SORT_NODES;
                UINode* sorted_children[MAX_SORT_NODES];
                for (int i = 0; i < N; i++) sorted_children[i] = node->children[i];
                for (int i = 1; i < N; i++) {
                    UINode* key = sorted_children[i];
                    int key_z = GetActiveZIndex(key);
                    int j = i - 1;
                    while (j >= 0 && GetActiveZIndex(sorted_children[j]) > key_z) {
                        sorted_children[j + 1] = sorted_children[j];
                        j = j - 1;
                    }
                    sorted_children[j + 1] = key;
                }
                for (int i = 0; i < N; i++) DrawUINode(sorted_children[i]);
            }

            UpdateAndDrawParticles();

            EndMode2D();

            if (active_style->border_width > 0 && active_style->border_color.a > 0) {
                Rectangle border_rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
                DrawRectangleLinesEx(border_rec, active_style->border_width, active_style->border_color); ctx.g_draw_calls++;
            }
        }
        return;
    }

    Shader prev_shader = current_active_shader;
    int has_shader = (active_style->shader_path && active_style->shader_path[0] != '\0');
    if (has_shader) {
        Shader sh = GetCachedShader(active_style->shader_path);
        if (sh.id > 0) {
            SetActiveShader(sh);
        }
    }

    if (node->type == NODE_VIEW) {
        if (active_style->bg_color.a > 0) {
            if (active_style->border_radius > 0) {
                Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
                float min_dim = node->layout.width > node->layout.height ? node->layout.height : node->layout.width;
                float roundness = min_dim > 0 ? (active_style->border_radius / min_dim) : 0.0f;
                if (roundness > 1.0f) roundness = 1.0f;
                DrawRectangleRounded(rec, roundness, 8, active_style->bg_color); ctx.g_draw_calls++;
            } else {
                DrawRectangle(node->layout.x, node->layout.y, node->layout.width, node->layout.height, active_style->bg_color); ctx.g_draw_calls++;
            }
        }
    } else if (node->type == NODE_TEXT || node->type == NODE_BUTTON) {
        if (node->type == NODE_BUTTON && active_style->bg_color.a > 0) {
            if (active_style->border_radius > 0) {
                Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
                float min_dim = node->layout.width > node->layout.height ? node->layout.height : node->layout.width;
                float roundness = min_dim > 0 ? (active_style->border_radius / min_dim) : 0.0f;
                if (roundness > 1.0f) roundness = 1.0f;
                DrawRectangleRounded(rec, roundness, 8, active_style->bg_color); ctx.g_draw_calls++;
            } else {
                DrawRectangle(node->layout.x, node->layout.y, node->layout.width, node->layout.height, active_style->bg_color); ctx.g_draw_calls++;
            }
        }

        const char* text = node->text_content;
        if (text && text[0] != '\0') {
            float font_size = active_style->font_size;
            Color text_color = active_style->text_color;
            Font font = GetFontDefault();
            int has_custom_font = 0;
            if (active_style->font_path && active_style->font_path[0] != '\0') {
                font = GetCachedFont(active_style->font_path);
                if (font.texture.id > 0) has_custom_font = 1;
            }

            Vector2 text_size;
            if (has_custom_font) {
                text_size = MeasureTextEx(font, text, font_size, 1.0f);
            } else {
                text_size = (Vector2){ (float)MeasureText(text, (int)font_size), font_size };
            }

            float draw_x = node->layout.x;
            float draw_y = node->layout.y;

            if (strcmp(active_style->text_align, "center") == 0) {
                draw_x = node->layout.x + (node->layout.width - text_size.x) / 2.0f;
                draw_y = node->layout.y + (node->layout.height - text_size.y) / 2.0f;
            } else if (node->type == NODE_BUTTON) {
                draw_x = node->layout.x + 5;
                draw_y = node->layout.y + (node->layout.height - text_size.y) / 2.0f;
            }

            if (has_custom_font) {
                DrawTextEx(font, text, (Vector2){draw_x, draw_y}, font_size, 1.0f, text_color); ctx.g_draw_calls++;
            } else {
                DrawText(text, (int)draw_x, (int)draw_y, (int)font_size, text_color); ctx.g_draw_calls++;
            }
        }
    } else if (node->type == NODE_IMAGE) {
        if (node->asset_path && node->asset_path[0] != '\0') {
            Texture2D tex = GetCachedTexture(node->asset_path);
            if (tex.id > 0) {
                Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
                Rectangle dest = {
                    node->layout.x + node->layout.width / 2.0f,
                    node->layout.y + node->layout.height / 2.0f,
                    node->layout.width,
                    node->layout.height
                };
                Vector2 origin = {node->layout.width / 2.0f, node->layout.height / 2.0f};
                Color tint = (active_style->tint_color.a > 0) ? active_style->tint_color : WHITE;
                DrawTexturePro(tex, src, dest, origin, active_style->rotation, tint); ctx.g_draw_calls++;
            }
        }
    } else if (node->type == NODE_CIRCLE) {
        float r = node->radius;
        if (r <= 0) r = node->layout.width / 2.0f;
        DrawCircle(node->layout.x + r, node->layout.y + r, r, node->shape_color); ctx.g_draw_calls++;
    } else if (node->type == NODE_LINE) {
        DrawLineEx((Vector2){node->layout.x, node->layout.y}, (Vector2){node->layout.x + node->x2, node->layout.y + node->y2}, node->thickness, node->shape_color); ctx.g_draw_calls++;
    }

    // Border drawing
    if (active_style->border_width > 0 && active_style->border_color.a > 0) {
        Rectangle rec = {node->layout.x, node->layout.y, node->layout.width, node->layout.height};
        if (active_style->border_radius > 0) {
            float min_dim = node->layout.width > node->layout.height ? node->layout.height : node->layout.width;
            float roundness = min_dim > 0 ? (active_style->border_radius / min_dim) : 0.0f;
            if (roundness > 1.0f) roundness = 1.0f;
            DrawRectangleRoundedLines(rec, roundness, 8, active_style->border_width, active_style->border_color); ctx.g_draw_calls++;
        } else {
            DrawRectangleLinesEx(rec, active_style->border_width, active_style->border_color); ctx.g_draw_calls++;
        }
    }

    if (node->child_count > 0) {
        int N = node->child_count;
            if (N > MAX_SORT_NODES) N = MAX_SORT_NODES;
            UINode* sorted_children[MAX_SORT_NODES];

            for (int i = 0; i < N; i++) {
                sorted_children[i] = node->children[i];
            }
            for (int i = 1; i < N; i++) {
                UINode* key = sorted_children[i];
                int key_z = GetActiveZIndex(key);
                int j = i - 1;
                while (j >= 0 && GetActiveZIndex(sorted_children[j]) > key_z) {
                    sorted_children[j + 1] = sorted_children[j];
                    j = j - 1;
                }
                sorted_children[j + 1] = key;
            }

            for (int i = 0; i < N; i++) {
                DrawUINode(sorted_children[i]);
            }
    }

    if (has_shader) {
        SetActiveShader(prev_shader);
    }

    if (node->parent == NULL) {
        SetActiveShader((Shader){0});
    }
}
