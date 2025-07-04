/*
 * Camera System for Boulder Scene - Interactive View Control and Navigation
 *
 * This component implements a dual-mode camera system that provides both first-person and orbital viewing
 * experiences for the 3D scene. It handles user input, camera transformations, terrain collision detection,
 * and view matrix calculations. The system supports smooth navigation with terrain-following and boundary
 * constraints for immersive exploration of the procedural landscape.
 *
 * Key Concepts:
 * - Dual Camera Modes: First-person (walking) and free-orbit (flying) modes with seamless transitions.
 * - Terrain Integration: Camera height automatically follows the landscape surface for realistic movement.
 * - Boundary Constraints: Prevents camera from leaving the valid terrain area for consistent experience.
 * - Input Processing: Handles keyboard movement and mouse rotation with configurable sensitivity.
 * - View Matrix Management: Automatically updates camera vectors and OpenGL view transformations.
 *
 * Function Roles:
 * - viewCameraCreate: Initializes a new camera with default settings and memory allocation.
 * - viewCameraSetProjection: Configures OpenGL projection matrix for different camera modes.
 * - viewCameraUpdateVectors: Recalculates camera position, look-at point, and up vector based on mode.
 * - viewCameraMove: Handles keyboard input for camera movement with terrain collision detection.
 * - viewCameraRotate: Processes mouse input for camera rotation with sensitivity controls.
 * - viewCameraSetMode: Switches between camera modes with appropriate state transitions.
 * - viewCameraUpdate: Performs per-frame updates including terrain following and boundary checks.
 * - clampCameraToBounds: Ensures camera stays within valid terrain boundaries.
 * - viewCameraDestroy: Frees camera memory and cleans up resources.
 */

#include "CSCIx229.h"
#include "camera.h"
#include "landscape.h"
#include <math.h>
#include <stdlib.h>

extern Landscape* landscape;

// Utility macro to convert degrees to radians
#define RADIAN(x) ((x) * 0.01745329252f)
// Camera distance constraints for orbit mode
#define CAM_MIN_DIST 8.0f
#define CAM_MAX_DIST 800.0f
// Eye level height above terrain for first-person mode
#define CAM_EYE_LVL 1.8f
// Movement speed for camera navigation
#define CAM_WALK_SPEED 18.0f
// Mouse sensitivity for orbit camera rotation
#define CAM_ORBIT_SENS 0.35f
// Mouse sensitivity for first-person camera rotation
#define CAM_FP_SENS 0.18f

// Forward declaration for boundary clamping function
static void clampCameraToBounds(ViewCamera* cam);

// viewCameraCreate: Initializes a new camera with default settings and memory allocation.
// Contribution: This function is the entry point for creating a camera system. It allocates memory for the camera structure, sets up default positions and orientations for both camera modes, and initializes the camera vectors. This establishes the foundation for all camera operations in the scene.
ViewCamera* viewCameraCreate(void) {
    ViewCamera* c = (ViewCamera*)calloc(1, sizeof(ViewCamera)); // Allocate and zero-initialize camera structure
    if (!c) return NULL; // Check for allocation failure
    c->mode = CAMERA_MODE_FREE_ORBIT; // Start in orbit mode by default
    c->fpPosition[0] = 2.0f; // Initial X position for first-person mode
    c->fpPosition[1] = 3.0f; // Initial Y position (height)
    c->fpPosition[2] = 2.0f; // Initial Z position for first-person mode
    c->fpYaw = 60.0f; // Initial yaw angle for first-person mode
    c->fpPitch = 0.0f; // Initial pitch angle for first-person mode
    c->orbitYaw = 60.0f; // Initial yaw angle for orbit mode
    c->orbitPitch = 20.0f; // Initial pitch angle for orbit mode
    c->orbitDistance = LANDSCAPE_SCALE * 0.7f; // Initial orbit distance from center
    c->lookAt[0] = 0.0f; // Look-at point X (center of scene)
    c->lookAt[1] = 0.0f; // Look-at point Y (center of scene)
    c->lookAt[2] = 0.0f; // Look-at point Z (center of scene)
    c->upVec[0] = 0.0f; // Up vector X component
    c->upVec[1] = 1.0f; // Up vector Y component (world up)
    c->upVec[2] = 0.0f; // Up vector Z component
    viewCameraUpdateVectors(c); // Initialize camera vectors based on default settings
    return c; // Return the initialized camera
}

