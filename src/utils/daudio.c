#include "daudio.h"
#include "fast_math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#undef CloseWindow
#undef ShowCursor
#undef PlaySound
#undef DrawText

typedef CRITICAL_SECTION SynthMutex;
#define SynthMutexInit(m) InitializeCriticalSection(m)
#define SynthMutexLock(m) EnterCriticalSection(m)
#define SynthMutexUnlock(m) LeaveCriticalSection(m)
#define SynthMutexDestroy(m) DeleteCriticalSection(m)
#else
#include <pthread.h>
typedef pthread_mutex_t SynthMutex;
#define SynthMutexInit(m) pthread_mutex_init(m, NULL)
#define SynthMutexLock(m) pthread_mutex_lock(m)
#define SynthMutexUnlock(m) pthread_mutex_unlock(m)
#define SynthMutexDestroy(m) pthread_mutex_destroy(m)
#endif

static unsigned int synth_rand_seed = 123456789U;

#ifndef PI
#define PI 3.14159265358979323846f
#endif

typedef enum {
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE,
    ENV_DONE
} EnvStage;

#define MAX_VOICES 16
#define SAMPLE_RATE 44100
#define SIN_LUT_SIZE 1024

typedef struct {
    int active;
    float frequency;
    float phase;
    float phase_increment;
    float duration;
    int wave_type;
    ADSREnvelope env;

    // Fast incremental ADSR state
    EnvStage env_stage;
    int env_samples_left;
    float env_step;
    float env_value;

    // Pitch slide (Hz / second)
    float frequency_slide;

    // Vibrato (Pitch LFO)
    float vibrato_speed;
    float vibrato_depth;
    float vibrato_phase;

    // Tremolo (Amplitude LFO)
    float tremolo_speed;
    float tremolo_depth;
    float tremolo_phase;

    // Resonant Low-Pass Filter
    int filter_enabled;
    float filter_alpha;
    float filter_state;

    // Stereo Panning
    float pan;
    float left_gain;
    float right_gain;
} SynthVoice;

static SynthVoice voices[MAX_VOICES];
static AudioStream synth_stream;
static int synth_initialized = 0;
static float sin_lut[SIN_LUT_SIZE + 1];
static SynthMutex synth_mutex;

static inline float lookup_sin(float phase) {
    float idx_f = phase * (float)SIN_LUT_SIZE;
    int idx = (int)idx_f;
    float frac = idx_f - (float)idx;
    if (idx < 0) idx = 0;
    else if (idx >= SIN_LUT_SIZE) idx = SIN_LUT_SIZE - 1;
    return sin_lut[idx] + frac * (sin_lut[idx + 1] - sin_lut[idx]);
}

static void TransitionVoiceStage(SynthVoice *voice) {
    switch (voice->env_stage) {
        case ENV_ATTACK: {
            voice->env_stage = ENV_DECAY;
            int decay_samples = (int)(voice->env.decay * (float)SAMPLE_RATE);
            if (decay_samples > 0) {
                voice->env_value = 1.0f;
                voice->env_samples_left = decay_samples;
                voice->env_step = (voice->env.sustain - 1.0f) / (float)decay_samples;
                break;
            }
            // Fall through if decay is 0
        }
        case ENV_DECAY: {
            voice->env_stage = ENV_SUSTAIN;
            voice->env_value = voice->env.sustain;
            float sustain_time = voice->duration - voice->env.attack - voice->env.decay;
            if (sustain_time < 0.0f) sustain_time = 0.0f;
            int sustain_samples = (int)(sustain_time * (float)SAMPLE_RATE);
            if (sustain_samples > 0) {
                voice->env_samples_left = sustain_samples;
                voice->env_step = 0.0f;
                break;
            }
            // Fall through if sustain duration is 0
        }
        case ENV_SUSTAIN: {
            voice->env_stage = ENV_RELEASE;
            int release_samples = (int)(voice->env.release * (float)SAMPLE_RATE);
            if (release_samples > 0) {
                voice->env_samples_left = release_samples;
                voice->env_step = -voice->env_value / (float)release_samples;
            } else {
                voice->env_value = 0.0f;
                voice->env_stage = ENV_DONE;
                voice->active = 0;
            }
            break;
        }
        case ENV_RELEASE:
        default: {
            voice->env_value = 0.0f;
            voice->env_stage = ENV_DONE;
            voice->active = 0;
            break;
        }
    }
}

