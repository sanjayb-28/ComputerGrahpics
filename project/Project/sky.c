/*
 * sky.c - Procedural Sky System for Boulder Scene
 *
 * This file implements the procedural sky system for the 3D Boulder-inspired scene.
 * It is responsible for simulating the sun and moon, animating their movement across the sky,
 * and dynamically adjusting scene lighting to match the time of day. The system renders the sun and moon
 * as glowing spheres, blending their influence to create smooth transitions between day and night.
 *
 * Key Features:
 * - Simulates the sun and moon as celestial bodies moving in a circular arc above the landscape.
 * - Computes their positions and brightness based on the time of day, so the sun rises, sets, and the moon follows.
 * - Dynamically blends scene lighting (direction, color, ambient/diffuse) between sun and moon for realistic dawn/dusk.
 * - Renders the sun and moon as glowing spheres using OpenGL emission, so they appear as light sources.
 * - Designed to be modular: the sky system can be initialized, advanced, and rendered independently.
 *
 * The code is structured for clarity and extensibility, with detailed comments explaining every step.
 * This file is ideal for demoing procedural sky and lighting logic, and for answering questions about
 * OpenGL state management, lighting, and procedural animation.
 *
 * Sphere Implementation based on:
 *   - https://www.songho.ca/opengl/gl_sphere.html
 *
 * All code is my own original work, except the section marked as AI
 */

#include "CSCIx229.h"      // Custom header for OpenGL and utility functions
#include "sky.h"           // Header for sky system types and prototypes
#include "landscape.h"     // Needed for LANDSCAPE_SCALE constant

// =========================
// Renders a simple sphere using quad strips.
// Used for sun and moon bodies in the sky.
// =========================
static void renderSimpleSphere(float radius) {
    // We use quads to approximate the sphere's surface. Each quad is defined by four vertices.
    // The sphere is built from latitude and longitude bands, like lines of latitude/longitude on a globe.
    glBegin(GL_QUADS); // Start drawing quads for sphere surface. Quads are used for simplicity and efficiency in this context.
    for (int i = 0; i < 16; i++) { // Loop over latitude bands (vertical slices of the sphere)
        for (int j = 0; j < 16; j++) { // Loop over longitude bands (horizontal slices)
            // Compute latitude and longitude for the four corners of the quad
            float lat1 = M_PI * (-0.5f + (float)i / 16); // Lower latitude (from -pi/2 to +pi/2)
            float lat2 = M_PI * (-0.5f + (float)(i + 1) / 16); // Upper latitude
            float lng1 = 2 * M_PI * (float)j / 16; // Left longitude (from 0 to 2pi)
            float lng2 = 2 * M_PI * (float)(j + 1) / 16; // Right longitude
            // Convert spherical coordinates to Cartesian for each corner
            float x1 = cosf(lng1) * cosf(lat1) * radius;
            float y1 = sinf(lng1) * cosf(lat1) * radius;
            float z1 = sinf(lat1) * radius;
            float x2 = cosf(lng2) * cosf(lat1) * radius;
            float y2 = sinf(lng2) * cosf(lat1) * radius;
            float z2 = sinf(lat1) * radius;
            float x3 = cosf(lng2) * cosf(lat2) * radius;
            float y3 = sinf(lng2) * cosf(lat2) * radius;
            float z3 = sinf(lat2) * radius;
            float x4 = cosf(lng1) * cosf(lat2) * radius;
            float y4 = sinf(lng1) * cosf(lat2) * radius;
            float z4 = sinf(lat2) * radius;
            // Set normals for lighting (unit vectors from center). This is crucial for correct lighting calculations:
            // The normal at each vertex tells OpenGL how light should reflect off the surface at that point.
            // For a sphere, the normal is just the normalized position vector from the center.
            glNormal3f(x1/radius, y1/radius, z1/radius); // First corner normal
            glVertex3f(x1, y1, z1); // First corner position
            glNormal3f(x2/radius, y2/radius, z2/radius); // Second corner normal
            glVertex3f(x2, y2, z2); // Second corner position
            glNormal3f(x3/radius, y3/radius, z3/radius); // Third corner normal
            glVertex3f(x3, y3, z3); // Third corner position
            glNormal3f(x4/radius, y4/radius, z4/radius); // Fourth corner normal
            glVertex3f(x4, y4, z4); // Fourth corner position
            // By specifying normals per vertex, we ensure smooth shading across the sphere's surface.
        }
    }
    glEnd(); // End drawing sphere. All vertices and normals are now sent to the GPU.
}

// =========================
// Renders a celestial body (sun or moon) as a glowing sphere.
// Handles OpenGL state for emission, lighting, and depth.
// =========================
static void renderCelestialBody(SkyObject* body) {
    if (body->brightness <= 0.0f) return; // Skip if not visible (e.g., sun below horizon)
    glPushMatrix(); // Save current transformation state. This isolates the translation for this object only.
    // Move to the celestial body's position in the sky. This positions the sun/moon at the correct place in the scene.
    glTranslatef(body->position[0], body->position[1], body->position[2]);
    // Disable lighting so the body is not affected by scene lights (it emits its own light).
    glDisable(GL_LIGHTING);
    // Disable depth test so the body is always drawn on top of the sky (prevents it from being hidden by terrain or clouds).
    glDisable(GL_DEPTH_TEST);
    // Set color and alpha based on body color and brightness. This controls the RGBA color used for the sphere.
    glColor4f(body->color[0], body->color[1], body->color[2], body->brightness);
    // Set emission so the sphere appears to glow (not affected by scene lighting).
    // Emission is used here to make the sun/moon look like a light source, not just a colored object.
    float emission[4] = { body->color[0] * body->brightness, 
                         body->color[1] * body->brightness, 
                         body->color[2] * body->brightness, 1.0f };
    glMaterialfv(GL_FRONT, GL_EMISSION, emission); // This makes the sphere appear to emit light.
    // Draw the sphere for the sun or moon. This uses the normals set in renderSimpleSphere, but lighting is off, so only emission and color matter.
    renderSimpleSphere(body->size);
    // Reset emission to zero to avoid affecting other objects. This is important because emission is a global material state.
    float zero[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_EMISSION, zero);
    // Re-enable depth test for subsequent rendering. This restores normal depth handling for the rest of the scene.
    glEnable(GL_DEPTH_TEST);
    glPopMatrix(); // Restore transformation state. This undoes the translation so other objects are not affected.
}