// viewCameraSetProjection: Configures OpenGL projection matrix for different camera modes.
// Contribution: This function sets up the perspective projection matrix for rendering. It uses different near plane values for first-person mode (closer near plane for better detail) versus orbit mode (standard near plane). This ensures optimal rendering quality for each camera mode.
void viewCameraSetProjection(ViewCamera* cam, float fov, float aspect, float nearPlane, float farPlane) {
    glMatrixMode(GL_PROJECTION); // Switch to projection matrix mode
    glLoadIdentity(); // Reset projection matrix
    gluPerspective(fov, aspect, cam->mode == CAMERA_MODE_FIRST_PERSON ? 0.12f : nearPlane, farPlane); // Set perspective projection with mode-specific near plane
    glMatrixMode(GL_MODELVIEW); // Switch back to modelview matrix mode
}

// viewCameraUpdateVectors: Recalculates camera position, look-at point, and up vector based on mode.
// Contribution: This function is the core of the camera system, converting camera parameters (position, angles, distance) into the actual 3D vectors needed for rendering. It handles the mathematical conversion between spherical coordinates (orbit mode) and Cartesian coordinates (first-person mode), ensuring the camera always has valid view vectors.
void viewCameraUpdateVectors(ViewCamera* cam) {
    if (!cam) return; // Early exit if camera is null
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        float y = sinf(RADIAN(cam->fpPitch)); // Calculate Y component of view direction from pitch
        float r = cosf(RADIAN(cam->fpPitch)); // Calculate radius component for XZ plane
        float x = -sinf(RADIAN(cam->fpYaw)) * r; // Calculate X component of view direction from yaw
        float z = -cosf(RADIAN(cam->fpYaw)) * r; // Calculate Z component of view direction from yaw
        cam->lookAt[0] = cam->fpPosition[0] + x; // Set look-at point X (position + view direction)
        cam->lookAt[1] = cam->fpPosition[1] + y; // Set look-at point Y (position + view direction)
        cam->lookAt[2] = cam->fpPosition[2] + z; // Set look-at point Z (position + view direction)
        cam->upVec[0] = 0.0f; // Set up vector X (world up)
        cam->upVec[1] = 1.0f; // Set up vector Y (world up)
        cam->upVec[2] = 0.0f; // Set up vector Z (world up)
    } else if (cam->mode == CAMERA_MODE_FREE_ORBIT) {
        float r = cam->orbitDistance * cosf(RADIAN(cam->orbitPitch)); // Calculate radius in XZ plane from distance and pitch
        float y = cam->orbitDistance * sinf(RADIAN(cam->orbitPitch)); // Calculate Y position from distance and pitch
        float x = r * -sinf(RADIAN(cam->orbitYaw)); // Calculate X position from radius and yaw
        float z = r * cosf(RADIAN(cam->orbitYaw)); // Calculate Z position from radius and yaw
        cam->fpPosition[0] = x; // Set camera position X (orbit position)
        cam->fpPosition[1] = y; // Set camera position Y (orbit position)
        cam->fpPosition[2] = z; // Set camera position Z (orbit position)
        cam->lookAt[0] = 0.0f; // Set look-at point X (center of scene)
        cam->lookAt[1] = 0.0f; // Set look-at point Y (center of scene)
        cam->lookAt[2] = 0.0f; // Set look-at point Z (center of scene)
        cam->upVec[0] = 0.0f; // Set up vector X (world up)
        cam->upVec[1] = 1.0f; // Set up vector Y (world up)
        cam->upVec[2] = 0.0f; // Set up vector Z (world up)
    }
}

