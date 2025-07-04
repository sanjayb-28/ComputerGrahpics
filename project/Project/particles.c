/*
 * particles.c - GPU-Based Particle System for Dynamic Weather in Boulder Scene
 *
 * This file implements a high-performance particle system for simulating dynamic weather effects (such as snow and rain)
 * in the interactive 3D Boulder-inspired scene. The system is designed to handle tens of thousands of particles efficiently
 * by leveraging modern OpenGL features, including transform feedback and custom GLSL shaders.
 *
 * Key Features and Design:
 * - Uses transform feedback to update particle positions, velocities, and states entirely on the GPU, minimizing CPU-GPU traffic.
 * - Employs two sets of Vertex Array Objects (VAOs) and Vertex Buffer Objects (VBOs) to "ping-pong" particle data between update and render passes.
 * - Particles are initialized with randomized positions, velocities, and lifetimes to create natural, varied weather effects.
 * - The update pass is performed by a vertex shader (particle_update.vert) that simulates gravity, wind, and collision with the landscape.
 * - The render pass uses a separate shader (particle_render.vert/frag) to draw each particle as a point sprite, with blending for soft, semi-transparent effects.
 * - The system is modular: it can be initialized, updated, rendered, and cleaned up independently of other scene systems.
 *
 * This file is ideal for demoing advanced real-time graphics techniques, including GPU-based simulation, OpenGL buffer management,
 * and modular system design. The comments throughout the file are written as a story, explaining the intent, logic, and syntax
 * behind every operation, so you can confidently answer questions about the code during a demo or technical review.
 *
 * Particle system and transform feedback concepts were learned from:
 *   - https://learnopengl.com/In-Practice/2D-Game/Particles
 *   - https://www.ogldev.org/www/tutorial28/tutorial28.html
 *   - https://open.gl/feedback
 *
 * All code is my own original work, except the section marked as AI
 */

/* --- Concept: Transform Feedback, Ping-Pong Buffers, and Point Sprites ---
 *
 * Transform Feedback:
 *   Transform feedback is a powerful OpenGL feature that allows the GPU to capture the output of the vertex (or geometry) shader
 *   and write it directly into buffer objects, bypassing the CPU entirely. This is essential for real-time particle systems,
 *   as it enables the simulation of thousands of particles per frame without the bottleneck of CPU-GPU data transfer.
 *   In this system, transform feedback is used to update each particle's position, velocity, and state in a single pass on the GPU.
 *
 * Ping-Pong Buffers:
 *   To avoid read/write conflicts when updating particle data, we use two sets of buffers (VBOs and VAOs) in a technique called
 *   "ping-pong buffering." In each frame, one buffer is used as the source (read) and the other as the destination (write).
 *   After the update, the roles are swapped for the next frame. This ensures that the GPU never reads and writes to the same buffer
 *   simultaneously, which is critical for correctness and performance.
 *
 * Point Sprites:
 *   Point sprites are a rendering technique where each particle is drawn as a textured square (billboard) that always faces the camera.
 *   This is much more efficient than rendering a full quad for each particle, and is ideal for effects like snow and rain.
 *   The render shader can control the color, size, and transparency of each point sprite, allowing for soft, natural-looking particles.
 *
 * Together, these techniques enable the creation of visually rich, high-performance weather effects that scale to thousands of particles
 * in real time, all while keeping the code modular and maintainable.
 */

#include "CSCIx229.h"      // Custom OpenGL and utility header for this project
#include "particles.h"     // Header for particle system types and function prototypes
#include "shaders.h"       // Header for shader loading utilities
#include "landscape.h"     // Header for landscape constants and types

