/*
 * Main Application File - Mountain Valley Simulator
 *
 * This is the primary application file for the Mountain Valley Simulator, a comprehensive
 * 3D graphics application that creates an interactive, procedurally generated landscape
 * inspired by Boulder, Colorado. The application integrates multiple systems including
 * terrain generation, weather effects, camera controls, lighting, and user interaction.
 *
 * Key Systems Integrated:
 * - Landscape Generation: Procedural terrain with heightmap-based elevation
 * - Weather System: Dynamic snow/rain particles with physics simulation
 * - Sky System: Animated sky with time-of-day color transitions
 * - Cloud System: Volumetric atmospheric clouds that respond to sun position
 * - Camera System: Dual-mode camera (free orbit and first-person)
 * - Lighting System: Dynamic sun positioning with day/night cycles
 * - Object System: Procedural placement of trees, rocks, and vegetation
 * - Grass System: Animated grass blades with wind effects
 * - Water System: Animated water surface with reflections
 * - Sound System: Ambient audio for immersive experience
 * - User Interface: Real-time controls and status display
 *
 * Technical Features:
 * - OpenGL-based rendering with modern shader pipeline
 * - Real-time particle physics for weather effects
 * - Procedural content generation for infinite variety
 * - Multi-threaded systems for performance optimization
 * - Adaptive LOD (Level of Detail) for terrain rendering
 * - Dynamic lighting and shadow systems
 * - Interactive camera controls with collision detection
 *
 * User Controls:
 * - Arrow Keys: Camera movement and orbit controls
 * - WASD: First-person camera movement
 * - Mouse: Camera rotation and zoom
 * - Q: Toggle wireframe mode
 * - E: Toggle weather type (Fall/Winter)
 * - T: Toggle time animation
 * - K/L: Adjust time speed
 * - B: Toggle fog effects
 * - N: Toggle snow/rain particles
 * - M: Toggle ambient sound
 * - R: Reset camera to default position
 * - 1/2: Switch between camera modes
 * - Z: Zoom in/out (orbit mode)
 *
 * Architecture:
 * - Modular design with separate systems for each major feature
 * - Event-driven input handling with GLUT
 * - Real-time rendering loop with delta time management
 * - Resource management with proper cleanup
 * - Error handling and graceful degradation
 */

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

// Global system instances and resources
static AtmosphericCloudSystem* cloudSystem = NULL; // Volumetric cloud rendering system
static SkySystem skySystemInstance; // Sky dome and atmospheric effects

// Texture resources for various surface materials
GLuint rockTexture = 0;    // Rock surface texture for terrain
GLuint sandTexture = 0;    // Sand surface texture for beaches
GLuint boulderTexture = 0; // Boulder surface texture for large rocks
GLuint barkTexture = 0;    // Tree bark texture for trunks and branches
GLuint leafTexture = 0;    // Leaf texture for tree foliage

// Camera orbit parameters (legacy support)
static int th = 45;        // Theta angle for orbit camera (horizontal rotation)
static int ph = 10;        // Phi angle for orbit camera (vertical rotation)
static float dim = 100;    // Distance from camera to target point
static int fov = 1;        // Field of view toggle (perspective/orthographic)

// Lighting and environmental parameters
static float lightHeight = 250.0f; // Height of the sun light source
static float dayTime = 0.0f;       // Current time of day (0-24 hours)
static float windStrength = 0.0f;  // Current wind strength for animations

// Water system parameters
#define WATER_LEVEL -4.0f  // Height of water surface relative to terrain
static float waterTime = 0.0f;     // Animation time for water effects
static int animateWater = 1;       // Toggle for water animation

// Rendering mode toggles
static int wireframe = 0;  // Wireframe rendering mode
static int showAxes = 0;   // Display coordinate axes

// Landscape system instance
Landscape* landscape = NULL; // Main terrain and elevation data

// Time management for smooth animations
static float lastTime = 0;     // Previous frame time
static float deltaTime = 0;    // Time between frames

// Atmospheric effects
int fogEnabled = 0; // Fog effect toggle

// Animation control
static int animateTime = 1;        // Time progression toggle
static float timeSpeed = 1.0f;     // Time progression speed multiplier