// viewCameraMove: Handles keyboard input for camera movement with terrain collision detection.
// Contribution: This function processes user input for camera movement, handling different behaviors for each camera mode. In first-person mode, it performs terrain collision detection and height following. In orbit mode, it adjusts distance and rotation angles. This enables intuitive navigation through the 3D scene.
void viewCameraMove(ViewCamera* cam, int direction, float deltaTime) {
    if (!cam) return; // Early exit if camera is null
    float moveStep = CAM_WALK_SPEED * deltaTime; // Calculate movement distance based on speed and time
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        float yawRad = RADIAN(cam->fpYaw); // Convert yaw to radians for trigonometric calculations
        float dx = 0, dz = 0; // Initialize movement deltas
        if (direction == CAMERA_MOVE_FORWARD) {
            dx = -sinf(yawRad) * moveStep; // Calculate X movement for forward direction
            dz = -cosf(yawRad) * moveStep; // Calculate Z movement for forward direction
        } else if (direction == CAMERA_MOVE_BACKWARD) {
            dx = sinf(yawRad) * moveStep; // Calculate X movement for backward direction
            dz = cosf(yawRad) * moveStep; // Calculate Z movement for backward direction
        } else if (direction == CAMERA_MOVE_LEFT) {
            dx = -cosf(yawRad) * moveStep; // Calculate X movement for left direction
            dz = sinf(yawRad) * moveStep; // Calculate Z movement for left direction
        } else if (direction == CAMERA_MOVE_RIGHT) {
            dx = cosf(yawRad) * moveStep; // Calculate X movement for right direction
            dz = -sinf(yawRad) * moveStep; // Calculate Z movement for right direction
        }
        float nx = cam->fpPosition[0] + dx; // Calculate new X position
        float nz = cam->fpPosition[2] + dz; // Calculate new Z position
        float half = LANDSCAPE_SCALE * 0.5f; // Calculate terrain boundary
        if (nx >= -half && nx <= half && nz >= -half && nz <= half) { // Check if new position is within bounds
            float h = landscapeGetHeight(landscape, nx, nz); // Get terrain height at new position
            cam->fpPosition[0] = nx; // Update X position
            cam->fpPosition[2] = nz; // Update Z position
            cam->fpPosition[1] = h + CAM_EYE_LVL; // Update Y position to terrain height plus eye level
        }
    } else if (cam->mode == CAMERA_MODE_FREE_ORBIT) {
        if (direction == CAMERA_MOVE_FORWARD) {
            cam->orbitDistance -= moveStep * 6.0f; // Decrease orbit distance (zoom in)
        } else if (direction == CAMERA_MOVE_BACKWARD) {
            cam->orbitDistance += moveStep * 6.0f; // Increase orbit distance (zoom out)
        } else if (direction == CAMERA_MOVE_LEFT) {
            cam->orbitYaw -= moveStep * 60.0f; // Decrease yaw angle (rotate left)
        } else if (direction == CAMERA_MOVE_RIGHT) {
            cam->orbitYaw += moveStep * 60.0f; // Increase yaw angle (rotate right)
        }
        if (cam->orbitDistance < CAM_MIN_DIST) cam->orbitDistance = CAM_MIN_DIST; // Clamp minimum distance
        if (cam->orbitDistance > CAM_MAX_DIST) cam->orbitDistance = CAM_MAX_DIST; // Clamp maximum distance
    }
    viewCameraUpdateVectors(cam); // Update camera vectors after movement
}

// viewCameraRotate: Processes mouse input for camera rotation with sensitivity controls.
// Contribution: This function handles mouse input for camera rotation, applying different sensitivity settings for each camera mode. It clamps pitch angles to prevent gimbal lock and ensures smooth, responsive camera control that enhances the user experience.
void viewCameraRotate(ViewCamera* cam, float deltaYaw, float deltaPitch) {
    if (!cam) return; // Early exit if camera is null
    if (cam->mode == CAMERA_MODE_FREE_ORBIT) {
        cam->orbitYaw += deltaYaw * CAM_ORBIT_SENS; // Update orbit yaw with sensitivity
        cam->orbitPitch += deltaPitch * CAM_ORBIT_SENS; // Update orbit pitch with sensitivity
        if (cam->orbitPitch > 89.0f) cam->orbitPitch = 89.0f; // Clamp maximum pitch to prevent flipping
        if (cam->orbitPitch < -89.0f) cam->orbitPitch = -89.0f; // Clamp minimum pitch to prevent flipping
    } else if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        cam->fpYaw += deltaYaw * CAM_FP_SENS; // Update first-person yaw with sensitivity
        cam->fpPitch += deltaPitch * CAM_FP_SENS; // Update first-person pitch with sensitivity
        if (cam->fpPitch > 89.0f) cam->fpPitch = 89.0f; // Clamp maximum pitch to prevent flipping
        if (cam->fpPitch < -89.0f) cam->fpPitch = -89.0f; // Clamp minimum pitch to prevent flipping
    }
    viewCameraUpdateVectors(cam); // Update camera vectors after rotation
}

