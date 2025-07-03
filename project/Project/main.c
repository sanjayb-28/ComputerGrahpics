#include "CSCIx229.h"
#include "landscape.h"
#include "shaders.h"
#include "sky.h"
#include "sky_clouds.h"
#include "camera.h"
#include "fractal_tree.h"
#include "objects_render.h"
#include "particles.h"
#include "grass.h"
#include "sound.h"
#include "boulder.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static AtmosphericCloudSystem* cloudSystem = NULL;
static SkySystem skySystemInstance;

GLuint rockTexture = 0;
GLuint sandTexture = 0;
GLuint boulderTexture = 0;
GLuint barkTexture = 0;
GLuint leafTexture = 0;

static int th = 45;
static int ph = 10;
static float dim = 100;
static int fov = 1;
      
static float lightHeight = 250.0f;
static float dayTime = 0.0f;
static float windStrength = 0.0f;

#define WATER_LEVEL -4.0f
static float waterTime = 0.0f;
static int animateWater = 1;

static int wireframe = 0;
static int showAxes = 0;

Landscape* landscape = NULL;

static float lastTime = 0;
static float deltaTime = 0;

int fogEnabled = 0;

static int animateTime = 1;
static float timeSpeed = 1.0f;

static ViewCamera* camera = NULL;
float asp;
static int lastX = 0, lastY = 0;
static int mouseButtons = 0;

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

void reshape(int width, int height);
void display();
void special(int key, int x, int y);
void keyboard(unsigned char key, int x, int y);
void idle();

void reshape(int width, int height) {
    asp = (height>0) ? (double)width/height : 1;
    glViewport(0,0, RES*width,RES*height);
    if (camera) {
        viewCameraSetProjection(camera, 55.0f, asp, dim/4, dim*4);
    } else {
        Project(fov?55:0, asp, dim);
    }
}

void updateDeltaTime() {
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
}