// Camera system
static ViewCamera* camera = NULL;  // Main camera instance
float asp;                         // Aspect ratio (width/height)
static int lastX = 0, lastY = 0;   // Previous mouse position
static int mouseButtons = 0;       // Current mouse button state

// Tree animation parameters
float treeSwayAngle = 0.0f; // Current tree sway angle for wind effects

// Weather system parameters
static int snowOn = 0;      // Snow particle system toggle
static int weatherType = 0; // Weather type (0 = Fall, 1 = Winter)

// Audio system
static int ambientSoundOn = 1; // Ambient sound toggle

// Camera distance constraints
#define DIM_MIN 30.0f  // Minimum camera distance
#define DIM_MAX 200.0f // Maximum camera distance

/*
 * Camera Distance Synchronization and Clamping
 *
 * This function ensures camera distance parameters are within valid bounds
 * and synchronizes the legacy orbit parameters with the modern camera system.
 * It prevents the camera from getting too close or too far from the scene.
 */
void clampAndSyncDim() {
    // Clamp distance to valid range
    if (dim < DIM_MIN) dim = DIM_MIN;
    if (dim > DIM_MAX) dim = DIM_MAX;
    
    // Synchronize with modern camera system if available
    if (camera) {
        // Clamp camera orbit distance
        if (camera->orbitDistance < DIM_MIN) camera->orbitDistance = DIM_MIN;
        if (camera->orbitDistance > DIM_MAX) camera->orbitDistance = DIM_MAX;
        
        // Average the distances for smooth transitions
        dim = camera->orbitDistance = (dim + camera->orbitDistance) * 0.5f;
    }
}

// Function declarations for GLUT callbacks
void reshape(int width, int height);
void display();
void special(int key, int x, int y);
void keyboard(unsigned char key, int x, int y);
void idle();

/*
 * Window Reshape Handler
 *
 * Handles window resize events by updating the viewport, aspect ratio,
 * and camera projection parameters. Ensures proper rendering regardless
 * of window size changes.
 *
 * Parameters:
 * - width: New window width in pixels
 * - height: New window height in pixels
 */
void reshape(int width, int height) {
    // Calculate aspect ratio (width/height)
    asp = (height>0) ? (double)width/height : 1;
    
    // Set viewport with resolution scaling
    glViewport(0,0, RES*width,RES*height);
    
    // Update camera projection if using modern camera system
    if (camera) {
        viewCameraSetProjection(camera, 55.0f, asp, dim/4, dim*4);
    } else {
        // Fallback to legacy projection system
        Project(fov?55:0, asp, dim);
    }
}

/*
 * Delta Time Calculation
 *
 * Calculates the time elapsed between frames for smooth animations.
 * This ensures consistent animation speed regardless of frame rate.
 */
void updateDeltaTime() {
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // Convert to seconds
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
}

/*
 * Smoothstep Interpolation Function
 *
 * Provides smooth interpolation between two values using cubic Hermite interpolation.
 * This creates natural-looking transitions for color blending and animations.
 *
 * Parameters:
 * - edge0: Start value
 * - edge1: End value
 * - x: Current value to interpolate
 *
 * Returns: Smoothly interpolated value between edge0 and edge1
 */
float smoothstep(float edge0, float edge1, float x) {
    // Clamp x to [0,1] range
    float t = (x - edge0) / (edge1 - edge0);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    
    // Apply smoothstep formula: t²(3-2t)
    return t * t * (3.0f - 2.0f * t);
}

/*
 * Sky Color Calculation
 *
 * Generates realistic sky colors based on time of day, creating smooth
 * transitions between dawn, day, dusk, and night. The function uses
 * multiple color keyframes and smoothstep interpolation for natural
 * atmospheric effects.
 *
 * Parameters:
 * - time: Current time of day (0-24 hours)
 * - color: Output array for RGB sky color
 */
