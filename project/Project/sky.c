<<<<<<< Updated upstream
// ---------------------------------------------
// sky.c - Sun, moon, and sky system
// ---------------------------------------------

=======
/*
 * Sphere Implementation based on:
 *   - https://www.songho.ca/opengl/gl_sphere.html
 *
 * All code is my own original work, except the section marked as AI
 */

#include "CSCIx229.h"
>>>>>>> Stashed changes
#include "sky.h"
#include "landscape.h"
#include <math.h>
#include <stdlib.h>

<<<<<<< Updated upstream
#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

static SkySystem* globalSky = NULL;
static GLUquadric* quadric = NULL;

// --- Initialize sky system ---
/* Sets up celestial objects and rendering resources */
void skySystemInit(SkySystem* sky) {
    quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);
    sky->sun.size = LANDSCAPE_SCALE * 0.18f;
    sky->sun.color[0] = 1.0f;
    sky->sun.color[1] = 0.92f;
    sky->sun.color[2] = 0.55f;
    sky->sun.color[3] = 0.95f;
    sky->moon.size = LANDSCAPE_SCALE * 0.13f;
    sky->moon.color[0] = 0.95f;
    sky->moon.color[1] = 0.95f;
    sky->moon.color[2] = 1.0f;
    sky->moon.color[3] = 0.9f;
    globalSky = sky;
}

// --- Render a sky object ---
/* Renders sun or moon as an emissive sphere */
static void renderSkyObject(SkyObject* obj) {
    if (obj->brightness <= 0.0f) return;
=======
static void renderSimpleSphere(float radius) {
    glBegin(GL_QUADS);
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            float lat1 = M_PI * (-0.5f + (float)i / 16);
            float lat2 = M_PI * (-0.5f + (float)(i + 1) / 16);
            float lng1 = 2 * M_PI * (float)j / 16;
            float lng2 = 2 * M_PI * (float)(j + 1) / 16;
            
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
            
            glNormal3f(x1/radius, y1/radius, z1/radius);
            glVertex3f(x1, y1, z1);
            glNormal3f(x2/radius, y2/radius, z2/radius);
            glVertex3f(x2, y2, z2);
            glNormal3f(x3/radius, y3/radius, z3/radius);
            glVertex3f(x3, y3, z3);
            glNormal3f(x4/radius, y4/radius, z4/radius);
            glVertex3f(x4, y4, z4);
        }
    }
    glEnd();
}

static void renderCelestialBody(SkyObject* body) {
    if (body->brightness <= 0.0f) return;
    
>>>>>>> Stashed changes
    glPushMatrix();
    glTranslatef(body->position[0], body->position[1], body->position[2]);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glColor4f(body->color[0], body->color[1], body->color[2], body->brightness);
    
    float emission[4] = { body->color[0] * body->brightness, 
                         body->color[1] * body->brightness, 
                         body->color[2] * body->brightness, 1.0f };
    glMaterialfv(GL_FRONT, GL_EMISSION, emission);
    renderSimpleSphere(body->size);
    
    float zero[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_EMISSION, zero);
    
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}

<<<<<<< Updated upstream
// --- Update sky positions ---
/* Updates sun/moon positions based on time of day */
void skySystemUpdate(SkySystem* sky, float dayTime) {
    float timeNormalized = dayTime / 24.0f;
    float angle = (timeNormalized - 0.25f) * 2 * M_PI;
    float height = LANDSCAPE_SCALE * 1.2f;
    float radius = LANDSCAPE_SCALE * 1.4f;
    sky->sun.position[0] = radius * cos(angle);
    sky->sun.position[1] = height * sin(angle);
    sky->sun.position[2] = 0;
    float sunHeight = sin(angle);
    sky->sun.brightness = fmax(0.0f, sunHeight * 1.2f);
    sky->moon.position[0] = radius * cos(angle + M_PI);
    sky->moon.position[1] = height * sin(angle + M_PI);
    sky->moon.position[2] = 0;
    sky->moon.brightness = fmax(0.0f, -sunHeight) * 0.9f;
}

