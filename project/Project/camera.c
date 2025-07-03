<<<<<<< Updated upstream
// ---------------------------------------------
// camera.c - Camera system for orbit and first-person views
// ---------------------------------------------

=======
/*
 * Original implementation by Sanjay Baskaran for CSCI 5229 Final Project.
 *
 * Followed the below resources for conceptual reference:
 *   - LearnOpenGL Camera chapter (https://learnopengl.com/Getting-started/Camera)
 *   - OpenGL-tutorial.org (https://www.opengl-tutorial.org/beginners-tutorials/tutorial-6-keyboard-and-mouse/)
 *   - https://nerdhut.de/2019/12/04/arcball-camera-opengl
 *   - Class Examples
 */

#include "CSCIx229.h"
>>>>>>> Stashed changes
#include "camera.h"
#include "landscape.h"
#include <math.h>
#include <stdlib.h>
<<<<<<< Updated upstream
#include <stdio.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

extern Landscape* landscape;

/* Camera system constants */
#define PI 3.14159265359f
#define DEG2RAD (PI/180.0f)
#define MIN_DISTANCE 1.0f
#define MAX_DISTANCE 300.0f
#define EYE_HEIGHT 2.0f
#define CAMERA_SPEED 20.0f
#define MIN_ZOOM 5.0f
#define MAX_ZOOM 1000.0f
#define ZOOM_SPEED 0.1f
=======

extern Landscape* landscape;

#define RADIAN(x) ((x) * 0.01745329252f)
#define CAM_MIN_DIST 8.0f
#define CAM_MAX_DIST 800.0f
#define CAM_EYE_LVL 1.8f
#define CAM_WALK_SPEED 18.0f
#define CAM_ORBIT_SENS 0.35f
#define CAM_FP_SENS 0.18f
>>>>>>> Stashed changes

static void clampCameraToBounds(ViewCamera* cam);

<<<<<<< Updated upstream
// --- Camera creation and initialization ---
ViewCamera* viewCameraCreate(void) {
    /* Allocate and initialize a new camera with default values
       Initial mode is orbit view looking at the center of the landscape */
    ViewCamera* cam = (ViewCamera*)malloc(sizeof(ViewCamera));
    if (!cam) return NULL;
    cam->mode = CAMERA_MODE_FREE_ORBIT;
    cam->lookAt[0] = 0.0f;
    cam->lookAt[1] = 0.0f;
    cam->lookAt[2] = 0.0f;
    cam->horizontalAngle = 45.0f;
    cam->verticalAngle = 10.0f;
    cam->orbitDistance = LANDSCAPE_SCALE * 0.8f;
    float yawRad = cam->horizontalAngle * DEG2RAD;
    float pitchRad = cam->verticalAngle * DEG2RAD;
    cam->position[0] = -2 * cam->orbitDistance * sin(yawRad) * cos(pitchRad);
    cam->position[1] = 2 * cam->orbitDistance * sin(pitchRad);
    cam->position[2] = 2 * cam->orbitDistance * cos(yawRad) * cos(pitchRad);
    cam->upVec[0] = 0.0f;
    cam->upVec[1] = 1.0f;
    cam->upVec[2] = 0.0f;
    viewCameraUpdateVectors(cam);
    return cam;
}