void getSkyColor(float time, float* color) {
    float t = time / 24.0f;  // Normalize time to [0,1] range
    
    // Define color keyframes for different times of day
    const int NUM_COLORS = 6;
    float timePoints[6] = {0.0f, 0.25f, 0.4f, 0.6f, 0.75f, 1.0f};  // Time points (0-1)
    float colors[6][3] = {
        {0.02f, 0.02f, 0.1f},   // Deep night blue
        {0.7f, 0.4f, 0.4f},     // Dawn orange
        {0.4f, 0.7f, 1.0f},     // Day blue
        {0.4f, 0.7f, 1.0f},     // Day blue (continued)
        {0.7f, 0.4f, 0.4f},     // Dusk orange
        {0.02f, 0.02f, 0.1f}    // Deep night blue
    };
    
    // Find the appropriate color segment for current time
    int i;
    for(i = 0; i < NUM_COLORS-1; i++) {
        if(t >= timePoints[i] && t <= timePoints[i+1]) break;
    }
    
    // Calculate interpolation factor within the segment
    float segmentPos = (t - timePoints[i]) / (timePoints[i+1] - timePoints[i]);
    float blend = smoothstep(0.0f, 1.0f, segmentPos);
    
    // Calculate sun height for additional atmospheric effects
    float sunHeight = sin(t * 2.0f * M_PI);
    
    // Interpolate between color keyframes
    for(int j = 0; j < 3; j++) {
        color[j] = colors[i][j] * (1.0f - blend) + colors[i+1][j] * blend;
    }
    
    // Add blue tint when sun is above horizon (atmospheric scattering)
    if(sunHeight > 0) {
        color[2] = fmin(1.0f, color[2] + sunHeight * 0.04f);
    }
    
    // Apply night blending for smooth transitions at day boundaries
    if(t < 0.1f || t > 0.9f) {
        float nightBlend = (t < 0.1f) ? (t / 0.1f) : ((1.0f - t) / 0.1f);
        nightBlend = smoothstep(0.0f, 1.0f, nightBlend);
        float nightColor[3] = {0.02f, 0.02f, 0.1f}; // Deep night color
        for(int j = 0; j < 3; j++) {
            color[j] = color[j] * nightBlend + nightColor[j] * (1.0f - nightBlend);
        }
    }
}

/*
 * Tree Animation Update
 *
 * Updates tree sway animation based on wind strength and time.
 * Creates realistic wind effects that cause trees to sway naturally.
 */
void updateTreeAnimation() {
    static float windTime = 0.0f;
    windTime += deltaTime;
    
    // Calculate wind strength using sine wave (0.0 to 1.0 range)
    windStrength = sin(windTime * 0.5f) * 0.5f + 0.5f;
    
    // Calculate tree sway angle (oscillating between -8 and +8 degrees)
    treeSwayAngle = sin(windTime) * 8.0f;
}

/*
 * OpenGL Lighting Setup
 *
 * Configures the OpenGL lighting system with ambient and diffuse lighting,
 * material properties, and light positioning. Sets up the foundation for
 * realistic lighting calculations throughout the scene.
 */
