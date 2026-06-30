#include "particles.h"
#include "color.h"
#include "fast_math.h"

#include "engine_shared.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#ifndef DEG2RAD
#define DEG2RAD (PI/180.0f)
#endif

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float size;
    float lifetime;
    float inv_max_lifetime;
} Particle;

#define MAX_PARTICLES 512
static Particle particle_pool[MAX_PARTICLES];
static int particle_count = 0;
int particles_drawn_this_frame = 0;

void SpawnParticles(float x, float y, int count, const char* color_hex, float speed, float lifetime) {
    Color col = ParseColor(color_hex);
    float inv_life = (lifetime > 0.0f) ? (1.0f / lifetime) : 0.0f;
    
    for (int i = 0; i < count; i++) {
        int idx = -1;
        if (particle_count < MAX_PARTICLES) {
            idx = particle_count++;
        } else {
            idx = GetRandomValue(0, MAX_PARTICLES - 1);
        }
        
        if (idx != -1) {
            particle_pool[idx].position = (Vector2){x, y};
            float angle = GetRandomValue(0, 360) * DEG2RAD;
            float current_speed = GetRandomValue(20, 100) * 0.01f * speed;
            particle_pool[idx].velocity = (Vector2){ fast_cos(angle) * current_speed, fast_sin(angle) * current_speed };
            particle_pool[idx].color = col;
            particle_pool[idx].size = (float)GetRandomValue(2, 6);
            particle_pool[idx].lifetime = lifetime;
            particle_pool[idx].inv_max_lifetime = inv_life;
        }
    }
}

void UpdateAndDrawParticles(void) {
    if (particles_drawn_this_frame) return;
    particles_drawn_this_frame = 1;
    float dt = GetFrameTime();
    int active_idx = 0;
    for (int i = 0; i < particle_count; i++) {
        particle_pool[i].lifetime -= dt;
        if (particle_pool[i].lifetime > 0.0f) {
            particle_pool[i].position.x += particle_pool[i].velocity.x * dt * 60.0f;
            particle_pool[i].position.y += particle_pool[i].velocity.y * dt * 60.0f;
            
            Color c = particle_pool[i].color;
            c.a = (unsigned char)(255.0f * particle_pool[i].lifetime * particle_pool[i].inv_max_lifetime);
            DrawRectangleV(particle_pool[i].position, (Vector2){particle_pool[i].size, particle_pool[i].size}, c); ctx.g_draw_calls++;
            
            if (active_idx != i) {
                particle_pool[active_idx] = particle_pool[i];
            }
            active_idx++;
        }
    }
    particle_count = active_idx;
}

int GetActiveParticleCount(void) {
    return particle_count;
}
