// ---------------------------------------------
// main.c - Main entry and core logic for terrain demo
// ---------------------------------------------

#define GL_SILENCE_DEPRECATION
#include "CSCIx229.h"
#include "landscape.h"
#include "objects.h"
#include "weather_particles.h"
#include "shaders.h"
#include "sky.h"
#include "sky_clouds.h"
#include "camera.h"
#include <stdbool.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

<<<<<<< Updated upstream
// --- Global state ---
/* Environmental systems and rendering resources */
static SkyCloudSystem* cloudSystem = NULL;
=======
static AtmosphericCloudSystem* cloudSystem = NULL;
>>>>>>> Stashed changes
static SkySystem skySystemInstance;

GLuint grassTexture = 0;
GLuint rockTexture = 0;
GLuint sandTexture = 0;
<<<<<<< Updated upstream
int terrainShader = 0;
int skyShader = 0;
=======
GLuint boulderTexture = 0;
GLuint barkTexture = 0;
GLuint leafTexture = 0;
>>>>>>> Stashed changes

static int th = 45;
static int ph = 10;
static float dim = 100;
static int fov = 1;
      
static float lightHeight = 250.0f;
static float dayTime = 0.0f;
static float windStrength = 0.0f;

<<<<<<< Updated upstream
float waterLevel = -4.0f;
static float waterSpeed = 1.0f;
=======
#define WATER_LEVEL -4.0f
>>>>>>> Stashed changes
static float waterTime = 0.0f;
static int animateWater = 1;

static int wireframe = 0;
static int showAxes = 0;
static int animateLight = 1;
static float lightSpeed = 1.0f; 

/* Weather and particle system configuration */
#define SNOW_PARTICLES 20000
#define RAIN_PARTICLES 12000
static WeatherParticleSystem* snowSystem = NULL;
static WeatherParticleSystem* rainSystem = NULL;
static int snowEnabled = 0;
static int rainEnabled = 0;
static int weatherType = 0;
static float weatherIntensity = 1.0f;

/* World and scene objects */
Landscape* landscape = NULL;
static ForestSystem* forest = NULL;

/* Timing utilities */
static float lastTime = 0;
static float deltaTime = 0;

/* Atmospheric effects */
float fogDensity = 0.005f;
int fogEnabled = 0;

static int animateTime = 1;
static float timeSpeed = 1.0f;

/* Camera system and mouse interaction */
static ViewCamera* camera = NULL;
float asp;
static int lastX = 0, lastY = 0;
static int mouseButtons = 0;

<<<<<<< Updated upstream
/* Environment object collections */
static RockField* rocks = NULL;
static ShrubField* shrubs = NULL;
static LogField* logs = NULL;

// --- Persistent camera state for view switching ---
/* Stores view settings to allow seamless switching between camera modes */
static float lastOrbitYaw = 45.0f, lastOrbitPitch = 10.0f, lastOrbitDistance = 70.0f;
static int lastOrbitTh = 45, lastOrbitPh = 10;
static float lastOrbitDim = 70.0f;
static float lastFPPos[3] = {0.0f, 2.0f, 0.0f};
static float lastFPYaw = 45.0f, lastFPPitch = 10.0f;
static const float INIT_ORBIT_YAW = 45.0f, INIT_ORBIT_PITCH = 10.0f, INIT_ORBIT_DISTANCE = 70.0f;
static const int INIT_ORBIT_TH = 45, INIT_ORBIT_PH = 10;
static const float INIT_ORBIT_DIM = 70.0f;
static const float INIT_FP_POS[3] = {0.0f, 2.0f, 0.0f};
static const float INIT_FP_YAW = 45.0f, INIT_FP_PITCH = 10.0f;

// --- Gull model ---
static GullFlock* gullFlock = NULL;

static int showBirds = 1;
=======
float treeSwayAngle = 0.0f;

static int snowOn = 0;
static int weatherType = 0;

static int ambientSoundOn = 1;

#define DIM_MIN 30.0f
#define DIM_MAX 200.0f

void clampAndSyncDim() {
    if (dim < DIM_MIN) dim = DIM_MIN;
    if (dim > DIM_MAX) dim = DIM_MAX;
    if (camera) {
        if (camera->orbitDistance < DIM_MIN) camera->orbitDistance = DIM_MIN;
        if (camera->orbitDistance > DIM_MAX) camera->orbitDistance = DIM_MAX;
        dim = camera->orbitDistance = (dim + camera->orbitDistance) * 0.5f;
    }
}
>>>>>>> Stashed changes

// --- Function declarations ---
void reshape(int width, int height);
void display();
void special(int key, int x, int y);
void keyboard(unsigned char key, int x, int y);
void idle();

<<<<<<< Updated upstream
#define MAX_STARS 1000
// Simple star struct for night sky
typedef struct {
    float x, y, z;
    float brightness;
} Star;
Star stars[MAX_STARS];