void AudioSynthCallback(void *buffer, unsigned int frames) {
    float *out = (float *)buffer;
    memset(out, 0, frames * 2 * sizeof(float));

    const float inv_sample_rate = 1.0f / (float)SAMPLE_RATE;

    SynthMutexLock(&synth_mutex);

    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i].active) continue;

        SynthVoice *v = &voices[i];
        
        // Cache voice parameters in registers for performance
        float freq = v->frequency;
        float phase = v->phase;
        float freq_slide = v->frequency_slide;
        
        float vib_speed = v->vibrato_speed;
        float vib_depth = v->vibrato_depth;
        float vib_phase = v->vibrato_phase;
        
        float trem_speed = v->tremolo_speed;
        float trem_depth = v->tremolo_depth;
        float trem_phase = v->tremolo_phase;
        
        int filter_enabled = v->filter_enabled;
        float filter_alpha = v->filter_alpha;
        float filter_state = v->filter_state;
        
        float left_gain = v->left_gain;
        float right_gain = v->right_gain;
        
        int wave_type = v->wave_type;

        // Envelope state variables
        float env_val = v->env_value;
        float env_step = v->env_step;
        int env_left = v->env_samples_left;

        for (unsigned int f = 0; f < frames; f++) {
            // 1. Update pitch slide
            if (freq_slide != 0.0f) {
                freq += freq_slide * inv_sample_rate;
                if (freq < 20.0f) freq = 20.0f;
                else if (freq > 20000.0f) freq = 20000.0f;
            }

            // 2. Update Vibrato (Pitch LFO)
            float current_freq = freq;
            if (vib_depth > 0.0f) {
                vib_phase += vib_speed * inv_sample_rate;
                if (vib_phase >= 1.0f) vib_phase -= 1.0f;
                current_freq += vib_depth * lookup_sin(vib_phase);
                if (current_freq < 20.0f) current_freq = 20.0f;
            }

            // 3. Update main oscillator phase
            float phase_inc = current_freq * inv_sample_rate;
            phase += phase_inc;
            if (phase >= 1.0f) phase -= 1.0f;

            // 4. Generate raw wave sample
            float val = 0.0f;
            switch (wave_type) {
                case WAVE_SINE:
                    val = lookup_sin(phase);
                    break;
                case WAVE_SQUARE:
                    val = (phase < 0.5f) ? 1.0f : -1.0f;
                    break;
                case WAVE_TRIANGLE:
                    val = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
                    break;
                case WAVE_SAWTOOTH:
                    val = 2.0f * phase - 1.0f;
                    break;
                case WAVE_NOISE:
                    val = fast_rand_f(&synth_rand_seed);
                    break;
            }

            // 5. Apply Low-Pass Filter
            if (filter_enabled) {
                filter_state += filter_alpha * (val - filter_state);
                val = filter_state;
            }

            // 6. Update ADSR envelope
            env_val += env_step;
            if (--env_left <= 0) {
                v->env_value = env_val;
                v->env_samples_left = env_left;
                TransitionVoiceStage(v);
                
                env_val = v->env_value;
                env_step = v->env_step;
                env_left = v->env_samples_left;
                
                if (!v->active) {
                    break;
                }
            }

            // 7. Update Tremolo (Amplitude LFO)
            float final_amp = env_val;
            if (trem_depth > 0.0f) {
                trem_phase += trem_speed * inv_sample_rate;
                if (trem_phase >= 1.0f) trem_phase -= 1.0f;
                final_amp *= (1.0f - trem_depth * (0.5f + 0.5f * lookup_sin(trem_phase)));
            }

            // 8. Scale volume and mix into stereo output buffer (L/R interleaved)
            float sample_out = val * final_amp * 0.15f;
            out[f * 2] += sample_out * left_gain;
            out[f * 2 + 1] += sample_out * right_gain;
        }

        // Save registers back to voice structure
        v->frequency = freq;
        v->phase = phase;
        v->vibrato_phase = vib_phase;
        v->tremolo_phase = trem_phase;
        v->filter_state = filter_state;
        v->env_value = env_val;
        v->env_samples_left = env_left;
    }

    // Clip / Normalize output mix to prevent clipping
    for (unsigned int f = 0; f < frames * 2; f++) {
        if (out[f] > 1.0f) out[f] = 1.0f;
        else if (out[f] < -1.0f) out[f] = -1.0f;
    }

    SynthMutexUnlock(&synth_mutex);
}