// --- Update lighting ---
/* Sets scene lighting based on sun/moon position */
void skySystemUpdateLighting(SkySystem* sky) {
    float lightPos[4];
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float sunHeight = sky->sun.position[1] / (LANDSCAPE_SCALE * 2.0);
    if (sky->sun.brightness > sky->moon.brightness) {
        lightPos[0] = sky->sun.position[0];
        lightPos[1] = sky->sun.position[1];
        lightPos[2] = sky->sun.position[2];
        lightPos[3] = 0.0f;
        float ambientStrength = 0.2f + 0.3f * sunHeight;
        ambient[0] = ambientStrength;
        ambient[1] = ambientStrength;
        ambient[2] = ambientStrength * 0.9f;
        ambient[3] = 1.0f;
        float intensity = 0.6f + 0.4f * sunHeight;
        diffuse[0] = intensity * 1.0f;
        diffuse[1] = intensity * 0.95f;
        diffuse[2] = intensity * 0.85f;
        diffuse[3] = 1.0f;
        specular[0] = intensity * 0.7f;
        specular[1] = intensity * 0.7f;
        specular[2] = intensity * 0.7f;
        specular[3] = 1.0f;
    } else {
        lightPos[0] = sky->moon.position[0];
        lightPos[1] = sky->moon.position[1];
        lightPos[2] = sky->moon.position[2];
        lightPos[3] = 0.0f;
        float ambientStrength = 0.1f + 0.1f * sky->moon.brightness;
        ambient[0] = ambientStrength * 0.7f;
        ambient[1] = ambientStrength * 0.7f;
        ambient[2] = ambientStrength * 0.9f;
        ambient[3] = 1.0f;
        float intensity = 0.3f * sky->moon.brightness;
        diffuse[0] = intensity * 0.8f;
        diffuse[1] = intensity * 0.8f;
        diffuse[2] = intensity * 1.0f;
        diffuse[3] = 1.0f;
        specular[0] = intensity * 0.4f;
        specular[1] = intensity * 0.4f;
        specular[2] = intensity * 0.5f;
        specular[3] = 1.0f;
    }
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
=======
void skySystemInitialize(SkySystem* sky) {
    sky->sun.size = LANDSCAPE_SCALE * 0.19f;
    sky->sun.color[0] = 1.0f; sky->sun.color[1] = 0.95f; 
    sky->sun.color[2] = 0.7f; sky->sun.color[3] = 1.0f;
    
    sky->moon.size = LANDSCAPE_SCALE * 0.13f;
    sky->moon.color[0] = 0.95f; sky->moon.color[1] = 0.98f; 
    sky->moon.color[2] = 1.0f; sky->moon.color[3] = 0.9f;
}

void skySystemAdvance(SkySystem* sky, float timeOfDay) {
    float phase = (timeOfDay / 24.0f - 0.22f) * 2 * M_PI;
    float elev = LANDSCAPE_SCALE * 1.1f;
    float dist = LANDSCAPE_SCALE * 1.5f;
    float sunElev = sinf(phase);
    
    sky->sun.position[0] = dist * cosf(phase);
    sky->sun.position[1] = elev * sinf(phase);
    sky->sun.position[2] = 0.0f;
    sky->sun.brightness = fmaxf(0.0f, sunElev * 1.1f);
    
    sky->moon.position[0] = dist * cosf(phase + M_PI);
    sky->moon.position[1] = elev * sinf(phase + M_PI);
    sky->moon.position[2] = 0.0f;
    sky->moon.brightness = fmaxf(0.0f, -sunElev) * 0.8f;
}

// Claude Generated this function, because the blending was not coming out right
void skySystemApplyLighting(SkySystem* sky) {
    float blend = sky->sun.brightness / (sky->sun.brightness + sky->moon.brightness + 1e-3f);
    float invBlend = 1.0f - blend;

    float pos[4] = { 
        blend * sky->sun.position[0] + invBlend * sky->moon.position[0],
        blend * sky->sun.position[1] + invBlend * sky->moon.position[1],
        blend * sky->sun.position[2] + invBlend * sky->moon.position[2], 0.0f
    };
    float ambient[4] = { blend * 0.42f + invBlend * 0.10f, 
                        blend * 0.42f + invBlend * 0.10f,
                        blend * 0.42f + invBlend * 0.10f, 1.0f };
    float diffuse[4] = { 
        blend * 0.98f + invBlend * 0.19f,
        blend * 0.91f + invBlend * 0.17f,
        blend * 0.78f + invBlend * 0.29f, 1.0f
    };
    
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
>>>>>>> Stashed changes
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
}
// Claude generated code ends here

<<<<<<< Updated upstream
// --- Render celestial objects ---
/* Renders sun and moon with current lighting */
void skySystemRenderSunAndMoon(SkySystem* sky, float dayTime) {
    skySystemUpdate(sky, dayTime);
    skySystemUpdateLighting(sky);
    renderSkyObject(&sky->sun);
    renderSkyObject(&sky->moon);
}

// --- Cleanup ---
/* Frees sky system resources */
void skySystemDestroy(SkySystem* sky) {
    if (quadric) {
        gluDeleteQuadric(quadric);
        quadric = NULL;
    }
    globalSky = NULL;
=======
void skySystemRender(SkySystem* sky, float timeOfDay) {
    skySystemAdvance(sky, timeOfDay);
    skySystemApplyLighting(sky);
    renderCelestialBody(&sky->sun);
    renderCelestialBody(&sky->moon);
>>>>>>> Stashed changes
}