// --- Window reshape handler ---
/* Updates projection matrix when window size changes */
=======
>>>>>>> Stashed changes
void reshape(int width, int height) {
    asp = (height>0) ? (double)width/height : 1;
    glViewport(0,0, RES*width,RES*height);
    if (camera) {
        viewCameraSetProjection(camera, 55.0f, asp, dim/4, dim*4);
    } else {
        Project(fov?55:0, asp, dim);
    }
}

// --- Timing utilities ---
/* Calculates time between frames for smooth animation */
void updateDeltaTime() {
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
}

/* Maps 24-hour time to 0.0-1.0 range for shader inputs */
float getNormalizedDayTime(float time) {
    return time / 24.0f;
}

/* Smooth transition function for natural blending */
float smoothstep(float edge0, float edge1, float x) {
    float t = (x - edge0) / (edge1 - edge0);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

<<<<<<< Updated upstream
// --- Sky color interpolation for day/night cycle ---
/* Computes sky color based on time of day with smooth transitions */
=======
>>>>>>> Stashed changes
void getSkyColor(float time, float* color) {
    float t = time / 24.0f;  
    const int NUM_COLORS = 6;
    float timePoints[6] = {0.0f, 0.25f, 0.4f, 0.6f, 0.75f, 1.0f};  
    float colors[6][3] = {
        {0.02f, 0.02f, 0.1f},  // Night
        {0.7f, 0.4f, 0.4f},    // Dawn
        {0.4f, 0.7f, 1.0f},    // Day
        {0.4f, 0.7f, 1.0f},    // Day
        {0.7f, 0.4f, 0.4f},    // Dusk
        {0.02f, 0.02f, 0.1f}   // Night
    };
    int i;
    for(i = 0; i < NUM_COLORS-1; i++) {
        if(t >= timePoints[i] && t <= timePoints[i+1]) break;
    }
    float segmentPos = (t - timePoints[i]) / (timePoints[i+1] - timePoints[i]);
    float blend = smoothstep(0.0f, 1.0f, segmentPos);
    float sunHeight = sin(t * 2.0f * M_PI);
    for(int j = 0; j < 3; j++) {
        color[j] = colors[i][j] * (1.0f - blend) + colors[i+1][j] * blend;
    }
    if(sunHeight > 0) {
        color[2] = fmin(1.0f, color[2] + sunHeight * 0.04f);
    }
    if(t < 0.1f || t > 0.9f) {
        float nightBlend = (t < 0.1f) ? (t / 0.1f) : ((1.0f - t) / 0.1f);
        nightBlend = smoothstep(0.0f, 1.0f, nightBlend);
        float nightColor[3] = {0.02f, 0.02f, 0.1f};
        for(int j = 0; j < 3; j++) {
            color[j] = color[j] * nightBlend + nightColor[j] * (1.0f - nightBlend);
        }
    }
}

<<<<<<< Updated upstream
// --- Update ambient and diffuse lighting for day/night ---
/* Adjusts scene lighting based on time of day */
void updateDayCycle() {
    dayTime += deltaTime * 0.1f;  
    if (dayTime >= 24.0f) dayTime = 0.0f;
    float timeNormalized = dayTime / 24.0f;
    float sunHeight = sin(timeNormalized * 2 * M_PI);
    float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    float diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    if (sunHeight > 0) {
        float intensity = 0.6f + sunHeight * 0.4f; 
        ambient[0] = 0.3f + sunHeight * 0.1f;
        ambient[1] = 0.3f + sunHeight * 0.1f;
        ambient[2] = 0.3f + sunHeight * 0.1f;
        diffuse[0] = intensity * 1.0f;
        diffuse[1] = intensity * 0.95f;
        diffuse[2] = intensity * 0.8f;
    } else {
        float intensity = 0.3f; 
        ambient[0] = intensity * 0.2f;
        ambient[1] = intensity * 0.2f;
        ambient[2] = intensity * 0.3f;
        diffuse[0] = intensity * 0.6f;
        diffuse[1] = intensity * 0.6f;
        diffuse[2] = intensity * 0.8f;
    }
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
}

// --- Dynamic sun/moon lighting ---
/* Updates light source position and parameters based on time of day */
void updateDynamicLighting(float dayTime) {
    float timeNormalized = dayTime / 24.0f;
    float sunAngle = (timeNormalized - 0.25f) * 2 * M_PI;
    float sunHeight = sin(sunAngle);
    float sunX = 500 * cos(sunAngle);
    float sunY = lightHeight * sunHeight;
    float sunZ = 0;
    float moonX = 500 * cos(sunAngle + M_PI);
    float moonY = lightHeight * (-sunHeight);
    float moonZ = 0;
    float lightPos[4];
    float ambient[4];
    float diffuse[4];
    float specular[4];
    if (sunHeight > 0) {
        // Sun is the primary light source during day
        lightPos[0] = sunX;
        lightPos[1] = sunY;
        lightPos[2] = sunZ;
        lightPos[3] = 0.0f;  // Directional light
        float intensity = 0.5f + sunHeight * 0.5f;
        ambient[0] = 0.15f + sunHeight * 0.15f;
        ambient[1] = 0.15f + sunHeight * 0.15f;
        ambient[2] = 0.15f + sunHeight * 0.15f;
        ambient[3] = 1.0f;
        diffuse[0] = intensity * 1.0f;
        diffuse[1] = intensity * 0.95f;
        diffuse[2] = intensity * 0.85f;
        diffuse[3] = 1.0f;
        specular[0] = intensity * 0.7f;
        specular[1] = intensity * 0.7f;
        specular[2] = intensity * 0.6f;
        specular[3] = 1.0f;
    }
    else {
        // Moon is the primary light source at night
        lightPos[0] = moonX;
        lightPos[1] = moonY;
        lightPos[2] = moonZ;
        lightPos[3] = 0.0f; 
        float moonIntensity = 0.15f + (-sunHeight) * 0.1f;
        ambient[0] = 0.02f;
        ambient[1] = 0.02f;
        ambient[2] = 0.04f; // Slightly blue for night
        ambient[3] = 1.0f;
        diffuse[0] = moonIntensity * 0.7f;
        diffuse[1] = moonIntensity * 0.7f;
        diffuse[2] = moonIntensity * 0.9f;
        diffuse[3] = 1.0f;
        specular[0] = moonIntensity * 0.3f;
        specular[1] = moonIntensity * 0.3f;
        specular[2] = moonIntensity * 0.4f;
        specular[3] = 1.0f;
    }
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    float matSpecular[] = {0.3f, 0.3f, 0.3f, 1.0f};
    float matShininess = sunHeight > 0 ? 30.0f : 15.0f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);
}

