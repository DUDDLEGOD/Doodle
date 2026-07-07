#ifndef DAUDIO_H
#define DAUDIO_H

#include "raylib.h"

#define WAVE_SINE 0
#define WAVE_SQUARE 1
#define WAVE_TRIANGLE 2
#define WAVE_SAWTOOTH 3
#define WAVE_NOISE 4

typedef struct {
    float attack;  // seconds
    float decay;   // seconds
    float sustain; // level (0.0 to 1.0)
    float release; // seconds
} ADSREnvelope;

void InitSynth(void);
void CloseSynth(void);
void PlaySynthTone(float frequency, float duration, int wave_type, ADSREnvelope env);
void PlaySynthToneEx(
    float frequency, 
    float duration, 
    int wave_type, 
    ADSREnvelope env,
    float frequency_slide,
    float vibrato_speed,
    float vibrato_depth,
    float tremolo_speed,
    float tremolo_depth,
    float filter_cutoff,
    float pan
);
void AudioSynthCallback(void *buffer, unsigned int frames);

#endif // DAUDIO_H
