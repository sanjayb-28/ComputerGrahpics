/*
 * Original Implementation - took the methods from SDL2V2 class example.
 * 
 */

#include "sound.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>

static Mix_Music* music = NULL;
static int soundEnabled = 0;

int soundInit(const char* filename)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }
    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096) < 0)
    {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        return 0;
    }
    music = Mix_LoadMUS(filename);
    if (!music)
    {
        fprintf(stderr, "Mix_LoadMUS failed: %s\n", Mix_GetError());
        return 0;
    }
    soundEnabled = 1;
    return 1;
}

void soundPlay()
{
    if (soundEnabled && music)
    {
        if (Mix_PlayingMusic() == 0)
            Mix_PlayMusic(music, -1);
        else
            Mix_ResumeMusic();
    }
}

void soundStop()
{
    if (soundEnabled && Mix_PlayingMusic())
        Mix_PauseMusic();
}

void soundCleanup()
{
    if (music)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(music);
        music = NULL;
    }
    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    soundEnabled = 0;
} 