// --- Animate tree swaying for wind effect ---
/* Updates tree tilt angles based on wind strength and time */
=======
>>>>>>> Stashed changes
void updateTreeAnimation() {
    static float windTime = 0.0f;
    windTime += deltaTime;
    windStrength = sin(windTime * 0.5f) * 0.5f + 0.5f;
    for (int i = 0; i < forest->instanceCount; i++) {
        float windEffect = windStrength * (1.0f + sin(forest->instances[i].x * 0.1f + windTime));
        forest->instances[i].tiltX = sin(windTime + forest->instances[i].x) * windEffect * 5.0f;
        forest->instances[i].tiltZ = cos(windTime * 0.7f + forest->instances[i].z) * windEffect * 5.0f;
    }
}

<<<<<<< Updated upstream
// --- Lighting/material setup ---
/* Configures OpenGL lighting state for the scene */
=======
>>>>>>> Stashed changes
void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    float position[] = {1.0f, 2.0f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    float mSpecular[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 30.0f);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

<<<<<<< Updated upstream
// --- Fog effect for atmosphere ---
/* Configures fog parameters based on time of day */
=======
>>>>>>> Stashed changes
void updateFog(float dayTime) {
    float timeNormalized = dayTime / 24.0f;
    float sunAngle = (timeNormalized - 0.25f) * 2 * M_PI;
    float sunHeight = sin(sunAngle);
<<<<<<< Updated upstream
    float baseDensity;
    float fogColor[4];
    if (sunHeight > 0) {
        baseDensity = 0.003f;
        fogColor[0] = 0.95f;
        fogColor[1] = 0.95f;
        fogColor[2] = 0.95f;
    } else {
        baseDensity = 0.008f;
        fogColor[0] = 0.7f;
        fogColor[1] = 0.7f;
        fogColor[2] = 0.7f;
=======
    float baseDensity = 0.008f;
    float fogColor[4] = {0.95f, 0.95f, 0.95f, 1.0f};
    
    if (sunHeight <= 0) {
        fogColor[0] = fogColor[1] = fogColor[2] = 0.7f;
>>>>>>> Stashed changes
    }
    
    if (fogEnabled) {
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_EXP2);
        glFogf(GL_FOG_DENSITY, baseDensity);
        glFogfv(GL_FOG_COLOR, fogColor);
        float fogStart = sunHeight > 0 ? dim * 0.1f : dim * 0.05f;
        float fogEnd = sunHeight > 0 ? dim * 0.8f : dim * 0.4f;
        glFogf(GL_FOG_START, fogStart);
        glFogf(GL_FOG_END, fogEnd);
        glHint(GL_FOG_HINT, GL_NICEST);
    }
}

<<<<<<< Updated upstream
// --- OpenGL state initialization ---
/* Sets up global rendering state for the scene */
=======
>>>>>>> Stashed changes
void initGL() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonOffset(1.0f, 1.0f);
}