void setupLighting() {
    glEnable(GL_LIGHTING);        // Enable lighting calculations
    glEnable(GL_LIGHT0);          // Enable light source 0
    glEnable(GL_COLOR_MATERIAL);  // Enable material color tracking
    glEnable(GL_NORMALIZE);       // Normalize normals for proper lighting
    
    // Set light position (directional light from above)
    float position[] = {1.0f, 2.0f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    
    // Set material specular properties
    float mSpecular[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 30.0f);
    
    // Enable automatic color material tracking
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

/*
 * Fog System Update
 *
 * Updates the OpenGL fog system based on time of day, creating realistic
 * atmospheric effects that vary with sun position. Fog density and color
 * change dynamically to simulate different weather conditions.
 *
 * Parameters:
 * - dayTime: Current time of day for sun position calculation
 */
void updateFog(float dayTime) {
    float timeNormalized = dayTime / 24.0f;
    float sunAngle = (timeNormalized - 0.25f) * 2 * M_PI;
    float sunHeight = sin(sunAngle);
    
    // Base fog density
    float baseDensity = 0.008f;
    
    // Fog color (light gray during day, darker at night)
    float fogColor[4] = {0.95f, 0.95f, 0.95f, 1.0f};
    
    // Darken fog color when sun is below horizon
    if (sunHeight <= 0) {
        fogColor[0] = fogColor[1] = fogColor[2] = 0.7f;
    }
    
    // Apply fog settings if enabled
    if (fogEnabled) {
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_EXP2);           // Exponential squared fog
        glFogf(GL_FOG_DENSITY, baseDensity);    // Fog density
        glFogfv(GL_FOG_COLOR, fogColor);        // Fog color
        
        // Adjust fog range based on sun height
        float fogStart = sunHeight > 0 ? dim * 0.1f : dim * 0.05f;
        float fogEnd = sunHeight > 0 ? dim * 0.8f : dim * 0.4f;
        glFogf(GL_FOG_START, fogStart);
        glFogf(GL_FOG_END, fogEnd);
        glHint(GL_FOG_HINT, GL_NICEST);
    } else {
        glDisable(GL_FOG);
    }
}

/*
 * OpenGL State Initialization
 *
 * Sets up fundamental OpenGL rendering states including depth testing,
 * face culling, blending, and polygon offset. These settings ensure
 * proper rendering order and visual quality.
 */
void initGL() {
    glEnable(GL_DEPTH_TEST);                    // Enable depth testing
    glDepthFunc(GL_LEQUAL);                     // Use less-equal depth function
    glEnable(GL_CULL_FACE);                     // Enable face culling
    glCullFace(GL_BACK);                        // Cull back faces
    glFrontFace(GL_CCW);                        // Counter-clockwise front faces
    glEnable(GL_BLEND);                         // Enable alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard alpha blending
    glPolygonOffset(1.0f, 1.0f);               // Polygon offset for z-fighting prevention
}

/*
 * Main Rendering Function
 *
 * The primary rendering function that draws the entire scene. It handles
 * all rendering passes including sky, terrain, objects, water, and UI
 * elements. The function manages the rendering order and state changes
 * for optimal visual quality and performance.
 */
void display() {
    // Calculate sky color based on time of day
    float skyColor[3];
    getSkyColor(dayTime, skyColor);
    
    // Set clear color to sky color for seamless sky integration
    glClearColor(skyColor[0], skyColor[1], skyColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Reset modelview matrix
    glLoadIdentity();
    
    // Set up camera view matrix
    gluLookAt(camera->fpPosition[0], camera->fpPosition[1], camera->fpPosition[2],
              camera->lookAt[0], camera->lookAt[1], camera->lookAt[2],
              camera->upVec[0], camera->upVec[1], camera->upVec[2]);
    
    // Render sky system (sky dome and atmospheric effects)
    skySystemRender(&skySystemInstance, dayTime);
    
    // Update and apply fog effects
    updateFog(dayTime);
    
    // Render volumetric clouds (with depth mask disabled for transparency)
    if (cloudSystem) {
        glDepthMask(GL_FALSE);
        atmosphericCloudSystemRender(cloudSystem);
        glDepthMask(GL_TRUE);
    }
    
    // Render main terrain landscape
    landscapeRender(landscape, weatherType);
    
    // Calculate sun position and lighting parameters
    float timeNormalized = dayTime / 24.0f;
    float sunAngle = (timeNormalized - 0.25f) * 2 * M_PI;
    float sunHeight = sin(sunAngle);
    float sunX = 500 * cos(sunAngle);
    float sunY = lightHeight * sunHeight;
    float sunZ = 0;
    
    // Calculate sun direction vector
    float sunDir[3];
    float len = sqrtf(sunX*sunX + sunY*sunY + sunZ*sunZ);
    sunDir[0] = sunX / len;
    sunDir[1] = sunY / len;
    sunDir[2] = sunZ / len;
    
    // Calculate ambient lighting based on sun height
    float ambient[3];
    if (sunHeight > 0) {
        ambient[0] = ambient[1] = ambient[2] = 0.15f + sunHeight * 0.15f;
    } else {
        ambient[0] = 0.02f;
        ambient[1] = 0.02f;
        ambient[2] = 0.04f;
    }
    
    // Render animated grass system
    grassSystemRender(dayTime, windStrength, sunDir, ambient);
    
    // Render landscape objects (trees, rocks, etc.)
    renderLandscapeObjects(landscape);
    
    // Render water surface with transparency
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    landscapeRenderWater(WATER_LEVEL, landscape, dayTime);
    glDepthMask(GL_TRUE);
    
    // Render coordinate axes if enabled
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
    
    // Render UI overlay with status information
    glDisable(GL_DEPTH_TEST);
    glColor3f(1,1,1);
    glWindowPos2i(5, glutGet(GLUT_WINDOW_HEIGHT) - 20);
    Print("Time: %02d:%02d  Weather: %s", 
          (int)dayTime, (int)((dayTime-(int)dayTime)*60),
          weatherType == 1 ? "Winter" : "Fall");
    
    // Render detailed status information
    int y = 5;
    glWindowPos2i(5, y);
    Print("Angle=%d,%d  Dim=%.1f  View=%s   |   Wireframe=%d   |   Axes=%d   |   TimeAnim: %s  Speed: %.1fx   |   Fog: %s  Snow: %s  |   Sound: %s",
        th, ph, dim, camera->mode == CAMERA_MODE_FREE_ORBIT ? "Free Orbit" : "First Person",
        wireframe,
        showAxes,
        animateTime ? "On" : "Off", timeSpeed,
        fogEnabled ? "On" : "Off",
        snowOn ? "On" : "Off",
        ambientSoundOn ? "On" : "Off");
    glEnable(GL_DEPTH_TEST);
    
    // Render weather particles if enabled
    if (snowOn) {
        particleSystemRender();
    }
    
    // Swap buffers for double buffering
    glutSwapBuffers();
}

/*
 * Mouse Button Event Handler
 *
 * Handles mouse button press and release events for camera controls.
 * Tracks button states for continuous motion handling.
 *
 * Parameters:
 * - button: GLUT button identifier (GLUT_LEFT_BUTTON, etc.)
 * - state: GLUT_DOWN or GLUT_UP
 * - x, y: Mouse coordinates in window space
 */
void mouse(int button, int state, int x, int y) {
    lastX = x;
    lastY = y;
    
    // Update button state bitmask
    if (state == GLUT_DOWN) {
        mouseButtons |= 1<<button;
    } else {
        mouseButtons &= ~(1<<button);
    }
    
    glutPostRedisplay();
}

/*
 * Mouse Motion Event Handler
 *
 * Handles mouse movement for camera rotation in first-person mode.
 * Provides smooth camera control based on mouse movement.
 *
 * Parameters:
 * - x, y: Current mouse coordinates in window space
 */
void mouseMotion(int x, int y) {
    int dx = x - lastX;
    int dy = y - lastY;
    
    // Rotate camera in first-person mode when left button is held
    if (camera->mode != CAMERA_MODE_FREE_ORBIT && (mouseButtons & 1)) {
        viewCameraRotate(camera, dx * 0.5f, -dy * 0.5f);
    }
    
    lastX = x;
    lastY = y;
    glutPostRedisplay();
}

/*
 * Special Key Event Handler
 *
 * Handles special keyboard keys (arrow keys, function keys) for camera
 * controls and navigation. Supports both orbit and first-person camera modes.
 *
 * Parameters:
 * - key: GLUT special key identifier
 * - x, y: Mouse coordinates (unused)
 */
void special(int key, int x, int y) {
    float deltaTime = 0.016f; // Fixed time step for consistent movement
    
    // Handle orbit camera mode controls
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
        
        // Normalize angles to [0, 360) range
        th %= 360;
        ph %= 360;
        
        // Update camera vectors and projection
        viewCameraUpdateVectors(camera);  
        Project(fov?55:0, asp, dim);
    }
    // Handle first-person camera mode controls
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

/*
 * Keyboard Event Handler
 *
 * Handles regular keyboard input for various application controls including
 * rendering modes, camera presets, animation controls, and system toggles.
 * Provides comprehensive user interface for scene manipulation.
 *
 * Parameters:
 * - key: ASCII character code
 * - x, y: Mouse coordinates (unused)
 */
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 27: // ESC key - exit application
            exit(0);
            break;
            
        case 'q':
        case 'Q': // Toggle wireframe rendering mode
            wireframe = !wireframe;
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
            break;
            
        case 'e':
        case 'E': // Toggle weather type (Fall/Winter)
            weatherType = !weatherType;
            break;
            
        case 'w':
        case 'W': // Move forward (first-person mode)
            if (camera->mode == CAMERA_MODE_FIRST_PERSON) {
                viewCameraMove(camera, CAMERA_MOVE_FORWARD, 0.016f);
            }
            break;
            
        case 's':
        case 'S': // Move backward (first-person mode)
            if (camera->mode == CAMERA_MODE_FIRST_PERSON) {
                viewCameraMove(camera, CAMERA_MOVE_BACKWARD, 0.016f);
            }
            break;
            
        case 'a':
        case 'A': // Move left (first-person mode) or toggle axes (orbit mode)
            if (camera->mode == CAMERA_MODE_FIRST_PERSON) {
                viewCameraMove(camera, CAMERA_MOVE_LEFT, 0.016f);
            } else {
                showAxes = !showAxes;
            }
            break;
            
        case 'd':
        case 'D': // Move right (first-person mode)
            if (camera->mode == CAMERA_MODE_FIRST_PERSON) {
                viewCameraMove(camera, CAMERA_MOVE_RIGHT, 0.016f);
            }
            break;
            
        case 'r': // Reset camera to default position
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
            
        case '1': // Switch to first-person camera mode
            {
                camera->fpYaw = 45.0f;
                camera->fpPitch = 10.0f;
                camera->fpPosition[0] = 0.0f;
                camera->fpPosition[2] = 0.0f;
                float groundHeight = landscapeGetHeight(landscape, camera->fpPosition[0], camera->fpPosition[2]);
                camera->fpPosition[1] = groundHeight + 2.0f;
                viewCameraSetMode(camera, CAMERA_MODE_FIRST_PERSON);
                viewCameraUpdateVectors(camera);
                viewCameraSetProjection(camera, 55.0f, asp, dim/4, dim*4);
            }
            break;
            
        case '2': // Switch to orbit camera mode
            {
                camera->orbitYaw = 45.0f;
                camera->orbitPitch = 10.0f;
                camera->orbitDistance = 70.0f;
                th = 45; ph = 10; dim = 70.0f;
                clampAndSyncDim();
                viewCameraSetMode(camera, CAMERA_MODE_FREE_ORBIT);
                viewCameraUpdateVectors(camera);
            }
            break;
            
        case 't': // Toggle time animation
            animateTime = !animateTime;
            glutIdleFunc(animateTime ? idle : NULL);
            break;
            
        case 'k': // Decrease time speed
            timeSpeed = fmax(0.1f, timeSpeed - 0.1f);
            break;
            
        case 'l': // Increase time speed
            timeSpeed = fmin(5.0f, timeSpeed + 0.1f);
            break;
            
        case 'b': // Toggle fog effects
            fogEnabled = !fogEnabled;
            break;
            
        case 'z': // Zoom in (orbit mode)
            if (camera->mode == CAMERA_MODE_FREE_ORBIT) {
                dim -= 5.0f;
                camera->orbitDistance -= 5.0f;
                clampAndSyncDim();
                viewCameraUpdateVectors(camera);
                Project(fov?55:0, asp, dim);
            }
            break;
            
        case 'Z': // Zoom out (orbit mode)
            if (camera->mode == CAMERA_MODE_FREE_ORBIT) {
                dim += 5.0f;
                camera->orbitDistance += 5.0f;
                clampAndSyncDim();
                viewCameraUpdateVectors(camera);
                Project(fov?55:0, asp, dim);
            }
            break;
            
        case 'n': // Toggle snow/rain particles
            snowOn = !snowOn;
            particleSystemSetEnabled(snowOn);
            break;
            
        case 'm': // Toggle ambient sound
            ambientSoundOn = !ambientSoundOn;
            if (ambientSoundOn) {
                soundPlay();
            } else {
                soundStop();
            }
            break;
    }
    
    glutPostRedisplay();
}