// viewCameraSetMode: Switches between camera modes with appropriate state transitions.
// Contribution: This function enables seamless switching between camera modes. When switching to first-person mode, it ensures the camera is positioned at the correct terrain height. When switching to orbit mode, it resets the orbit parameters to provide a good overview of the scene. This maintains a consistent and intuitive user experience.
void viewCameraSetMode(ViewCamera* cam, CameraMode newMode) {
    if (!cam) return; // Early exit if camera is null
    if (cam->mode == newMode) return; // Early exit if already in the requested mode
    cam->mode = newMode; // Set the new camera mode
    if (newMode == CAMERA_MODE_FIRST_PERSON) {
        float h = landscapeGetHeight(landscape, cam->fpPosition[0], cam->fpPosition[2]); // Get terrain height at current position
        cam->fpPosition[1] = h + CAM_EYE_LVL; // Set camera height to terrain height plus eye level
    } else if (newMode == CAMERA_MODE_FREE_ORBIT) {
        cam->orbitDistance = LANDSCAPE_SCALE * 0.7f; // Reset orbit distance to default
        cam->orbitYaw = 60.0f; // Reset orbit yaw to default
        cam->orbitPitch = 20.0f; // Reset orbit pitch to default
    }
    viewCameraUpdateVectors(cam); // Update camera vectors for the new mode
}

// viewCameraUpdate: Performs per-frame updates including terrain following and boundary checks.
// Contribution: This function is called every frame to ensure the camera state remains consistent. In first-person mode, it continuously updates the camera height to follow the terrain surface. It also performs boundary checks to prevent the camera from leaving the valid area. This maintains realism and prevents camera-related glitches.
void viewCameraUpdate(ViewCamera* cam, float deltaTime) {
    if (!cam) return; // Early exit if camera is null
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        float h = landscapeGetHeight(landscape, cam->fpPosition[0], cam->fpPosition[2]); // Get current terrain height
        cam->fpPosition[1] = h + CAM_EYE_LVL; // Update camera height to follow terrain
    }
    clampCameraToBounds(cam); // Ensure camera stays within valid boundaries
}

// clampCameraToBounds: Ensures camera stays within valid terrain boundaries.
// Contribution: This function prevents the camera from moving outside the valid terrain area, which could cause rendering issues or disorient the user. It clamps the camera position to the landscape boundaries, ensuring a consistent and bounded exploration experience.
static void clampCameraToBounds(ViewCamera* cam) {
    float half = LANDSCAPE_SCALE * 0.5f; // Calculate terrain boundary
    if (cam->mode == CAMERA_MODE_FIRST_PERSON) {
        if (cam->fpPosition[0] < -half) cam->fpPosition[0] = -half; // Clamp X position to minimum boundary
        if (cam->fpPosition[0] > half) cam->fpPosition[0] = half; // Clamp X position to maximum boundary
        if (cam->fpPosition[2] < -half) cam->fpPosition[2] = -half; // Clamp Z position to minimum boundary
        if (cam->fpPosition[2] > half) cam->fpPosition[2] = half; // Clamp Z position to maximum boundary
    }
}

// viewCameraDestroy: Frees camera memory and cleans up resources.
// Contribution: This function ensures proper cleanup of camera resources when the camera is no longer needed. It prevents memory leaks and allows the system to properly deallocate the camera structure, maintaining system stability and resource management.
void viewCameraDestroy(ViewCamera* cam) {
    if (cam) free(cam); // Free camera memory if it exists
}