<<<<<<< Updated upstream
// --- Main display/render function ---
/* Renders the complete scene with all objects and effects */
=======
>>>>>>> Stashed changes
void display() {
    float skyColor[3];
    getSkyColor(dayTime, skyColor);
    glClearColor(skyColor[0], skyColor[1], skyColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(camera->position[0], camera->position[1], camera->position[2],
              camera->lookAt[0], camera->lookAt[1], camera->lookAt[2],
              camera->upVec[0], camera->upVec[1], camera->upVec[2]);
    skySystemRender(&skySystemInstance, dayTime);
    updateFog(dayTime);
<<<<<<< Updated upstream
    updateDynamicLighting(dayTime);
    landscapeRender(landscape, weatherType);
    useShader(0);
    if (cloudSystem) {
        skyCloudSystemUpdate(cloudSystem, deltaTime, dayTime);
        skyCloudSystemRender(cloudSystem, dayTime);
    }
    if (forest && forest->instanceCount > 0) {
        glPushMatrix();
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);
        glShadeModel(GL_SMOOTH);
        forestSystemRender(forest, dayTime, weatherType == 1);
        glPopMatrix();
    }
    if (rocks && rocks->instanceCount > 0) {
        glPushMatrix();
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);
        glShadeModel(GL_SMOOTH);
        rockFieldRender(rocks, weatherType == 1);
        glPopMatrix();
    }
    if (shrubs && shrubs->instanceCount > 0) {
        glPushMatrix();
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);
        glShadeModel(GL_SMOOTH);
        shrubFieldRender(shrubs, weatherType == 1);
        glPopMatrix();
    }
    if (logs && logs->instanceCount > 0) {
        glPushMatrix();
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);
        glShadeModel(GL_SMOOTH);
        logFieldRender(logs, weatherType == 1);
        glPopMatrix();
    }
    // --- Render animated gull flock only ---
    if (showBirds && gullFlock) {
        gullFlockRender(gullFlock, 0);
=======
    if (cloudSystem) {
        glDepthMask(GL_FALSE);
        atmosphericCloudSystemRender(cloudSystem);
        glDepthMask(GL_TRUE);
    }
    landscapeRender(landscape, weatherType);
    float timeNormalized = dayTime / 24.0f;
    float sunAngle = (timeNormalized - 0.25f) * 2 * M_PI;
    float sunHeight = sin(sunAngle);
    float sunX = 500 * cos(sunAngle);
    float sunY = lightHeight * sunHeight;
    float sunZ = 0;
    float sunDir[3];
    float len = sqrtf(sunX*sunX + sunY*sunY + sunZ*sunZ);
    sunDir[0] = sunX / len;
    sunDir[1] = sunY / len;
    sunDir[2] = sunZ / len;
    float ambient[3];
    if (sunHeight > 0) {
        ambient[0] = ambient[1] = ambient[2] = 0.15f + sunHeight * 0.15f;
    } else {
        ambient[0] = 0.02f;
        ambient[1] = 0.02f;
        ambient[2] = 0.04f;
>>>>>>> Stashed changes
    }
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    landscapeRenderWater(WATER_LEVEL, landscape, dayTime);
    glDepthMask(GL_TRUE);
    if (snowEnabled && snowSystem) {
        glDisable(GL_FOG);
        weatherParticleSystemRender(snowSystem);
        glEnable(GL_FOG);
    }
    if (rainEnabled && rainSystem) {
        glDisable(GL_FOG);
        weatherParticleSystemRender(rainSystem);
        glEnable(GL_FOG);
    }
    glDisable(GL_FOG);
    if (showAxes) {
        glDisable(GL_DEPTH_TEST);
        glColor3f(1,1,1);
        glBegin(GL_LINES);
        glVertex3f(0.0,0.0,0.0);
        glVertex3f(dim/2,0.0,0.0);
        glVertex3f(0.0,0.0,0.0);
        glVertex3f(0.0,dim/2,0.0);
        glVertex3f(0.0,0.0,0.0);
        glVertex3f(0.0,0.0,dim/2);
        glEnd();
        glEnable(GL_DEPTH_TEST);
    }
    // UI overlays
    glDisable(GL_DEPTH_TEST);
    glColor3f(1,1,1);
    glWindowPos2i(5, glutGet(GLUT_WINDOW_HEIGHT) - 20);
    Print("Time: %02d:%02d  Weather: %s", 
          (int)dayTime, (int)((dayTime-(int)dayTime)*60), 
          weatherType ? "Winter" : "Fall");
    int y = 5;
    glWindowPos2i(5, y);
    Print("Angle=%d,%d  Dim=%.1f  View=%s   |   Water=%.1f   |   Wireframe=%d   |   Axes=%d   |   TimeAnim: %s  Speed: %.1fx   |   Fog: %s   |   Birds: %s   |   Snow: %s   |   Rain: %s",
        th, ph, dim, camera->mode == CAMERA_MODE_FREE_ORBIT ? "Free Orbit" : "First Person",
        WATER_LEVEL,
        wireframe,
        showAxes,
        animateTime ? "On" : "Off", timeSpeed,
        fogEnabled ? "On" : "Off",
        showBirds ? "On" : "Off",
        snowEnabled ? "On" : "Off",
        rainEnabled ? "On" : "Off");
    glEnable(GL_DEPTH_TEST);
    glutSwapBuffers();
}

<<<<<<< Updated upstream
// --- Sky shader loader ---
/* Initializes the sky background shader program */
void initSkyBackground() {
    skyShader = loadShader("shaders/sky_shader.vert", "shaders/sky_shader.frag");
}