// =========================
// Initializes the sun and moon properties in the sky system.
// Sets their size and color based on the landscape scale.
// =========================
void skySystemInitialize(SkySystem* sky) {
    // Set sun size proportional to the landscape scale for visual consistency.
    sky->sun.size = LANDSCAPE_SCALE * 0.19f;
    // Set sun color to a warm yellowish-white (RGBA).
    sky->sun.color[0] = 1.0f; sky->sun.color[1] = 0.95f; 
    sky->sun.color[2] = 0.7f; sky->sun.color[3] = 1.0f;
    // Set moon size slightly smaller than sun.
    sky->moon.size = LANDSCAPE_SCALE * 0.13f;
    // Set moon color to a cool bluish-white (RGBA), with some transparency.
    sky->moon.color[0] = 0.95f; sky->moon.color[1] = 0.98f; 
    sky->moon.color[2] = 1.0f; sky->moon.color[3] = 0.9f;
}

// =========================
// Advances the sun and moon positions and brightness based on time of day.
// Simulates the sun/moon moving in a circular arc across the sky.
// =========================
void skySystemAdvance(SkySystem* sky, float timeOfDay) {
    // Compute the phase of the day (0 to 2pi), offset so noon is at the top.
    float phase = (timeOfDay / 24.0f - 0.22f) * 2 * M_PI;
    // Set the elevation and distance of the celestial bodies from the scene center.
    float elev = LANDSCAPE_SCALE * 1.1f; // Height above ground
    float dist = LANDSCAPE_SCALE * 1.5f; // Distance from scene center
    // Compute the sun's elevation (vertical position) as a sine wave over the day.
    float sunElev = sinf(phase); // +1 at noon, -1 at midnight
    // Sun position: moves in a circle in the sky.
    sky->sun.position[0] = dist * cosf(phase); // X position (east-west)
    sky->sun.position[1] = elev * sinf(phase); // Y position (height in sky)
    sky->sun.position[2] = 0.0f; // Z position (fixed for simplicity)
    // Sun brightness: only positive when above the horizon, scaled for intensity.
    sky->sun.brightness = fmaxf(0.0f, sunElev * 1.1f); // 0 at night, up to 1.1 at noon
    // Moon position: opposite the sun (180 degrees out of phase).
    sky->moon.position[0] = dist * cosf(phase + M_PI);
    sky->moon.position[1] = elev * sinf(phase + M_PI);
    sky->moon.position[2] = 0.0f;
    // Moon brightness: only positive when sun is below horizon, scaled for subtlety.
    sky->moon.brightness = fmaxf(0.0f, -sunElev) * 0.8f; // 0 during day, up to 0.8 at night
}

// =========================
// Applies blended lighting to the scene based on sun and moon.
// Sets OpenGL light position, ambient, and diffuse color to match sky state.
// =========================
// Claude Generated this function, because the blending was not coming out right
void skySystemApplyLighting(SkySystem* sky) {
    // Compute blend factor: how much the sun vs. moon should influence lighting.
    // This ensures a smooth transition at dawn/dusk.
    float blend = sky->sun.brightness / (sky->sun.brightness + sky->moon.brightness + 1e-3f); // Avoid divide by zero
    float invBlend = 1.0f - blend;
    // Compute blended light direction: weighted average of sun and moon positions.
    float pos[4] = { // Directional Vector (w=0 means directional light)
        blend * sky->sun.position[0] + invBlend * sky->moon.position[0],
        blend * sky->sun.position[1] + invBlend * sky->moon.position[1],
        blend * sky->sun.position[2] + invBlend * sky->moon.position[2], 0.0f
    };
    // Compute ambient light: more ambient during the day, less at night.
    float ambient[4] = { blend * 0.42f + invBlend * 0.10f, 
                        blend * 0.42f + invBlend * 0.10f,
                        blend * 0.42f + invBlend * 0.10f, 1.0f };
    // Compute diffuse light: warm and bright during the day, cooler and dimmer at night.
    float diffuse[4] = { 
        blend * 0.98f + invBlend * 0.19f,
        blend * 0.91f + invBlend * 0.17f,
        blend * 0.78f + invBlend * 0.29f, 1.0f
    };
    // Set OpenGL light 0's position (directional light from sun/moon)
    glLightfv(GL_LIGHT0, GL_POSITION, pos); // This controls the direction of shadows and highlights in the scene.
    // Set ambient light color
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient); // This controls the base level of light everywhere.
    // Set diffuse light color
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); // This controls the color and intensity of direct illumination.
}
// Claude generated code ends here

// =========================
// Renders the sky system for the current time of day.
// Advances sun/moon, applies lighting, and draws both bodies.
// =========================
void skySystemRender(SkySystem* sky, float timeOfDay) {
    skySystemAdvance(sky, timeOfDay); // Update sun/moon positions and brightness
    skySystemApplyLighting(sky);      // Set OpenGL lighting to match sky state
    renderCelestialBody(&sky->sun);   // Draw the sun (if visible)
    renderCelestialBody(&sky->moon);  // Draw the moon (if visible)
}