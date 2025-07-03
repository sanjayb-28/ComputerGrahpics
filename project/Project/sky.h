// ---------------------------------------------
// sky.h - Celestial object system for sun, moon, and sky rendering
// ---------------------------------------------

#ifndef CELESTIAL_H
#define CELESTIAL_H

// --- Celestial object data structures ---
/* Based on Real-Time Rendering 4th Ed. Ch. 10 */

/* Individual celestial body (sun/moon) */
typedef struct {
    float position[3];           // Position
    float size;                  // Diameter
    float brightness;            // Visibility (0-1)
    float color[4];              // RGBA color
} SkyObject;

/* Complete sky system containing celestial objects */
typedef struct {
    SkyObject sun;               // Sun
    SkyObject moon;              // Moon
    // Add more fields if needed for stars, planets, etc.
} SkySystem;

<<<<<<< Updated upstream
// --- Sky system API ---
/* Initializes sky system with default parameters */
void skySystemInit(SkySystem* sky);

/* Updates celestial body positions based on time of day */
void skySystemUpdate(SkySystem* sky, float dayTime);

/* Renders sun and moon as visible celestial bodies */
void skySystemRenderSunAndMoon(SkySystem* sky, float dayTime);

/* Configures scene lighting based on sun/moon positions */
void skySystemUpdateLighting(SkySystem* sky);

/* Frees resources used by the sky system */
void skySystemDestroy(SkySystem* sky);
=======
void skySystemInitialize(SkySystem* sky);

void skySystemAdvance(SkySystem* sky, float dayTime);

void skySystemRender(SkySystem* sky, float dayTime);

void skySystemApplyLighting(SkySystem* sky);
>>>>>>> Stashed changes

#endif