// --- Platform-specific macros for VAO and transform feedback support ---
// These macros abstract away the differences between Apple and non-Apple OpenGL implementations.
// On Apple platforms, VAO and transform feedback functions have different names, so we define macros
// to map to the correct function names depending on the platform. This ensures cross-platform compatibility.
#ifdef __APPLE__
    #define VAO_BIND(vao) glBindVertexArrayAPPLE(vao) // Bind a vertex array object (Apple-specific)
    #define VAO_UNBIND() glBindVertexArrayAPPLE(0)    // Unbind any vertex array object (Apple-specific)
    #define VAO_GEN(count, arrays) glGenVertexArraysAPPLE(count, arrays) // Generate VAOs (Apple-specific)
    #define VAO_DELETE(count, arrays) glDeleteVertexArraysAPPLE(count, arrays) // Delete VAOs (Apple-specific)
    #define POINT_SPRITE_ON() glEnable(GL_POINT_SPRITE) // Enable point sprite rendering (Apple-specific)
    #define POINT_SPRITE_OFF() glDisable(GL_POINT_SPRITE) // Disable point sprite rendering (Apple-specific)
    #define TF_BEGIN() glBeginTransformFeedbackEXT(GL_POINTS) // Begin transform feedback (Apple-specific)
    #define TF_END() glEndTransformFeedbackEXT() // End transform feedback (Apple-specific)
    #define RASTER_DISCARD_ON() glEnable(GL_RASTERIZER_DISCARD_EXT) // Enable rasterizer discard (Apple-specific)
    #define RASTER_DISCARD_OFF() glDisable(GL_RASTERIZER_DISCARD_EXT) // Disable rasterizer discard (Apple-specific)
    #define TF_BIND_BUFFER(buffer) glBindBufferBaseEXT(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, buffer) // Bind transform feedback buffer (Apple-specific)
    #define TF_UNBIND_BUFFER() glBindBufferBaseEXT(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, 0) // Unbind transform feedback buffer (Apple-specific)
    #define TF_SETUP(shader, count, varyings) glTransformFeedbackVaryingsEXT(shader, count, varyings, GL_INTERLEAVED_ATTRIBS_EXT) // Set up transform feedback varyings (Apple-specific)
#else
    #define VAO_BIND(vao) glBindVertexArray(vao) // Bind a vertex array object (standard OpenGL)
    #define VAO_UNBIND() glBindVertexArray(0)    // Unbind any vertex array object (standard OpenGL)
    #define VAO_GEN(count, arrays) glGenVertexArrays(count, arrays) // Generate VAOs (standard OpenGL)
    #define VAO_DELETE(count, arrays) glDeleteVertexArrays(count, arrays) // Delete VAOs (standard OpenGL)
    #define POINT_SPRITE_ON()                   // No-op on non-Apple platforms (point sprites handled differently)
    #define POINT_SPRITE_OFF()                  // No-op on non-Apple platforms
    #define TF_BEGIN() glBeginTransformFeedback(GL_POINTS) // Begin transform feedback (standard OpenGL)
    #define TF_END() glEndTransformFeedback() // End transform feedback (standard OpenGL)
    #define RASTER_DISCARD_ON() glEnable(GL_RASTERIZER_DISCARD) // Enable rasterizer discard (standard OpenGL)
    #define RASTER_DISCARD_OFF() glDisable(GL_RASTERIZER_DISCARD) // Disable rasterizer discard (standard OpenGL)
    #define TF_BIND_BUFFER(buffer) glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, buffer) // Bind transform feedback buffer (standard OpenGL)
    #define TF_UNBIND_BUFFER() glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0) // Unbind transform feedback buffer (standard OpenGL)
    #define TF_SETUP(shader, count, varyings) glTransformFeedbackVaryings(shader, count, varyings, GL_INTERLEAVED_ATTRIBS) // Set up transform feedback varyings (standard OpenGL)
#endif

// --- Particle system state variables ---
// These variables hold the OpenGL handles and simulation state for the particle system.
static GLuint particleVBOs[2] = {0, 0}; // Two Vertex Buffer Objects (VBOs) for ping-ponging particle data between update and render passes
static GLuint particleVAOs[2] = {0, 0}; // Two Vertex Array Objects (VAOs) for binding the correct VBO and attribute layout
static GLuint updateShader = 0;         // Handle for the shader program used to update particle state (vertex shader with transform feedback)
static GLuint renderShader = 0;         // Handle for the shader program used to render particles (vertex + fragment shaders)
static int curSrc = 0;                  // Index of the current source buffer (0 or 1); alternates each frame for ping-pong buffering
static GLuint heightmapTex = 0;         // Handle for the texture containing the landscape heightmap, used for particle-ground collision

