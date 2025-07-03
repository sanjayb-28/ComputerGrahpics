/*
 * Original Implementation - took the methods from SDL2V2 class example.
 * 
 */

#include "CSCIx229.h"
#include "sound.h"

static Mix_Music* ambienceMusic = NULL;
static int audioInitialized = 0;

int InitAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        return 0;
    }

    ambienceMusic = Mix_LoadMUS("sounds/forest-ambience.mp3");
    if (!ambienceMusic) {
        fprintf(stderr, "Cannot load forest-ambience.mp3: %s\n", Mix_GetError());
        return 0;
    }
    
    audioInitialized = 1;
    return 1;
}

void PlayAmbience() {
    if (audioInitialized && ambienceMusic) {
        Mix_PlayMusic(ambienceMusic, -1);
    }
}

void StopAmbience() {
    Mix_HaltMusic();
}

void CleanupAudio() {
    if (ambienceMusic) {
        Mix_FreeMusic(ambienceMusic);
        ambienceMusic = NULL;
    }
    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audioInitialized = 0;
} 