// --- Update camera vectors based on mode and angles ---
void viewCameraUpdateVectors(ViewCamera* cam) {
    float yawRad = cam->horizontalAngle * DEG2RAD;
    float pitchRad = cam->verticalAngle * DEG2RAD;
    switch(cam->mode) {
        case CAMERA_MODE_FIRST_PERSON: {
            /* First person: Camera stays at position, look direction changes
               with rotation angles. Front vector calculation determines where
               the camera is looking based on yaw and pitch. */
            float frontX = -sin(yawRad) * cos(pitchRad);
            float frontY = sin(pitchRad);
            float frontZ = -cos(yawRad) * cos(pitchRad);
            cam->lookAt[0] = cam->position[0] + frontX;
            cam->lookAt[1] = cam->position[1] + frontY;
            cam->lookAt[2] = cam->position[2] + frontZ;
            cam->upVec[0] = 0.0f;
            cam->upVec[1] = 1.0f;
            cam->upVec[2] = 0.0f;
            break;
        }
        case CAMERA_MODE_FREE_ORBIT: {
            /* Orbit mode: Camera moves around a fixed point (origin)
               Position is calculated using spherical coordinates based on
               orbit distance and rotation angles */
            cam->position[0] = -2 * cam->orbitDistance * sin(yawRad) * cos(pitchRad);
            cam->position[1] = 2 * cam->orbitDistance * sin(pitchRad);
            cam->position[2] = 2 * cam->orbitDistance * cos(yawRad) * cos(pitchRad);
            cam->lookAt[0] = 0.0f;
            cam->lookAt[1] = 0.0f;
            cam->lookAt[2] = 0.0f;
            cam->upVec[0] = 0.0f;
            cam->upVec[1] = 1.0f;
            cam->upVec[2] = 0.0f;
            break;
        }
=======
ViewCamera* viewCameraCreate(void) {
    ViewCamera* c = (ViewCamera*)calloc(1, sizeof(ViewCamera));
    if (!c) return NULL;
    c->mode = CAMERA_MODE_FREE_ORBIT;
    c->fpPosition[0] = 2.0f;
    c->fpPosition[1] = 3.0f;
    c->fpPosition[2] = 2.0f;
    c->fpYaw = 60.0f;
    c->fpPitch = 0.0f;
    c->orbitYaw = 60.0f;
    c->orbitPitch = 20.0f;
    c->orbitDistance = LANDSCAPE_SCALE * 0.7f;
    c->lookAt[0] = 0.0f;
    c->lookAt[1] = 0.0f;
    c->lookAt[2] = 0.0f;
    c->upVec[0] = 0.0f;
    c->upVec[1] = 1.0f;
    c->upVec[2] = 0.0f;
    viewCameraUpdateVectors(c);
    return c;
}

void viewCameraSetProjection(ViewCamera* cam, float fov, float aspect, float nearPlane, float farPlane) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, aspect, cam->mode == CAMERA_MODE_FIRST_PERSON ? 0.12f : nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
}

void viewCameraUpdateVectors(ViewCamera* cam) {
    if (!cam) return;
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        float y = sinf(RADIAN(cam->fpPitch));
        float r = cosf(RADIAN(cam->fpPitch));
        float x = -sinf(RADIAN(cam->fpYaw)) * r;
        float z = -cosf(RADIAN(cam->fpYaw)) * r;
        cam->lookAt[0] = cam->fpPosition[0] + x;
        cam->lookAt[1] = cam->fpPosition[1] + y;
        cam->lookAt[2] = cam->fpPosition[2] + z;
        cam->upVec[0] = 0.0f;
        cam->upVec[1] = 1.0f;
        cam->upVec[2] = 0.0f;
    } else if (cam->mode == CAMERA_MODE_FREE_ORBIT) {
        float r = cam->orbitDistance * cosf(RADIAN(cam->orbitPitch));
        float y = cam->orbitDistance * sinf(RADIAN(cam->orbitPitch));
        float x = r * -sinf(RADIAN(cam->orbitYaw));
        float z = r * cosf(RADIAN(cam->orbitYaw));
        cam->fpPosition[0] = x;
        cam->fpPosition[1] = y;
        cam->fpPosition[2] = z;
        cam->lookAt[0] = 0.0f;
        cam->lookAt[1] = 0.0f;
        cam->lookAt[2] = 0.0f;
        cam->upVec[0] = 0.0f;
        cam->upVec[1] = 1.0f;
        cam->upVec[2] = 0.0f;
>>>>>>> Stashed changes
    }
}