#define NUM_PARTICLES 20000             // Number of particles in the system (tunable for performance/quality)
static float cloudHeight = 128.0f;      // Default height above the terrain where cloud-originated particles spawn
static float terrainMinX = -LANDSCAPE_SCALE * 0.5f; // Minimum X coordinate of the landscape (centered at origin)
static float terrainMaxX = LANDSCAPE_SCALE * 0.5f;  // Maximum X coordinate of the landscape
static float terrainMinZ = -LANDSCAPE_SCALE * 0.5f; // Minimum Z coordinate of the landscape
static float terrainMaxZ = LANDSCAPE_SCALE * 0.5f;  // Maximum Z coordinate of the landscape

// Uniform locations for shader variables (cached after first lookup for efficiency)
static GLint timeLoc = -1;             // Location of the 'time' uniform in the update shader
static GLint dtLoc = -1;               // Location of the 'dt' (delta time) uniform in the update shader
static GLint cloudHeightLoc = -1;      // Location of the 'cloudHeight' uniform in the update shader
static GLint restThresholdLoc = -1;    // Location of the 'restThreshold' uniform in the update shader
static GLint landscapeScaleLoc = -1;   // Location of the 'landscapeScale' uniform in the update shader
static GLint landscapeSizeLoc = -1;    // Location of the 'landscapeSize' uniform in the update shader
static GLint terrainMinXLoc = -1;      // Location of the 'terrainMinX' uniform in the update shader
static GLint terrainMaxXLoc = -1;      // Location of the 'terrainMaxX' uniform in the update shader
static GLint terrainMinZLoc = -1;      // Location of the 'terrainMinZ' uniform in the update shader
static GLint terrainMaxZLoc = -1;      // Location of the 'terrainMaxZ' uniform in the update shader
static GLint windLoc = -1;             // Location of the 'wind' uniform in the update shader
static GLint heightmapLoc = -1;        // Location of the 'heightmap' uniform in the update shader

/* --- Function: particleSystemInit ---
 * Sets up the entire GPU-based particle system.
 * Loads and links the update and render shaders, sets up transform feedback,
 * and initializes all particle data and OpenGL buffers for efficient simulation.
 */
