#include "daudio.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif
#define TWO_PI 6.283185307179586f
#define INV_TWO_PI 0.159154943091895f

#define MAX_VOICES 16
#define SAMPLE_RATE 44100

typedef struct {
    int active;
    float frequency;
    float phase;
    float phase_increment;
    float duration;
    float time_elapsed;
    int wave_type;
    ADSREnvelope env;
} SynthVoice;

static SynthVoice voices[MAX_VOICES];
static AudioStream synth_stream;
static int synth_initialized = 0;

void AudioSynthCallback(void *buffer, unsigned int frames) {
    float *out = (float *)buffer;
    memset(out, 0, frames * sizeof(float));

    const float inv_sample_rate = 1.0f / (float)SAMPLE_RATE;

    for (unsigned int f = 0; f < frames; f++) {
        float sample = 0.0f;

        for (int i = 0; i < MAX_VOICES; i++) {
            if (!voices[i].active) continue;

            voices[i].time_elapsed += inv_sample_rate;
            float t = voices[i].time_elapsed;

            // Calculate ADSR envelope multiplier
            float amplitude = 0.0f;
            ADSREnvelope env = voices[i].env;
            float total_duration = voices[i].duration;

            if (t < env.attack) {
                amplitude = t / env.attack;
            } else if (t < env.attack + env.decay) {
                float dt = (t - env.attack) / env.decay;
                amplitude = 1.0f - dt * (1.0f - env.sustain);
            } else if (t < total_duration) {
                amplitude = env.sustain;
            } else if (t < total_duration + env.release) {
                float dt = (t - total_duration) / env.release;
                amplitude = env.sustain * (1.0f - dt);
            } else {
                voices[i].active = 0;
                continue;
            }

            // Generate raw wave sample
            float val = 0.0f;
            float p = voices[i].phase;
            voices[i].phase += voices[i].phase_increment;
            if (voices[i].phase > TWO_PI) voices[i].phase -= TWO_PI;

            switch (voices[i].wave_type) {
                case WAVE_SINE:
                    val = sinf(p);
                    break;
                case WAVE_SQUARE:
                    val = (p < PI) ? 1.0f : -1.0f;
                    break;
                case WAVE_TRIANGLE:
                    val = 2.0f * fabsf(2.0f * p * INV_TWO_PI - 1.0f) - 1.0f;
                    break;
                case WAVE_SAWTOOTH:
                    val = 2.0f * p * INV_TWO_PI - 1.0f;
                    break;
                case WAVE_NOISE:
                    val = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                    break;
            }

            sample += val * amplitude * 0.15f; // Scale down to prevent clipping
        }

        // Clip / Normalize output mix
        if (sample > 1.0f) sample = 1.0f;
        else if (sample < -1.0f) sample = -1.0f;
        out[f] = sample;
    }
}

void InitSynth(void) {
    if (synth_initialized) return;
    memset(voices, 0, sizeof(voices));
    
    // Create mono float stream
    synth_stream = LoadAudioStream(SAMPLE_RATE, 32, 1);
    SetAudioStreamCallback(synth_stream, AudioSynthCallback);
    PlayAudioStream(synth_stream);
    
    synth_initialized = 1;
}

void CloseSynth(void) {
    if (!synth_initialized) return;
    UnloadAudioStream(synth_stream);
    synth_initialized = 0;
}

void PlaySynthTone(float frequency, float duration, int wave_type, ADSREnvelope env) {
    if (!synth_initialized) return;

    // Find free voice
    int idx = -1;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i].active) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        // Steal the oldest voice if full
        idx = 0;
    }

    voices[idx].active = 1;
    voices[idx].frequency = frequency;
    voices[idx].phase = 0.0f;
    voices[idx].phase_increment = (TWO_PI * frequency) / (float)SAMPLE_RATE;
    voices[idx].duration = duration;
    voices[idx].time_elapsed = 0.0f;
    voices[idx].wave_type = wave_type;
    voices[idx].env = env;
}
