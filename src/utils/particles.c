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

static unsigned int particle_rand_seed = 987654321U;

void SpawnParticles(float x, float y, int count, const char* color_hex, float speed, float lifetime) {
    Color col = ParseColor(color_hex);
    float inv_life = (lifetime > 0.0f) ? (1.0f / lifetime) : 0.0f;
    
    for (int i = 0; i < count; i++) {
        int idx = -1;
        if (particle_count < MAX_PARTICLES) {
            idx = particle_count++;
        } else {
            float r = fast_rand_f(&particle_rand_seed) * 0.5f + 0.5f;
            idx = (int)(r * (float)(MAX_PARTICLES - 1));
        }
        
        if (idx != -1) {
            particle_pool[idx].position = (Vector2){x, y};
            float r_ang = fast_rand_f(&particle_rand_seed) * 0.5f + 0.5f;
            float angle = r_ang * TWO_PI;
            float r_sp = fast_rand_f(&particle_rand_seed) * 0.5f + 0.5f;
            float current_speed = (0.20f + 0.80f * r_sp) * speed;
            particle_pool[idx].velocity = (Vector2){ fast_cos(angle) * current_speed, fast_sin(angle) * current_speed };
            particle_pool[idx].color = col;
            float r_sz = fast_rand_f(&particle_rand_seed) * 0.5f + 0.5f;
            particle_pool[idx].size = 2.0f + 4.0f * r_sz;
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