void particleSystemInit(float terrainScale, float terrainHeight) {
    // Load and compile the update shader (vertex shader for transform feedback).
    // This shader is responsible for updating each particle's state (position, velocity, etc.) on the GPU.
    updateShader = loadShader("shaders/particle_update.vert", NULL); // Load the update shader from file.

    // Load and compile the render shader (vertex + fragment shaders for drawing particles as point sprites).
    renderShader = loadShader("shaders/particle_render.vert", "shaders/particle_render.frag"); // Load the render shader from files.

    // Bind attribute locations for the update shader so the layout matches our Particle struct.
    // This ensures the GPU interprets the buffer data correctly for each particle attribute.
    glBindAttribLocation(updateShader, 0, "pos");      // Attribute 0: Particle position (vec3)
    glBindAttribLocation(updateShader, 1, "vel");      // Attribute 1: Particle velocity (vec3)
    glBindAttribLocation(updateShader, 2, "restTime"); // Attribute 2: Particle rest time (float)
    glBindAttribLocation(updateShader, 3, "state");    // Attribute 3: Particle state (float)

    // Specify which outputs from the update shader should be captured by transform feedback.
    // This allows the GPU to write updated particle data directly into a buffer, avoiding CPU-GPU transfer.
    const char* varyings[] = { "outPos", "outVel", "outRestTime", "outState" }; // Names must match shader outputs.
    TF_SETUP(updateShader, 4, varyings); // Set up transform feedback to capture all four outputs.
    glLinkProgram(updateShader);         // Link the shader program so it's ready for use.

    // Generate two VAOs and two VBOs for ping-ponging particle data.
    // One buffer is used as the source (read), the other as the destination (write).
    VAO_GEN(2, particleVAOs); // Generate two VAOs for the two buffer sets.
    glGenBuffers(2, particleVBOs); // Generate two VBOs for the two buffer sets.

    // Allocate and initialize the particle data on the CPU.
    // Each particle is given a random position within the landscape, a random velocity, and default state values.
    // This randomness ensures that the weather effect (e.g., snow or rain) looks natural and not uniform.
    Particle* particles = (Particle*)malloc(NUM_PARTICLES * sizeof(Particle)); // Allocate memory for all particles.
    for (int i = 0; i < NUM_PARTICLES; ++i) {
        float x = terrainMinX + ((float)rand() / RAND_MAX) * (terrainMaxX - terrainMinX); // X position: random across landscape.
        float z = terrainMinZ + ((float)rand() / RAND_MAX) * (terrainMaxZ - terrainMinZ); // Z position: random across landscape.
        float y = cloudHeight + ((float)rand() / RAND_MAX) * 20.0f; // Y position: random height above terrain (cloud layer).
        float vx = ((float)rand() / RAND_MAX - 0.5f) * 4.0f;        // X velocity: random, simulates wind variation.
        float vy = -8.0f - ((float)rand() / RAND_MAX) * 4.0f;       // Y velocity: negative, simulates gravity pulling down.
        float vz = ((float)rand() / RAND_MAX - 0.5f) * 4.0f;        // Z velocity: random, simulates wind variation.
        particles[i].x = x; particles[i].y = y; particles[i].z = z; // Set position.
        particles[i].vx = vx; particles[i].vy = vy; particles[i].vz = vz; // Set velocity.
        particles[i].restTime = 0.0f; particles[i].state = 0.0f;    // Start at rest, default state.
    }
    // Upload the initial particle data to both VBOs (so both buffers are initialized identically).
    for (int b = 0; b < 2; ++b) {
        VAO_BIND(particleVAOs[b]); // Bind the VAO so we can set up its attributes.
        glBindBuffer(GL_ARRAY_BUFFER, particleVBOs[b]); // Bind the VBO to upload data.
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * sizeof(Particle), particles, GL_DYNAMIC_DRAW); // Upload data to GPU.
        // Set up attribute pointers so the GPU knows how to interpret the buffer data for each particle.
        glEnableVertexAttribArray(0); // Enable attribute 0 (position).
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, x)); // Position pointer.
        glEnableVertexAttribArray(1); // Enable attribute 1 (velocity).
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, vx)); // Velocity pointer.
        glEnableVertexAttribArray(2); // Enable attribute 2 (rest time).
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, restTime)); // Rest time pointer.
        glEnableVertexAttribArray(3); // Enable attribute 3 (state).
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, state)); // State pointer.
    }
    VAO_UNBIND(); // Unbind VAO to avoid accidental modification.
    free(particles); // Free the temporary CPU-side array, as data is now on the GPU.
}

/* --- Function: particleSystemUpdate ---
 * Updates all particles for the current frame using transform feedback.
 * Runs the update shader on the GPU, which simulates gravity, wind, and collision with the landscape.
 * Uses the ping-pong buffer technique to avoid read/write conflicts.
 */