float smoothstep(float edge0, float edge1, float x) {
    float t = (x - edge0) / (edge1 - edge0);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

void getSkyColor(float time, float* color) {
    float t = time / 24.0f;  
    const int NUM_COLORS = 6;
    float timePoints[6] = {0.0f, 0.25f, 0.4f, 0.6f, 0.75f, 1.0f};  
    float colors[6][3] = {
        {0.02f, 0.02f, 0.1f},
        {0.7f, 0.4f, 0.4f},
        {0.4f, 0.7f, 1.0f},
        {0.4f, 0.7f, 1.0f},
        {0.7f, 0.4f, 0.4f},
        {0.02f, 0.02f, 0.1f}
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

void updateTreeAnimation() {
    static float windTime = 0.0f;
    windTime += deltaTime;
    windStrength = sin(windTime * 0.5f) * 0.5f + 0.5f;
    treeSwayAngle = sin(windTime) * 8.0f;
}

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

void updateFog(float dayTime) {
    float timeNormalized = dayTime / 24.0f;
    float sunAngle = (timeNormalized - 0.25f) * 2 * M_PI;
    float sunHeight = sin(sunAngle);
    float baseDensity = 0.008f;
    float fogColor[4] = {0.95f, 0.95f, 0.95f, 1.0f};
    
    if (sunHeight <= 0) {
        fogColor[0] = fogColor[1] = fogColor[2] = 0.7f;
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
    } else {
        glDisable(GL_FOG);
    }
}

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

void display() {
    float skyColor[3];
    getSkyColor(dayTime, skyColor);
    glClearColor(skyColor[0], skyColor[1], skyColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(camera->fpPosition[0], camera->fpPosition[1], camera->fpPosition[2],
              camera->lookAt[0], camera->lookAt[1], camera->lookAt[2],
              camera->upVec[0], camera->upVec[1], camera->upVec[2]);
    skySystemRender(&skySystemInstance, dayTime);
    updateFog(dayTime);
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
    }
    grassSystemRender(dayTime, windStrength, sunDir, ambient);
    renderLandscapeObjects(landscape);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    landscapeRenderWater(WATER_LEVEL, landscape, dayTime);
    glDepthMask(GL_TRUE);
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
    glDisable(GL_DEPTH_TEST);
    glColor3f(1,1,1);
    glWindowPos2i(5, glutGet(GLUT_WINDOW_HEIGHT) - 20);
    Print("Time: %02d:%02d  Weather: %s", 
          (int)dayTime, (int)((dayTime-(int)dayTime)*60),
          weatherType == 1 ? "Winter" : "Fall");
    int y = 5;
    glWindowPos2i(5, y);
    Print("Angle=%d,%d  Dim=%.1f  View=%s   |   Water=%.1f   |   Wireframe=%d   |   Axes=%d   |   TimeAnim: %s  Speed: %.1fx   |   Fog: %s  Snow: %s  |   Sound: %s",
        th, ph, dim, camera->mode == CAMERA_MODE_FREE_ORBIT ? "Free Orbit" : "First Person",
        WATER_LEVEL,
        wireframe,
        showAxes,
        animateTime ? "On" : "Off", timeSpeed,
        fogEnabled ? "On" : "Off",
        snowOn ? "On" : "Off",
        ambientSoundOn ? "On" : "Off");
    glEnable(GL_DEPTH_TEST);
    if (snowOn) {
        particleSystemRender();
    }
    glutSwapBuffers();
}

void mouse(int button, int state, int x, int y) {
    lastX = x;
    lastY = y;
    if (state == GLUT_DOWN) {
        mouseButtons |= 1<<button;
    } else {
        mouseButtons &= ~(1<<button);
    }
    glutPostRedisplay();
}

void mouseMotion(int x, int y) {
    int dx = x - lastX;
    int dy = y - lastY;
    if (camera->mode != CAMERA_MODE_FREE_ORBIT && (mouseButtons & 1)) {
        viewCameraRotate(camera, dx * 0.5f, -dy * 0.5f);
    }
    lastX = x;
    lastY = y;
    glutPostRedisplay();
}

void special(int key, int x, int y) {
    float deltaTime = 0.016f;
    if (camera && camera->mode == CAMERA_MODE_FREE_ORBIT) {
        switch(key) {
            case GLUT_KEY_RIGHT:
                th += 5;
                camera->orbitYaw = th;  
                break;
            case GLUT_KEY_LEFT:
                th -= 5;
                camera->orbitYaw = th;
                break;
            case GLUT_KEY_UP:
                ph += 5;
                camera->orbitPitch = ph; 
                break;
            case GLUT_KEY_DOWN:
                ph -= 5;
                camera->orbitPitch = ph;
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

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 27:
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
            th = 45; ph = 10; dim = 70.0f;
            camera->orbitYaw = th;
            camera->orbitPitch = ph;
            camera->orbitDistance = dim;
            clampAndSyncDim();
            waterTime = 0.0f;
            lightHeight = 250.0f;
            fov = 1;
            viewCameraSetMode(camera, CAMERA_MODE_FREE_ORBIT);
            viewCameraUpdateVectors(camera);
            break;
        case '1': {
            camera->fpYaw = 45.0f;
            camera->fpPitch = 10.0f;
            camera->fpPosition[0] = 0.0f;
            camera->fpPosition[2] = 0.0f;
            float groundHeight = landscapeGetHeight(landscape, camera->fpPosition[0], camera->fpPosition[2]);
            camera->fpPosition[1] = groundHeight + 2.0f;
            viewCameraSetMode(camera, CAMERA_MODE_FIRST_PERSON);
            viewCameraUpdateVectors(camera);
            viewCameraSetProjection(camera, 55.0f, asp, dim/4, dim*4);
            break;
        }
        case '2': {
            camera->orbitYaw = 45.0f;
            camera->orbitPitch = 10.0f;
            camera->orbitDistance = 70.0f;
            th = 45; ph = 10; dim = 70.0f;
            clampAndSyncDim();
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
        case 'b':
            fogEnabled = !fogEnabled;
            break;
        case 'z':
            if (camera->mode == CAMERA_MODE_FREE_ORBIT) {
                dim -= 5.0f;
                camera->orbitDistance -= 5.0f;
                clampAndSyncDim();
                viewCameraUpdateVectors(camera);
                Project(fov?55:0, asp, dim);
            }
            break;
        case 'Z':
            if (camera->mode == CAMERA_MODE_FREE_ORBIT) {
                dim += 5.0f;
                camera->orbitDistance += 5.0f;
                clampAndSyncDim();
                viewCameraUpdateVectors(camera);
                Project(fov?55:0, asp, dim);
            }
            break;
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
            break;
    }
    glutPostRedisplay();
}

void idle() {
    if (animateTime) {
        dayTime += deltaTime * timeSpeed;
        if (dayTime >= 24.0f) dayTime = 0.0f;
    }
    updateDeltaTime();
    viewCameraUpdate(camera, deltaTime);
    updateTreeAnimation();
    if (animateWater) {
        waterTime += deltaTime;
    }
    if (snowOn) {
        particleSystemUpdate(deltaTime);
    }
    glutPostRedisplay();
}

int main(int argc, char* argv[]) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);
    int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
    int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Project: Sanjay Baskaran");
#ifdef USEGLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW initialization failed: %s\n", glewGetErrorString(err));
        return 1;
    }
#endif
    
    landscape = landscapeCreate();
    if (!landscape) {
        fprintf(stderr, "Failed to create landscape\n");
        return 1;
    }
    grassSystemInit(landscape, LANDSCAPE_SCALE, 500000);
    particleSystemUploadHeightmap(landscape->elevationData);
    initLandscapeObjects(landscape);
    initBoulders(landscape);
    camera = viewCameraCreate();
    if (!camera) {
        fprintf(stderr, "Failed to create camera\n");
        return 1;
    }
 
    camera->orbitYaw = 45.0f;
    camera->orbitPitch = 10.0f;
    camera->orbitDistance = 70.0f;
    th = 45; ph = 10; dim = 70.0f;
    viewCameraSetMode(camera, CAMERA_MODE_FREE_ORBIT);
    viewCameraUpdateVectors(camera);
    
    skySystemInitialize(&skySystemInstance);
    cloudSystem = atmosphericCloudSystemCreate(LANDSCAPE_SCALE * 0.4f);
    if (!cloudSystem) {
        fprintf(stderr, "Failed to create cloud system\n");
        return 1;
    }

    if (!(rockTexture = LoadTexBMP("tex/rocky.bmp"))) {
        fprintf(stderr, "Failed to load rock texture\n");
        return 1;
    }
    if (!(sandTexture = LoadTexBMP("tex/sandy.bmp"))) {
        fprintf(stderr, "Failed to load sand texture\n");
        return 1;
    }
    if (!(boulderTexture = LoadTexBMP("tex/boulder.bmp"))) {
        fprintf(stderr, "Failed to load boulder texture\n");
        return 1;
    }
    if (!(barkTexture = LoadTexBMP("tex/bark.bmp"))) {
        fprintf(stderr, "Failed to load bark texture\n");
        return 1;
    }
    if (!(leafTexture = LoadTexBMP("tex/leaf.bmp"))) {
        fprintf(stderr, "Failed to load leaf texture\n");
        return 1;
    }
    
    fractalTreeInit();
    boulderShaderInit();
    
    setupLighting();
    lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    
    particleSystemInit(2000.0f, 20000.0f);
    
    if (!InitAudio()) {
        fprintf(stderr, "Failed to initialize audio system.\n");
    } else {
        PlayAmbience();
    }
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(special);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(NULL);
    
    glutMainLoop();
    
    landscapeDestroy(landscape);
    freeBoulders();
    freeLandscapeObjects();
    atmosphericCloudSystemDestroy(cloudSystem);
    viewCameraDestroy(camera);
    particleSystemCleanup();
    grassSystemCleanup();
    CleanupAudio();
    return 0;
}