void InitSynth(void) {
    if (synth_initialized) return;
    memset(voices, 0, sizeof(voices));
    
    SynthMutexInit(&synth_mutex);

    // Initialize sine lookup table
    for (int i = 0; i <= SIN_LUT_SIZE; i++) {
        sin_lut[i] = sinf(((float)i / (float)SIN_LUT_SIZE) * 2.0f * PI);
    }

    // Create stereo float stream (32 bits, 2 channels)
    synth_stream = LoadAudioStream(SAMPLE_RATE, 32, 2);
    SetAudioStreamCallback(synth_stream, AudioSynthCallback);
    PlayAudioStream(synth_stream);
    
    synth_initialized = 1;
}

void CloseSynth(void) {
    if (!synth_initialized) return;
    UnloadAudioStream(synth_stream);
    SynthMutexDestroy(&synth_mutex);
    synth_initialized = 0;
}

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
) {
    if (!synth_initialized) return;

    SynthMutexLock(&synth_mutex);

    // Find free voice
    int idx = -1;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i].active) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        idx = 0; // Steal the oldest voice if full
    }

    SynthVoice *v = &voices[idx];
    memset(v, 0, sizeof(SynthVoice));

    v->active = 1;
    v->frequency = frequency;
    v->phase = 0.0f;
    v->phase_increment = frequency / (float)SAMPLE_RATE;
    v->duration = duration;
    v->wave_type = wave_type;
    v->env = env;

    // Pitch slide
    v->frequency_slide = frequency_slide;

    // Vibrato
    v->vibrato_speed = vibrato_speed;
    v->vibrato_depth = vibrato_depth;
    v->vibrato_phase = 0.0f;

    // Tremolo
    v->tremolo_speed = tremolo_speed;
    v->tremolo_depth = tremolo_depth;
    v->tremolo_phase = 0.0f;

    // Filter
    if (filter_cutoff > 0.0f && filter_cutoff < 20000.0f) {
        v->filter_enabled = 1;
        float alpha = (2.0f * PI * filter_cutoff) / (float)SAMPLE_RATE;
        if (alpha > 1.0f) alpha = 1.0f;
        else if (alpha < 0.0f) alpha = 0.0f;
        v->filter_alpha = alpha;
        v->filter_state = 0.0f;
    } else {
        v->filter_enabled = 0;
        v->filter_alpha = 1.0f;
        v->filter_state = 0.0f;
    }

    // Panning (Constant Power Panning)
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    v->pan = pan;
    float angle = (pan + 1.0f) * (PI / 4.0f);
    v->left_gain = cosf(angle);
    v->right_gain = sinf(angle);

    // Initial Envelope Stage setup
    v->env_stage = ENV_ATTACK;
    v->env_value = 0.0f;
    int attack_samples = (int)(env.attack * (float)SAMPLE_RATE);
    if (attack_samples > 0) {
        v->env_samples_left = attack_samples;
        v->env_step = 1.0f / (float)attack_samples;
    } else {
        TransitionVoiceStage(v);
    }

    SynthMutexUnlock(&synth_mutex);
}

void PlaySynthTone(float frequency, float duration, int wave_type, ADSREnvelope env) {
    PlaySynthToneEx(frequency, duration, wave_type, env, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}