// --- Mouse button handler ---
/* Processes mouse button events for camera control */
=======
>>>>>>> Stashed changes
void mouse(int button, int state, int x, int y) {
    lastX = x;
    lastY = y;
    if (state == GLUT_DOWN) {
        mouseButtons |= 1<<button;
    } else {
        mouseButtons &= ~(1<<button);
    }
    if (button == 3) {
        if (camera->mode == CAMERA_MODE_FREE_ORBIT) {
            dim = dim * 0.9;
            camera->orbitDistance *= 0.9;
        }
    } else if (button == 4) {
        if (camera->mode == CAMERA_MODE_FREE_ORBIT) {
            dim = dim * 1.1;
            camera->orbitDistance *= 1.1;
        }
    }
    glutPostRedisplay();
}

<<<<<<< Updated upstream
// --- Mouse drag handler ---
/* Processes mouse movement for camera rotation and zoom */
void mouseMotion(int x, int y) {
    int dx = x - lastX;
    int dy = y - lastY;
    if (camera->mode == CAMERA_MODE_FREE_ORBIT) {
        if (mouseButtons & 1) {
            th += dx;
            ph += dy;
            camera->horizontalAngle += dx;
            camera->verticalAngle += dy;
            if (ph > 89) ph = 89;
            if (ph < -89) ph = -89;
            if (camera->verticalAngle > 89) camera->verticalAngle = 89;
            if (camera->verticalAngle < -89) camera->verticalAngle = -89;
        }
        else if (mouseButtons & 4) {
            dim *= (1 + dy/100.0);
            camera->orbitDistance *= (1 + dy/100.0);
            if (dim < 1) dim = 1;
            if (camera->orbitDistance < 1) camera->orbitDistance = 1;
        }
    } else {
        if (mouseButtons & 1) {
            viewCameraRotate(camera, dx * 0.5f, -dy * 0.5f);
        }
=======
void mouseMotion(int x, int y) {
    int dx = x - lastX;
    int dy = y - lastY;
    if (camera->mode != CAMERA_MODE_FREE_ORBIT && (mouseButtons & 1)) {
        viewCameraRotate(camera, dx * 0.5f, -dy * 0.5f);
>>>>>>> Stashed changes
    }
    lastX = x;
    lastY = y;
    glutPostRedisplay();
}

<<<<<<< Updated upstream
// --- Keyboard special keys (arrows, page up/down) ---
/* Processes special key presses for camera movement */
=======
>>>>>>> Stashed changes
void special(int key, int x, int y) {
    float deltaTime = 0.016f;
    if (camera && camera->mode == CAMERA_MODE_FREE_ORBIT) {
        switch(key) {
            case GLUT_KEY_RIGHT:
                th += 5;
                camera->horizontalAngle = th;  
                break;
            case GLUT_KEY_LEFT:
                th -= 5;
                camera->horizontalAngle = th;
                break;
            case GLUT_KEY_UP:
                if (ph < 89) {
                    ph += 5;
                    if (ph < 10) ph = 10;
                    camera->verticalAngle = ph; 
                }
                break;
            case GLUT_KEY_DOWN:
                if (ph > -89) {
                    ph -= 5;
                    if (ph < 10) ph = 10;
                    camera->verticalAngle = ph;
                }
                break;
            case GLUT_KEY_PAGE_DOWN:
                dim += 0.1;
                camera->orbitDistance = dim;
                break;
            case GLUT_KEY_PAGE_UP:
                if (dim > 1) {
                    dim -= 0.1;
                    camera->orbitDistance = dim;
                }
                break;
        }
        th %= 360;
        ph %= 360;
        viewCameraUpdateVectors(camera);  
        Project(fov?55:0, asp, dim);
    }
    else {
        switch(key) {
            case GLUT_KEY_RIGHT:
                viewCameraMove(camera, CAMERA_MOVE_RIGHT, deltaTime);
                break;
            case GLUT_KEY_LEFT:
                viewCameraMove(camera, CAMERA_MOVE_LEFT, deltaTime);
                break;
            case GLUT_KEY_UP:
                viewCameraMove(camera, CAMERA_MOVE_FORWARD, deltaTime);
                break;
            case GLUT_KEY_DOWN:
                viewCameraMove(camera, CAMERA_MOVE_BACKWARD, deltaTime);
                break;
        }
    }
    glutPostRedisplay();
}

<<<<<<< Updated upstream
// --- Keyboard handler for all main controls ---
/* Processes standard key presses for scene control and effects */
=======
>>>>>>> Stashed changes
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 27:  // ESC key
            exit(0);
            break;
        case 'q':
        case 'Q':
            wireframe = !wireframe;
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
            break;
        case 'e':
        case 'E':
            weatherType = !weatherType;
            break;
        case 'w':
        case 'W':
            if (camera->mode == CAMERA_MODE_FIRST_PERSON) {
                viewCameraMove(camera, CAMERA_MOVE_FORWARD, 0.016f);
            }
            break;
        case 's':
        case 'S':
            if (camera->mode == CAMERA_MODE_FIRST_PERSON) {
                viewCameraMove(camera, CAMERA_MOVE_BACKWARD, 0.016f);
            }
            break;
        case 'a':
        case 'A':
            if (camera->mode == CAMERA_MODE_FIRST_PERSON) {
                viewCameraMove(camera, CAMERA_MOVE_LEFT, 0.016f);
            } else {
                showAxes = !showAxes;
            }
            break;
        case 'd':
        case 'D':
            if (camera->mode == CAMERA_MODE_FIRST_PERSON) {
                viewCameraMove(camera, CAMERA_MOVE_RIGHT, 0.016f);
            }
            break;
        case 'r':
<<<<<<< Updated upstream
            // Reset all persistent state
            lastOrbitYaw = INIT_ORBIT_YAW;
            lastOrbitPitch = INIT_ORBIT_PITCH;
            lastOrbitDistance = INIT_ORBIT_DISTANCE;
            lastOrbitTh = INIT_ORBIT_TH;
            lastOrbitPh = INIT_ORBIT_PH < 10 ? 10 : INIT_ORBIT_PH;
            lastOrbitDim = INIT_ORBIT_DIM;
            lastFPPos[0] = INIT_FP_POS[0];
            lastFPPos[1] = INIT_FP_POS[1];
            lastFPPos[2] = INIT_FP_POS[2];
            lastFPYaw = INIT_FP_YAW;
            lastFPPitch = INIT_FP_PITCH;
            th = INIT_ORBIT_TH;
            ph = INIT_ORBIT_PH < 10 ? 10 : INIT_ORBIT_PH;
            dim = INIT_ORBIT_DIM;
            waterLevel = -4.0f;
=======
            th = 45; ph = 10; dim = 70.0f;
            camera->orbitYaw = th;
            camera->orbitPitch = ph;
            camera->orbitDistance = dim;
            clampAndSyncDim();
            waterTime = 0.0f;
>>>>>>> Stashed changes
            lightHeight = 250.0f;
            waterSpeed = 1.0f;
            lightSpeed = 1.0f;
            fov = 1;
            camera->horizontalAngle = lastOrbitYaw;
            camera->verticalAngle = lastOrbitPitch;
            camera->orbitDistance = lastOrbitDistance;
            viewCameraSetMode(camera, CAMERA_MODE_FREE_ORBIT);
            viewCameraUpdateVectors(camera);
            break;
        case '1': {
<<<<<<< Updated upstream
            // Switch to first person, restore last FP state
            lastOrbitYaw = camera->horizontalAngle;
            lastOrbitPitch = camera->verticalAngle;
            lastOrbitDistance = camera->orbitDistance;
            lastOrbitTh = th;
            lastOrbitPh = ph < 10 ? 10 : ph;
            lastOrbitDim = dim;
            camera->horizontalAngle = lastFPYaw;
            camera->verticalAngle = lastFPPitch < 5.0f ? 10.0f : lastFPPitch;
            camera->position[0] = lastFPPos[0];
            camera->position[2] = lastFPPos[2];
            float groundHeight = landscapeGetHeight(landscape, camera->position[0], camera->position[2]);
            camera->position[1] = groundHeight + 2.0f;
=======
            camera->fpYaw = 45.0f;
            camera->fpPitch = 10.0f;
            camera->fpPosition[0] = 0.0f;
            camera->fpPosition[2] = 0.0f;
            float groundHeight = landscapeGetHeight(landscape, camera->fpPosition[0], camera->fpPosition[2]);
            camera->fpPosition[1] = groundHeight + 2.0f;
>>>>>>> Stashed changes
            viewCameraSetMode(camera, CAMERA_MODE_FIRST_PERSON);
            viewCameraUpdateVectors(camera);
            viewCameraSetProjection(camera, 55.0f, asp, dim/4, dim*4);
            break;
        }
        case '2': {
<<<<<<< Updated upstream
            // Switch to orbit, restore last orbit state
            lastFPPos[0] = camera->position[0];
            lastFPPos[1] = camera->position[1];
            lastFPPos[2] = camera->position[2];
            lastFPYaw = camera->horizontalAngle;
            lastFPPitch = camera->verticalAngle;
            camera->horizontalAngle = lastOrbitYaw;
            camera->verticalAngle = lastOrbitPitch;
            camera->orbitDistance = lastOrbitDistance;
            th = lastOrbitTh;
            ph = lastOrbitPh < 10 ? 10 : lastOrbitPh;
            dim = lastOrbitDim;
=======
            camera->orbitYaw = 45.0f;
            camera->orbitPitch = 10.0f;
            camera->orbitDistance = 70.0f;
            th = 45; ph = 10; dim = 70.0f;
            clampAndSyncDim();
>>>>>>> Stashed changes
            viewCameraSetMode(camera, CAMERA_MODE_FREE_ORBIT);
            viewCameraUpdateVectors(camera);
            break;
        }
        case 't':
            animateTime = !animateTime;
            glutIdleFunc(animateTime ? idle : NULL);
            break;
        case 'k':
            timeSpeed = fmax(0.1f, timeSpeed - 0.1f);
            break;
        case 'l':
            timeSpeed = fmin(5.0f, timeSpeed + 0.1f);
            break;
        case 'q':
            weatherType = (weatherType + 1) % 2;
            break;
        case 'b':
            fogEnabled = !fogEnabled;
            break;
        case 'n':
            snowEnabled = !snowEnabled;
            break;
        case 'm':
            rainEnabled = !rainEnabled;
            break;
        case 'z':
            if (camera->mode == CAMERA_MODE_FREE_ORBIT) {
                dim -= 5.0f;
                if (dim < 1.0f) dim = 1.0f;
                camera->orbitDistance -= 5.0f;
                if (camera->orbitDistance < 1.0f) camera->orbitDistance = 1.0f;
                viewCameraUpdateVectors(camera);
                Project(fov?55:0, asp, dim);
            }
            break;
        case 'Z':
            if (camera->mode == CAMERA_MODE_FREE_ORBIT) {
                dim += 5.0f;
                camera->orbitDistance += 5.0f;
                viewCameraUpdateVectors(camera);
                Project(fov?55:0, asp, dim);
            }
            break;
<<<<<<< Updated upstream
        case 'v':
            showBirds = !showBirds;
=======
        case 'n':
            snowOn = !snowOn;
            particleSystemSetEnabled(snowOn);
            break;
        case 'm':
            ambientSoundOn = !ambientSoundOn;
            if (ambientSoundOn) {
                PlayAmbience();
            } else {
                StopAmbience();
            }
>>>>>>> Stashed changes
            break;
    }
    glutPostRedisplay();
}

