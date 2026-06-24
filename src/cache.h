#ifndef CACHE_H
#define CACHE_H

#include "raylib.h"
#include "mparser.h"

Texture2D GetCachedTexture(const char* path);
void UnloadCachedTextures(void);

Shader GetCachedShader(const char* path);
void UnloadCachedShaders(void);

Font GetCachedFont(const char* path);
void UnloadCachedFonts(void);

Sound GetCachedSound(const char* path);
void UnloadCachedSounds(void);

void SetupAudioNodes(UINode* node);
void UpdateMusicStreams(void);
void UnloadActiveMusics(void);

#endif // CACHE_H
