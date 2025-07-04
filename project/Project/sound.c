/*
 * sound.c - Modular Sound System for Boulder Scene
 *
 * This file implements the sound system for the 3D Boulder-inspired scene.
 * It is responsible for initializing the audio subsystem, loading and playing background music,
 * and providing simple controls for pausing, resuming, and cleaning up audio resources.
 *
 * Key Features:
 * - Uses SDL2 and SDL_mixer to provide cross-platform audio playback.
 * - Loads a music file (e.g., MP3, OGG, WAV) and plays it in a loop as ambient background sound.
 * - Provides functions to start, pause, resume, and stop music playback, as well as to clean up resources.
 * - Handles all error cases gracefully, printing detailed error messages if initialization or playback fails.
 * - Designed to be modular: the sound system can be initialized, controlled, and cleaned up independently of other systems.
 *
 * This file is ideal for demoing how to integrate audio into a graphics project, and for answering questions about
 * SDL audio management, resource handling, and modular system design.
 *
 * Original Implementation - took the methods from SDL2V2 class example.
 */

#include "sound.h"           // Header for sound system prototypes
#include <SDL2/SDL.h>         // SDL core library for initializing audio subsystem
#include <SDL2/SDL_mixer.h>   // SDL_mixer for music playback
#include <stdio.h>            // Standard I/O for error reporting

// =========================
// Global static variables for sound system state
// =========================
static Mix_Music* music = NULL; // Pointer to loaded music track (NULL if not loaded)
static int soundEnabled = 0;    // Flag: 1 if sound system is initialized and ready, 0 otherwise

// =========================
// Initializes the sound system and loads a music file.
// Returns 1 on success, 0 on failure.
// =========================
int soundInit(const char* filename)
{
    // The first step is to initialize the SDL audio subsystem. This is required before any audio can be played.
    // SDL_INIT_AUDIO tells SDL to set up audio drivers and resources.
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        // If initialization fails (e.g., no audio device, driver issue), print a detailed error message.
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }
    // Next, open the audio device with the desired format:
    // - 44100 Hz sample rate (CD quality)
    // - AUDIO_S16SYS: 16-bit signed samples, native endianness
    // - 2 channels (stereo)
    // - 4096 byte buffer for smooth streaming
    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096) < 0)
    {
        // If the audio device cannot be opened (e.g., unsupported format, busy device), print error.
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        return 0;
    }
    // Load the music file into memory. SDL_mixer supports many formats (MP3, OGG, WAV, etc.).
    // This function returns a pointer to the loaded music, or NULL on failure.
    music = Mix_LoadMUS(filename);
    if (!music)
    {
        // If the file cannot be loaded (e.g., not found, unsupported format), print error.
        fprintf(stderr, "Mix_LoadMUS failed: %s\n", Mix_GetError());
        return 0;
    }
    // If we reach here, everything succeeded. Mark the sound system as enabled.
    soundEnabled = 1;
    return 1;
}

// =========================
// Starts or resumes music playback if sound is enabled.
// If music is not already playing, starts it in a loop.
// If paused, resumes playback.
// =========================
void soundPlay()
{
    // Only play if the sound system is enabled and music is loaded.
    if (soundEnabled && music)
    {
        // Mix_PlayingMusic() returns 0 if no music is currently playing.
        if (Mix_PlayingMusic() == 0)
            Mix_PlayMusic(music, -1); // Start playback, -1 means loop forever for ambient background.
        else
            Mix_ResumeMusic(); // If music was paused, resume playback from where it left off.
    }
}

// =========================
// Pauses music playback if it is currently playing.
// Does not free resources or stop the music completely.
// =========================
void soundStop()
{
    // Only pause if the sound system is enabled and music is currently playing.
    if (soundEnabled && Mix_PlayingMusic())
        Mix_PauseMusic(); // Pause playback (can be resumed later with soundPlay)
}

// =========================
// Cleans up the sound system, stops music, frees resources, and shuts down audio subsystem.
// =========================
void soundCleanup()
{
    // If music is loaded, stop playback and free the resource.
    if (music)
    {
        Mix_HaltMusic();      // Stop any playing music immediately
        Mix_FreeMusic(music); // Free the loaded music resource
        music = NULL;         // Mark as unloaded
    }
    // Close the audio device to release system resources.
    Mix_CloseAudio();         // Close the audio device
    // Shut down the SDL audio subsystem. This is important to avoid resource leaks.
    SDL_QuitSubSystem(SDL_INIT_AUDIO); // Shut down SDL audio subsystem
    soundEnabled = 0;         // Mark sound system as disabled
} 