// --- Idle handler for animation and updates ---
/* Updates all animated systems between frames */
void idle() {
    if (animateTime) {
        dayTime += deltaTime * timeSpeed;
        if (dayTime >= 24.0f) dayTime = 0.0f;
    }
    updateDeltaTime();
    if (animateLight) {
        updateDayCycle();
    }
    viewCameraUpdate(camera, deltaTime);
    if (snowEnabled && snowSystem) {
        weatherParticleSystemUpdate(snowSystem, landscape, deltaTime, weatherIntensity, lastTime);
    }
    if (rainEnabled && rainSystem) {
        weatherParticleSystemUpdate(rainSystem, landscape, deltaTime, weatherIntensity, lastTime);
    }
    updateTreeAnimation();
    if (gullFlock) {
        gullFlockUpdate(gullFlock, landscape, deltaTime);
    }
    if (animateWater) {
        waterTime += deltaTime;
    }
<<<<<<< Updated upstream
    glutPostRedisplay();
}

// --- Weather system initialization ---
/* Creates and configures the particle systems for weather effects */
void initWeatherSystem() {
    printf("Initializing weather system...\n");
    snowSystem = weatherParticleSystemCreate(SNOW_PARTICLES, WEATHER_PARTICLE_SNOW);
    if (!snowSystem) {
        fprintf(stderr, "Failed to create particle system\n");
        return;
    }
    printf("Weather system initialized: Type=%d, Particles=%d\n", 
           snowSystem->type, snowSystem->maxParticles);
    snowSystem->emitterY = lightHeight;
    snowSystem->emitterRadius = LANDSCAPE_SCALE * 0.5f;
}