void viewCameraMove(ViewCamera* cam, int direction, float deltaTime) {
    if (!cam) return;
<<<<<<< Updated upstream
    float speed = CAMERA_SPEED * deltaTime;
    float yawRad = cam->horizontalAngle * DEG2RAD;
    
    /* Calculate forward and right vectors for movement direction */
    float forward[3] = {
        -sin(yawRad),
        0.0f,
        -cos(yawRad)
    };
    float right[3] = {
        cos(yawRad),
        0.0f,
        -sin(yawRad)
    };
    
    float moveX = 0.0f;
    float moveZ = 0.0f;
    switch(direction) {
        case CAMERA_MOVE_FORWARD:
            moveX = forward[0] * speed;
            moveZ = forward[2] * speed;
            break;
        case CAMERA_MOVE_BACKWARD:
            moveX = -forward[0] * speed;
            moveZ = -forward[2] * speed;
            break;
        case CAMERA_MOVE_LEFT:
            moveX = -right[0] * speed;
            moveZ = -right[2] * speed;
            break;
        case CAMERA_MOVE_RIGHT:
            moveX = right[0] * speed;
            moveZ = right[2] * speed;
            break;
    }
    
    float halfScale = LANDSCAPE_SCALE * 0.5f;
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        /* First person: Move camera along terrain, maintaining eye height */
        float newX = cam->position[0] + moveX;
        float newZ = cam->position[2] + moveZ;
        if (newX >= -halfScale && newX <= halfScale && 
            newZ >= -halfScale && newZ <= halfScale) {
            float groundHeight = landscapeGetHeight(landscape, newX, newZ);
            cam->position[0] = newX;
            cam->position[2] = newZ;
            cam->position[1] = groundHeight + EYE_HEIGHT;
        }
    } else if (cam->mode == CAMERA_MODE_FREE_ORBIT) {
        /* Orbit mode: Movement changes orbit distance or horizontal angle */
        switch(direction) {
            case CAMERA_MOVE_FORWARD:
                cam->orbitDistance = fmax(10.0f, cam->orbitDistance - speed * 10);
                break;
            case CAMERA_MOVE_BACKWARD:
                cam->orbitDistance = fmin(500.0f, cam->orbitDistance + speed * 10);
                break;
            case CAMERA_MOVE_LEFT:
                cam->horizontalAngle -= speed * 100;
                break;
            case CAMERA_MOVE_RIGHT:
                cam->horizontalAngle += speed * 100;
                break;
=======
    float moveStep = CAM_WALK_SPEED * deltaTime;
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        float yawRad = RADIAN(cam->fpYaw);
        float dx = 0, dz = 0;
        if (direction == CAMERA_MOVE_FORWARD) {
            dx = -sinf(yawRad) * moveStep;
            dz = -cosf(yawRad) * moveStep;
        } else if (direction == CAMERA_MOVE_BACKWARD) {
            dx = sinf(yawRad) * moveStep;
            dz = cosf(yawRad) * moveStep;
        } else if (direction == CAMERA_MOVE_LEFT) {
            dx = -cosf(yawRad) * moveStep;
            dz = sinf(yawRad) * moveStep;
        } else if (direction == CAMERA_MOVE_RIGHT) {
            dx = cosf(yawRad) * moveStep;
            dz = -sinf(yawRad) * moveStep;
        }
        float nx = cam->fpPosition[0] + dx;
        float nz = cam->fpPosition[2] + dz;
        float half = LANDSCAPE_SCALE * 0.5f;
        if (nx >= -half && nx <= half && nz >= -half && nz <= half) {
            float h = landscapeGetHeight(landscape, nx, nz);
            cam->fpPosition[0] = nx;
            cam->fpPosition[2] = nz;
            cam->fpPosition[1] = h + CAM_EYE_LVL;
        }
    } else if (cam->mode == CAMERA_MODE_FREE_ORBIT) {
        if (direction == CAMERA_MOVE_FORWARD) {
            cam->orbitDistance -= moveStep * 6.0f;
        } else if (direction == CAMERA_MOVE_BACKWARD) {
            cam->orbitDistance += moveStep * 6.0f;
        } else if (direction == CAMERA_MOVE_LEFT) {
            cam->orbitYaw -= moveStep * 60.0f;
        } else if (direction == CAMERA_MOVE_RIGHT) {
            cam->orbitYaw += moveStep * 60.0f;
>>>>>>> Stashed changes
        }
        if (cam->orbitDistance < CAM_MIN_DIST) cam->orbitDistance = CAM_MIN_DIST;
        if (cam->orbitDistance > CAM_MAX_DIST) cam->orbitDistance = CAM_MAX_DIST;
    }
    viewCameraUpdateVectors(cam);
}

<<<<<<< Updated upstream
void viewCameraRotate(ViewCamera* cam, float deltaX, float deltaY) {
    if (!cam) return;
    /* Adjust camera orientation based on mouse movement
       Different sensitivity based on camera mode */
    float sensitivity = (cam->mode == CAMERA_MODE_FREE_ORBIT) ? 0.4f : 0.2f;
    cam->horizontalAngle += deltaX * sensitivity;
    cam->verticalAngle += deltaY * sensitivity;
    
    /* Constrain vertical angle to prevent camera flipping */
    if (cam->verticalAngle > 89.0f) cam->verticalAngle = 89.0f;
    if (cam->verticalAngle < -89.0f) cam->verticalAngle = -89.0f;
    
    /* Normalize horizontal angle to 0-360 range */
    if (cam->horizontalAngle >= 360.0f) cam->horizontalAngle -= 360.0f;
    if (cam->horizontalAngle < 0.0f) cam->horizontalAngle += 360.0f;
    
    viewCameraUpdateVectors(cam);
}