void particleSystemUpdate(float dt) {
    int src = curSrc;         // Index of the current source buffer (where particle data is read from). This buffer contains the current state of all particles.
    int dst = 1 - curSrc;     // Index of the destination buffer (where updated data will be written). This buffer will receive the new state after the update.

    glUseProgram(updateShader); // Activate the update shader program. This shader will process each particle and output its new state.

    // Cache uniform locations on first use for efficiency. This avoids repeated lookups and speeds up subsequent frames.
    if (timeLoc == -1) {
        timeLoc = glGetUniformLocation(updateShader, "time");           // Uniform for elapsed time in seconds since program start.
        dtLoc = glGetUniformLocation(updateShader, "dt");               // Uniform for delta time (time since last frame).
        cloudHeightLoc = glGetUniformLocation(updateShader, "cloudHeight"); // Uniform for the height at which new particles spawn.
        restThresholdLoc = glGetUniformLocation(updateShader, "restThreshold"); // Uniform for how long a particle can rest before respawning.
        landscapeScaleLoc = glGetUniformLocation(updateShader, "landscapeScale"); // Uniform for the scale of the landscape.
        landscapeSizeLoc = glGetUniformLocation(updateShader, "landscapeSize");   // Uniform for the size of the landscape grid.
        terrainMinXLoc = glGetUniformLocation(updateShader, "terrainMinX");      // Uniform for minimum X coordinate of terrain.
        terrainMaxXLoc = glGetUniformLocation(updateShader, "terrainMaxX");      // Uniform for maximum X coordinate of terrain.
        terrainMinZLoc = glGetUniformLocation(updateShader, "terrainMinZ");      // Uniform for minimum Z coordinate of terrain.
        terrainMaxZLoc = glGetUniformLocation(updateShader, "terrainMaxZ");      // Uniform for maximum Z coordinate of terrain.
        windLoc = glGetUniformLocation(updateShader, "wind");                   // Uniform for wind vector (X, Z).
        heightmapLoc = glGetUniformLocation(updateShader, "heightmap");         // Uniform for heightmap texture sampler.
    }

    // Set all the uniforms needed by the update shader. These provide the shader with the current simulation parameters and environment state.
    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // Get the elapsed time in seconds since the program started.
    glUniform1f(timeLoc, time);                        // Pass the current time to the shader.
    glUniform1f(dtLoc, dt);                            // Pass the time step for this frame.
    glUniform1f(cloudHeightLoc, cloudHeight);          // Pass the height at which new particles should spawn.
    glUniform1f(restThresholdLoc, 5.0f);               // Pass the threshold for how long a particle can rest.
    glUniform1f(landscapeScaleLoc, LANDSCAPE_SCALE);   // Pass the scale of the landscape.
    glUniform1f(landscapeSizeLoc, LANDSCAPE_SIZE);     // Pass the size of the landscape grid.
    glUniform1f(terrainMinXLoc, terrainMinX);          // Pass the minimum X coordinate of the terrain.
    glUniform1f(terrainMaxXLoc, terrainMaxX);          // Pass the maximum X coordinate of the terrain.
    glUniform1f(terrainMinZLoc, terrainMinZ);          // Pass the minimum Z coordinate of the terrain.
    glUniform1f(terrainMaxZLoc, terrainMaxZ);          // Pass the maximum Z coordinate of the terrain.
    glUniform2f(windLoc, 1.0f, 0.5f);                  // Pass the wind vector (X, Z) to the shader.
    glUniform1i(heightmapLoc, 0);                      // Tell the shader to use texture unit 0 for the heightmap.

    glActiveTexture(GL_TEXTURE0);                      // Activate texture unit 0 for the heightmap.
    glBindTexture(GL_TEXTURE_2D, heightmapTex);        // Bind the heightmap texture so the shader can sample terrain elevation for collision.

    VAO_BIND(particleVAOs[src]);                       // Bind the VAO containing the current particle data (source buffer).
    TF_BIND_BUFFER(particleVBOs[dst]);                 // Bind the destination buffer for transform feedback. This is where the updated particle data will be written.
    RASTER_DISCARD_ON();                               // Enable rasterizer discard. This tells OpenGL not to generate any fragments (pixels) during this pass,
                                                      // since we are only interested in updating data, not rendering anything to the screen.
    TF_BEGIN();                                        // Begin transform feedback. This tells OpenGL to capture the outputs of the vertex shader and write them to the buffer.
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);         // Issue a draw call for all particles as points. Each point will be processed by the update shader.
    TF_END();                                          // End transform feedback. All updated particle data is now in the destination buffer.
    RASTER_DISCARD_OFF();                              // Disable rasterizer discard so future draw calls will render as normal.
    TF_UNBIND_BUFFER();                                // Unbind the transform feedback buffer to avoid accidental modification.
    VAO_UNBIND();                                      // Unbind the VAO to avoid accidental modification.
    glUseProgram(0);                                   // Unbind the shader program.

    curSrc = dst;                                      // Swap the source and destination buffers for the next frame.
                                                      // This is the core of the ping-pong technique: next frame, the updated data becomes the source.
}