// --- Main entry point ---
/* Program initialization and main loop entry */
=======
    if (snowOn) {
        particleSystemUpdate(deltaTime);
    }
    glutPostRedisplay();
}

>>>>>>> Stashed changes
int main(int argc, char* argv[]) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);
    int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
    int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Project: Sanjay Baskaran");
    
    // Initialize all major subsystems
    landscape = landscapeCreate();
    if (!landscape) {
        fprintf(stderr, "Failed to create landscape\n");
        return 1;
    }
    camera = viewCameraCreate();
    if (!camera) {
        fprintf(stderr, "Failed to create camera\n");
        return 1;
    }
<<<<<<< Updated upstream
    
    // Set up initial camera configuration
    lastOrbitYaw = INIT_ORBIT_YAW;
    lastOrbitPitch = INIT_ORBIT_PITCH;
    lastOrbitDistance = INIT_ORBIT_DISTANCE;
    lastOrbitTh = INIT_ORBIT_TH;
    lastOrbitPh = INIT_ORBIT_PH < 10 ? 10 : INIT_ORBIT_PH;
    lastOrbitDim = INIT_ORBIT_DIM;
    lastFPPos[0] = INIT_FP_POS[0];
    lastFPPos[2] = INIT_FP_POS[2];
    float groundHeight = landscapeGetHeight(landscape, lastFPPos[0], lastFPPos[2]);
    lastFPPos[1] = groundHeight + 2.0f;
    lastFPYaw = INIT_FP_YAW;
    lastFPPitch = INIT_FP_PITCH;
    camera->horizontalAngle = lastOrbitYaw;
    camera->verticalAngle = lastOrbitPitch;
    camera->orbitDistance = lastOrbitDistance;
    dim = lastOrbitDim;
    viewCameraSetMode(camera, CAMERA_MODE_FREE_ORBIT);
    viewCameraUpdateVectors(camera);
    
    // Initialize vegetation systems
    forest = forestSystemCreate();
    if (!forest) {
        fprintf(stderr, "Failed to create forest system\n");
        return 1;
    }
    
    // Initialize weather systems
    printf("Initializing weather system...\n");
    snowSystem = weatherParticleSystemCreate(SNOW_PARTICLES, WEATHER_PARTICLE_SNOW);
    if (!snowSystem) {
        fprintf(stderr, "Failed to create particle system\n");
        return 1;
    }
    snowSystem->emitterY = lightHeight;
    snowSystem->emitterRadius = LANDSCAPE_SCALE * 0.5f;
    rainSystem = weatherParticleSystemCreate(RAIN_PARTICLES, WEATHER_PARTICLE_RAIN);
    if (!rainSystem) {
        fprintf(stderr, "Failed to create rain particle system\n");
        return 1;
    }
    rainSystem->emitterY = lightHeight;
    rainSystem->emitterRadius = LANDSCAPE_SCALE * 0.5f;
    
    // Initialize sky and cloud systems
    skySystemInit(&skySystemInstance);
    cloudSystem = skyCloudSystemCreate(LANDSCAPE_SCALE * 0.4f);