/*
 * Idle Function
 *
 * Called continuously when the application is idle. Handles all real-time
 * updates including time progression, camera updates, animations, and
 * particle system updates. This is the main update loop for the application.
 */
void idle() {
    // Update time if animation is enabled
    if (animateTime) {
        dayTime += deltaTime * timeSpeed;
        if (dayTime >= 24.0f) dayTime = 0.0f; // Wrap around to start of day
    }
    
    // Update delta time for smooth animations
    updateDeltaTime();
    
    // Update camera system
    viewCameraUpdate(camera, deltaTime);
    
    // Update tree animations
    updateTreeAnimation();
    
    // Update water animation if enabled
    if (animateWater) {
        waterTime += deltaTime;
    }
    
    // Update particle system if enabled
    if (snowOn) {
        particleSystemUpdate(deltaTime);
    }
    
    // Request redisplay for continuous rendering
    glutPostRedisplay();
}

/*
 * Main Application Entry Point
 *
 * Initializes the entire application including OpenGL, GLUT, all subsystems,
 * and sets up the main rendering loop. Handles resource allocation, error
 * checking, and proper cleanup on exit.
 *
 * Parameters:
 * - argc: Number of command line arguments
 * - argv: Array of command line argument strings
 *
 * Returns: 0 on successful execution, 1 on error
 */