/* --- Concept: Point Sprites ---
 * Point sprites are a way to render each particle as a textured square (billboard) that always faces the camera.
 * This is much more efficient than rendering a full quad for each particle, and is ideal for effects like snow and rain.
 * The render shader can control the color, size, and transparency of each point sprite for a soft, natural look.
 */

/* --- Function: particleSystemRender ---
 * Renders all particles as point sprites using the render shader.
 * Sets up OpenGL state for blending and point size, then draws all particles in a single call.
 */
void particleSystemRender() {
    glUseProgram(renderShader); // Activate the render shader program. This shader will handle the appearance of each particle when drawn.
    POINT_SPRITE_ON();          // Enable point sprite rendering (if supported on this platform). This allows each particle to be drawn as a camera-facing square.
    glEnable(GL_BLEND);         // Enable alpha blending so particles can be semi-transparent and blend smoothly with the background and each other.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set the blending function to standard alpha blending (source over destination).

    VAO_BIND(particleVAOs[curSrc]); // Bind the VAO containing the current particle data. This tells OpenGL which buffer and attribute layout to use.
    glPointSize(30.0f);         // Set the size of each particle in pixels. Larger values make particles appear bigger on screen.
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES); // Draw all particles as points. Each point will be rendered as a sprite by the shader.
    VAO_UNBIND();               // Unbind the VAO to avoid accidental modification or conflicts with other draw calls.

    POINT_SPRITE_OFF();         // Disable point sprite rendering (if it was enabled). This restores OpenGL state for the rest of the scene.
    glDisable(GL_BLEND);        // Disable blending to avoid affecting subsequent rendering operations.
    glUseProgram(0);            // Unbind the shader program to clean up OpenGL state.
}

/* --- Function: particleSystemCleanup ---
 * Deletes all OpenGL resources used by the particle system.
 * This is important to avoid memory leaks when the program exits or the system is reinitialized.
 */
void particleSystemCleanup() {
    // Delete the VAOs and VBOs to free GPU resources.
    // This is important to avoid memory leaks when the program exits or the system is reinitialized.
    VAO_DELETE(2, particleVAOs);      // Delete both Vertex Array Objects (VAOs) used for ping-pong buffering.
    glDeleteBuffers(2, particleVBOs); // Delete both Vertex Buffer Objects (VBOs) used for ping-pong buffering.
}

/* --- Function: particleSystemSetEnabled ---
 * Placeholder for enabling/disabling the particle system.
 * Could be used to toggle weather effects on/off in the future.
 */
void particleSystemSetEnabled(int enabled) {
    // This function is a placeholder for enabling/disabling the particle system.
    // It could be used in the future to toggle weather effects on or off, pause the simulation, or implement other controls.
    // Currently, it does nothing, but it is included for modularity and future extensibility.
}

/* --- Function: particleSystemUploadHeightmap ---
 * Uploads the landscape elevation data as a 128x128 single-channel (red) texture.
 * This texture is used by the update shader to detect when particles hit the ground.
 */
void particleSystemUploadHeightmap(float* elevationData) {
    // This function uploads the landscape elevation data as a 128x128 single-channel (red) texture to the GPU.
    // The update shader uses this texture to detect when particles hit the ground, enabling realistic collision and respawn behavior.
    if (!heightmapTex) { // If the heightmap texture has not been created yet...
        glGenTextures(1, &heightmapTex); // Generate a new texture object and store its handle.
        glBindTexture(GL_TEXTURE_2D, heightmapTex); // Bind the texture so we can set its parameters and upload data.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Set linear filtering for smooth sampling when minifying.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Set linear filtering for smooth sampling when magnifying.
    } else {
        glBindTexture(GL_TEXTURE_2D, heightmapTex); // If the texture already exists, just bind it for updating.
    }
    // Upload the elevation data to the GPU as a single-channel (GL_RED) floating-point texture.
    // The data is assumed to be a 128x128 array of floats representing terrain elevation.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 128, 128, 0, GL_RED, GL_FLOAT, elevationData); // Upload data to GPU.
}