void viewCameraZoom(ViewCamera* cam, float factor) {
    /* Zoom by adjusting orbit distance - only applies to orbit mode */
    if (!cam || cam->mode != CAMERA_MODE_FREE_ORBIT) return;
    float zoomAmount = (factor - 1.0f) * cam->orbitDistance * ZOOM_SPEED;
    cam->orbitDistance += zoomAmount;
    if (cam->orbitDistance < MIN_ZOOM) cam->orbitDistance = MIN_ZOOM;
    if (cam->orbitDistance > LANDSCAPE_SCALE * 2.0f) cam->orbitDistance = LANDSCAPE_SCALE * 2.0f;
=======
void viewCameraRotate(ViewCamera* cam, float deltaYaw, float deltaPitch) {
    if (!cam) return;
    if (cam->mode == CAMERA_MODE_FREE_ORBIT) {
        cam->orbitYaw += deltaYaw * CAM_ORBIT_SENS;
        cam->orbitPitch += deltaPitch * CAM_ORBIT_SENS;
        if (cam->orbitPitch > 89.0f) cam->orbitPitch = 89.0f;
        if (cam->orbitPitch < -89.0f) cam->orbitPitch = -89.0f;
    } else if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        cam->fpYaw += deltaYaw * CAM_FP_SENS;
        cam->fpPitch += deltaPitch * CAM_FP_SENS;
        if (cam->fpPitch > 89.0f) cam->fpPitch = 89.0f;
        if (cam->fpPitch < -89.0f) cam->fpPitch = -89.0f;
    }
>>>>>>> Stashed changes
    viewCameraUpdateVectors(cam);
}

void viewCameraSetMode(ViewCamera* cam, CameraMode newMode) {
    if (!cam) return;
    if (cam->mode == newMode) return;
    cam->mode = newMode;
    if (newMode == CAMERA_MODE_FIRST_PERSON) {
        float h = landscapeGetHeight(landscape, cam->fpPosition[0], cam->fpPosition[2]);
        cam->fpPosition[1] = h + CAM_EYE_LVL;
    } else if (newMode == CAMERA_MODE_FREE_ORBIT) {
        cam->orbitDistance = LANDSCAPE_SCALE * 0.7f;
        cam->orbitYaw = 60.0f;
        cam->orbitPitch = 20.0f;
    }
    viewCameraUpdateVectors(cam);
}

void viewCameraUpdate(ViewCamera* cam, float deltaTime) {
    /* Update camera position based on current mode and terrain */
    if (!cam) return;
<<<<<<< Updated upstream
    float groundHeight = landscapeGetHeight(landscape, cam->position[0], cam->position[2]);
    switch(cam->mode) {
        case CAMERA_MODE_FIRST_PERSON:
            cam->position[1] = groundHeight + EYE_HEIGHT;
            break;
        case CAMERA_MODE_FREE_ORBIT:
            break;
=======
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        float h = landscapeGetHeight(landscape, cam->fpPosition[0], cam->fpPosition[2]);
        cam->fpPosition[1] = h + CAM_EYE_LVL;
>>>>>>> Stashed changes
    }
    clampCameraToBounds(cam);
}

<<<<<<< Updated upstream
static void constrainToTerrain(ViewCamera* cam) {
    /* Keep camera within the landscape boundaries */
    float halfScale = LANDSCAPE_SCALE * 0.5f;
    if (cam->mode != CAMERA_MODE_FREE_ORBIT) {
        cam->position[0] = fmax(-halfScale, fmin(halfScale, cam->position[0]));
        cam->position[2] = fmax(-halfScale, fmin(halfScale, cam->position[2]));
=======
static void clampCameraToBounds(ViewCamera* cam) {
    float half = LANDSCAPE_SCALE * 0.5f;
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        if (cam->fpPosition[0] < -half) cam->fpPosition[0] = -half;
        if (cam->fpPosition[0] > half) cam->fpPosition[0] = half;
        if (cam->fpPosition[2] < -half) cam->fpPosition[2] = -half;
        if (cam->fpPosition[2] > half) cam->fpPosition[2] = half;
>>>>>>> Stashed changes
    }
}

void viewCameraReset(ViewCamera* cam) {
    /* Reset camera to default viewing position and orbit mode */
    if (!cam) return;
    cam->horizontalAngle = -60.0f;
    cam->verticalAngle = 30.0f;
    cam->orbitDistance = LANDSCAPE_SCALE * 0.8f;
    viewCameraSetMode(cam, CAMERA_MODE_FREE_ORBIT);
}

void viewCameraDestroy(ViewCamera* cam) {
<<<<<<< Updated upstream
    free(cam);
}

void viewCameraSetProjection(ViewCamera* cam, float fov, float aspect, float nearPlane, float farPlane) {
    /* Set up projection matrix based on camera mode
       First person uses closer near plane for better precision */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        gluPerspective(fov, aspect, 0.1f, farPlane);
    } else {
        gluPerspective(fov, aspect, nearPlane, farPlane);
    }
    glMatrixMode(GL_MODELVIEW);
=======
    if (cam) free(cam);
>>>>>>> Stashed changes
}