int main(int argc, char* argv[]) {
    // Initialize GLUT
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);
    
    // Get screen dimensions and create fullscreen window
    int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
    int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Project: Sanjay Baskaran");
    
    // Initialize GLEW for modern OpenGL extensions
#ifdef USEGLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW initialization failed: %s\n", glewGetErrorString(err));
        return 1;
    }
#endif
    
    // Initialize landscape system
    landscape = landscapeCreate();
    if (!landscape) {
        fprintf(stderr, "Failed to create landscape\n");
        return 1;
    }
    
    // Initialize grass system with 500,000 grass blades
    grassSystemInit(landscape, LANDSCAPE_SCALE, 500000);
    
    // Upload terrain heightmap to particle system for collision detection
    particleSystemUploadHeightmap(landscape->elevationData);
    
    // Initialize landscape objects (trees, rocks, etc.)
    initLandscapeObjects(landscape);
    
    // Initialize boulder system
    initBoulders(landscape);
    
    // Create and configure camera system
    camera = viewCameraCreate();
    if (!camera) {
        fprintf(stderr, "Failed to create camera\n");
        return 1;
    }
 
    // Set initial camera parameters
    camera->orbitYaw = 45.0f;
    camera->orbitPitch = 10.0f;
    camera->orbitDistance = 70.0f;
    th = 45; ph = 10; dim = 70.0f;
    viewCameraSetMode(camera, CAMERA_MODE_FREE_ORBIT);
    viewCameraUpdateVectors(camera);
    
    // Initialize sky and cloud systems
    skySystemInitialize(&skySystemInstance);
    cloudSystem = atmosphericCloudSystemCreate(LANDSCAPE_SCALE * 0.4f);
    if (!cloudSystem) {
        fprintf(stderr, "Failed to create cloud system\n");
        return 1;
    }

    // Load texture resources
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
    
    // Initialize fractal tree and boulder shader systems
    fractalTreeInit();
    boulderShaderInit();
    
    // Set up OpenGL lighting
    setupLighting();
    
    // Initialize time tracking
    lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    
    // Initialize particle system for weather effects
    particleSystemInit(2000.0f, 20000.0f);
    
    // Initialize and start ambient sound system
    if (!soundInit("sounds/forest-ambience.mp3")) {
        fprintf(stderr, "Failed to initialize audio system.\n");
    } else {
        soundPlay();
    }
    
    // Set up GLUT callback functions
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(special);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(NULL);
    
    // Enter main rendering loop
    glutMainLoop();
    
    // Cleanup resources (this code is reached when glutMainLoop exits)
    landscapeDestroy(landscape);
    freeBoulders();
    freeLandscapeObjects();
    atmosphericCloudSystemDestroy(cloudSystem);
    viewCameraDestroy(camera);
    particleSystemCleanup();
    grassSystemCleanup();
    soundCleanup();
    
    return 0;
}