=======
 
    camera->orbitYaw = 45.0f;
    camera->orbitPitch = 10.0f;
    camera->orbitDistance = 70.0f;
    th = 45; ph = 10; dim = 70.0f;
    viewCameraSetMode(camera, CAMERA_MODE_FREE_ORBIT);
    viewCameraUpdateVectors(camera);
    
    skySystemInitialize(&skySystemInstance);
    cloudSystem = atmosphericCloudSystemCreate(LANDSCAPE_SCALE * 0.4f);
>>>>>>> Stashed changes
    if (!cloudSystem) {
        fprintf(stderr, "Failed to create cloud system\n");
        return 1;
    }
<<<<<<< Updated upstream
    
    // Load shaders and textures
    terrainShader = loadShader("shaders/terrain_shader.vert", "shaders/terrain_shader.frag");
    if (!terrainShader) {
        fprintf(stderr, "Failed to load terrain shaders\n");
        return 1;
    }
    skyShader = loadShader("shaders/sky_shader.vert", "shaders/sky_shader.frag");
    if (!skyShader) {
        fprintf(stderr, "Failed to load sky shaders\n");
        return 1;
    }
    if (!(grassTexture = LoadTexBMP("tex/grassland.bmp"))) {
        fprintf(stderr, "Failed to load grass texture\n");
        return 1;
    }
=======

>>>>>>> Stashed changes
    if (!(rockTexture = LoadTexBMP("tex/rocky.bmp"))) {
        fprintf(stderr, "Failed to load rock texture\n");
        return 1;
    }
    if (!(sandTexture = LoadTexBMP("tex/sandy.bmp"))) {
        fprintf(stderr, "Failed to load sand texture\n");
        return 1;
    }
    
    // Generate scene content
    forestSystemGenerate(forest, landscape);
    setupLighting();
    lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    
    // Initialize decorative scene elements
    rocks = rockFieldCreate(100);
    rockFieldGenerate(rocks, landscape);
    shrubs = shrubFieldCreate(80);
    shrubFieldGenerate(shrubs, landscape);
    logs = logFieldCreate(60);
    logFieldGenerate(logs, landscape);
    
<<<<<<< Updated upstream
    // Load gull OBJ model
    gullFlock = gullFlockCreate(8, 4, LANDSCAPE_SCALE);
=======
    if (!InitAudio()) {
        fprintf(stderr, "Failed to initialize audio system.\n");
    } else {
        PlayAmbience();
    }
>>>>>>> Stashed changes
    
    // Set up GLUT callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(special);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(NULL);
    
    // Enter main loop
    glutMainLoop();
    
    // Cleanup resources
    weatherParticleSystemDestroy(snowSystem);
    weatherParticleSystemDestroy(rainSystem);
    forestSystemDestroy(forest);
    landscapeDestroy(landscape);
<<<<<<< Updated upstream
    skyCloudSystemDestroy(cloudSystem);
    deleteShader(terrainShader);
    deleteShader(skyShader);
    viewCameraDestroy(camera);
    rockFieldDestroy(rocks);
    shrubFieldDestroy(shrubs);
    logFieldDestroy(logs);
    skySystemDestroy(&skySystemInstance);
    gullFlockDestroy(gullFlock);
    
=======
    freeBoulders();
    freeLandscapeObjects();
    atmosphericCloudSystemDestroy(cloudSystem);
    viewCameraDestroy(camera);
    particleSystemCleanup();
    grassSystemCleanup();
    CleanupAudio();
>>>>>>> Stashed changes
    return 0;
}