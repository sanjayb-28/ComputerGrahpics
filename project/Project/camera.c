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
#include "camera.h"
#include "landscape.h"
#include <math.h>
#include <stdlib.h>

extern Landscape* landscape;

#define RADIAN(x) ((x) * 0.01745329252f)
#define CAM_MIN_DIST 8.0f
#define CAM_MAX_DIST 800.0f
#define CAM_EYE_LVL 1.8f
#define CAM_WALK_SPEED 18.0f
#define CAM_ORBIT_SENS 0.35f
#define CAM_FP_SENS 0.18f

static void clampCameraToBounds(ViewCamera* cam);

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
    }
}

void viewCameraMove(ViewCamera* cam, int direction, float deltaTime) {
    if (!cam) return;
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
        }
        if (cam->orbitDistance < CAM_MIN_DIST) cam->orbitDistance = CAM_MIN_DIST;
        if (cam->orbitDistance > CAM_MAX_DIST) cam->orbitDistance = CAM_MAX_DIST;
    }
    viewCameraUpdateVectors(cam);
}

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
    if (!cam) return;
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        float h = landscapeGetHeight(landscape, cam->fpPosition[0], cam->fpPosition[2]);
        cam->fpPosition[1] = h + CAM_EYE_LVL;
    }
    clampCameraToBounds(cam);
}

static void clampCameraToBounds(ViewCamera* cam) {
    float half = LANDSCAPE_SCALE * 0.5f;
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        if (cam->fpPosition[0] < -half) cam->fpPosition[0] = -half;
        if (cam->fpPosition[0] > half) cam->fpPosition[0] = half;
        if (cam->fpPosition[2] < -half) cam->fpPosition[2] = -half;
        if (cam->fpPosition[2] > half) cam->fpPosition[2] = half;
    }
}

void viewCameraDestroy(ViewCamera* cam) {
    if (cam) free(cam);
}