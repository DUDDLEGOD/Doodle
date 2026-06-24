#ifndef PARTICLES_H
#define PARTICLES_H

#include "raylib.h"

extern int particles_drawn_this_frame;

void SpawnParticles(float x, float y, int count, const char* color_hex, float speed, float lifetime);
void UpdateAndDrawParticles(void);

#endif // PARTICLES_H
