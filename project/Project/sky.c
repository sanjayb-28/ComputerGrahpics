/*
 * Sphere Implementation based on:
 *   - https://www.songho.ca/opengl/gl_sphere.html
 *
 * All code is my own original work, except the section marked as AI
 */

#include "CSCIx229.h"
#include "sky.h"
#include "landscape.h"

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
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
}
// Claude generated code ends here

void skySystemRender(SkySystem* sky, float timeOfDay) {
    skySystemAdvance(sky, timeOfDay);
    skySystemApplyLighting(sky);
    renderCelestialBody(&sky->sun);
    renderCelestialBody(&sky->moon);
}