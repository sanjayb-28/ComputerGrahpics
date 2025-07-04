/*
 * Boulder System for Boulder Scene - Procedural Rock Generation and Rendering
 *
 * This component implements a procedural boulder generation system that creates realistic rock formations
 * scattered throughout the landscape. It features procedurally generated boulder shapes with noise-based
 * vertex displacement, collision detection with other landscape objects, terrain-aware placement, and
 * shader-based rendering with texture mapping and lighting integration.
 *
 * Key Concepts:
 * - Procedural Generation: Each boulder has a unique shape generated from a base mesh with noise displacement.
 * - Collision Detection: Boulder placement avoids trees and other landscape objects for realistic distribution.
 * - Terrain Integration: Boulders are placed on suitable terrain slopes and above water level.
 * - Shader Rendering: Custom shaders provide texture mapping, lighting, and color variation.
 * - Mesh Generation: Complex polyhedral boulder shapes with proper normal calculations for lighting.
 *
 * Function Roles:
 * - freeBoulders: Cleans up boulder memory and resets the system state.
 * - boulderCollides: Checks for collisions between boulder placement and existing trees.
 * - boulderRandomScale: Generates random scale factors for boulder size variation.
 * - boulderNoise: Produces noise values for vertex displacement to create unique boulder shapes.
 * - boulderVertexNoise: Applies noise displacement to base vertices for procedural shape generation.
 * - isValidBoulderLocation: Validates boulder placement based on terrain slope and object collisions.
 * - generateRandomBoulder: Creates a single boulder instance with random properties and placement.
 * - initBoulders: Initializes the entire boulder system with procedural placement across the landscape.
 * - computeNormal: Calculates surface normals for proper lighting calculations.
 * - drawBoulderFace: Renders a single triangular face with texture coordinates and normals.
 * - drawBoulderMesh: Renders the complete boulder mesh using triangle faces.
 * - setupBoulderTransform: Applies transformation matrix for boulder positioning and scaling.
 * - cleanupBoulderDraw: Restores OpenGL state after boulder rendering.
 * - boulderDraw: Main rendering function that combines all boulder rendering steps.
 * - renderBoulders: Renders all boulders in the scene with their individual properties.
 * - boulderShaderInit: Initializes the boulder shader program for advanced rendering.
 */

#include "CSCIx229.h"
#include "boulder.h"
#include "objects_render.h"
#include "landscape.h"
#include "shaders.h"

// Maximum number of boulders to generate in the scene
#define NUM_BOULDERS 50

// Global boulder system state variables
static BoulderInstance* boulders = NULL; // Array of boulder instances
static int numBoulders = 0; // Current number of boulders
static int boulderShader = 0; // Shader program handle for boulder rendering

// External references to other systems
extern TreeInstance* treeInstances; // Tree instances for collision detection
extern int numTrees; // Number of trees in the scene
extern GLuint boulderTexture; // Texture handle for boulder surface

// Utility function to generate random float values between 0.0 and 1.0
static float randf() { return rand() / (float)RAND_MAX; }

// freeBoulders: Cleans up boulder memory and resets the system state.
// Contribution: This function ensures proper memory management by deallocating the boulder array and resetting system state. It prevents memory leaks and allows the system to be reinitialized cleanly.
void freeBoulders() {
    if (boulders) { // Check if boulder array exists
        free(boulders); // Deallocate boulder memory
        boulders = NULL; // Reset pointer to null
        numBoulders = 0; // Reset boulder count
    }
}

// boulderCollides: Checks for collisions between boulder placement and existing trees.
// Contribution: This function prevents boulders from being placed too close to trees, ensuring realistic object distribution. It calculates distance-based collision detection using squared distance for efficiency.
static int boulderCollides(float x, float z, float minDist) {
    for (int t = 0; t < numTrees; ++t) { // Iterate through all trees
        float dx = x - treeInstances[t].x; // Calculate X distance to tree
        float dz = z - treeInstances[t].z; // Calculate Z distance to tree
        float dist2 = dx*dx + dz*dz; // Calculate squared distance (avoiding square root)
        float minTreeDist = minDist + treeInstances[t].scale * 0.5f; // Calculate minimum required distance
        if (dist2 < minTreeDist * minTreeDist) { // Check if distance is too small
            return 1; // Collision detected
        }
    }
    return 0; // No collision found
}

// boulderRandomScale: Generates random scale factors for boulder size variation.
// Contribution: This function creates natural size variation for boulders using multiple random factors. The combination of uniform and multiplicative randomness produces realistic size distribution.
static float boulderRandomScale() {
    float a = randf(); // First random factor
    float b = randf(); // Second random factor
    float c = randf(); // Third random factor
    return 1.2f + a * 2.8f + b * c * 1.2f; // Combine factors for natural variation
}

// boulderNoise: Produces noise values for vertex displacement to create unique boulder shapes.
// Contribution: This function generates procedural noise for vertex displacement using trigonometric functions. The combination of sine and cosine with different frequencies creates natural-looking surface variation.
static float boulderNoise(unsigned int shapeSeed, int i, int j) {
    return (sinf(shapeSeed * 0.13f + i * 1.7f + j * 2.3f) + cosf(shapeSeed * 0.21f + i * 2.1f + j * 1.3f)) * 0.18f * randf(); // Generate noise using trigonometric functions and random scaling
}

// boulderVertexNoise: Applies noise displacement to base vertices for procedural shape generation.
// Contribution: This function creates unique boulder shapes by displacing base vertices with noise. Each boulder gets a different shape based on its seed, ensuring visual variety in the scene.
static void boulderVertexNoise(float verts[28][3], const float baseVerts[28][3], unsigned int shapeSeed) {
    for (int i = 0; i < 28; ++i) { // Iterate through all vertices
        for (int j = 0; j < 3; ++j) { // Iterate through X, Y, Z components
            verts[i][j] = baseVerts[i][j] + boulderNoise(shapeSeed, i, j); // Add noise displacement to base vertex
        }
    }
}

// Base vertex positions for the boulder polyhedron (28 vertices forming a complex rock shape)
static const float baseVerts[28][3] = {
    {0.0f, 1.0f, 0.0f}, {0.8f, 0.6f, 0.2f}, {0.5f, 0.5f, -0.9f}, {-0.7f, 0.7f, -0.6f}, // Top vertices
    {-1.0f, 0.5f, 0.4f}, {0.0f, -0.1f, 1.1f}, {1.1f, -0.2f, -0.3f}, {0.4f, -0.8f, -1.0f}, // Upper middle vertices
    {-0.8f, -0.6f, -0.8f}, {-1.0f, -0.7f, 0.6f}, {0.6f, 0.2f, 0.8f}, {-0.5f, 0.1f, 1.0f}, // Lower middle vertices
    {1.0f, 0.1f, 0.5f}, {1.2f, -0.5f, 0.2f}, {0.7f, -0.7f, 0.7f}, {-0.2f, -1.0f, 0.2f}, // Bottom vertices
    {-0.9f, -0.9f, -0.2f}, {-0.3f, -0.8f, 0.9f}, {0.3f, 0.7f, 0.7f}, {0.9f, -0.3f, 0.9f}, // Additional vertices for complexity
    {-0.6f, 0.3f, 1.0f}, {1.1f, 0.3f, -0.7f}, {-1.1f, 0.2f, -0.5f}, {0.2f, -0.9f, -0.7f}, // More vertices for detail
    {-0.7f, -0.8f, 0.3f}, {0.8f, -0.7f, -0.6f}, {-0.3f, 0.9f, 0.2f}, {0.5f, -0.5f, 1.0f} // Final vertices
};

// Triangle face definitions for the boulder polyhedron (52 faces forming the complete mesh)
static const int faces[52][3] = {
    {0,1,2},{0,2,3},{0,3,4},{0,4,1},{1,10,12},{1,12,2},{2,12,7},{2,7,3},{3,7,8},{3,8,4},{4,8,9},{4,9,1}, // Top and upper faces
    {1,9,11},{1,11,10},{5,10,11},{5,11,9},{5,9,8},{5,8,7},{5,7,13},{5,13,14},{5,14,10},{10,14,12},{12,14,13},{12,13,7}, // Middle faces
    {6,12,13},{6,13,7},{6,7,2},{6,2,12},{6,12,10},{6,10,15},{6,15,16},{6,16,7},{17,18,19},{17,19,20},{17,20,21},{17,21,18}, // Lower faces
    {18,22,23},{18,23,19},{19,23,24},{19,24,20},{20,24,25},{20,25,21},{21,25,26},{21,26,18},{18,26,22},{22,26,25},{22,25,23},{23,25,24} // Bottom faces
};

// boulderShaderUniforms: Sets up shader uniforms for boulder rendering including textures and lighting.
// Contribution: This function configures the boulder shader with texture binding, lighting parameters, and color variation. It enables advanced rendering effects like texture mapping and dynamic lighting.
static void boulderShaderUniforms(int colorIndex) {
    glActiveTexture(GL_TEXTURE0); // Activate texture unit 0
    glBindTexture(GL_TEXTURE_2D, boulderTexture); // Bind boulder texture to texture unit
    GLint texLoc = glGetUniformLocation(boulderShader, "boulderTex"); // Get texture uniform location
    glUniform1i(texLoc, 0); // Set texture uniform to use texture unit 0
    glEnable(GL_TEXTURE_2D); // Enable texture mapping
    float lightPos[4], diffuse[4]; // Arrays to store lighting parameters
    glGetLightfv(GL_LIGHT0, GL_POSITION, lightPos); // Get light position from OpenGL
    glGetLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); // Get light diffuse color from OpenGL
    GLint lightColorLoc = glGetUniformLocation(boulderShader, "lightColor"); // Get light color uniform location
    glUniform3fv(lightColorLoc, 1, diffuse); // Set light color uniform
    GLint colorIndexLoc = glGetUniformLocation(boulderShader, "boulderColorIndex"); // Get color index uniform location
    glUniform1i(colorIndexLoc, colorIndex); // Set color index for boulder variation
    GLint lightPosLoc = glGetUniformLocation(boulderShader, "lightPos"); // Get light position uniform location
    glUniform3fv(lightPosLoc, 1, lightPos); // Set light position uniform
}

// isValidBoulderLocation: Validates boulder placement based on terrain slope and object collisions.
// Contribution: This function ensures boulders are placed in suitable locations by checking terrain slope, water level, and object collisions. It converts world coordinates to terrain grid coordinates for accurate height and normal sampling.
static int isValidBoulderLocation(Landscape* landscape, float x, float z, float y) {
    float nx = (x / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1); // Convert X to terrain grid coordinate
    float nz = (z / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1); // Convert Z to terrain grid coordinate
    int ix = (int)nx; // Integer X grid index
    int iz = (int)nz; // Integer Z grid index
    if (ix < 0) ix = 0; // Clamp to terrain bounds
    if (iz < 0) iz = 0; // Clamp to terrain bounds
    if (ix >= LANDSCAPE_SIZE-1) ix = LANDSCAPE_SIZE-2; // Clamp to terrain bounds
    if (iz >= LANDSCAPE_SIZE-1) iz = LANDSCAPE_SIZE-2; // Clamp to terrain bounds
    int idx = iz * LANDSCAPE_SIZE + ix; // Calculate terrain array index
    float* n = &landscape->normals[idx*3]; // Get normal vector at this location
    float slope = acosf(fminf(fmaxf(n[1], -1.0f), 1.0f)) / (float)M_PI; // Calculate slope angle from Y normal component
    if (slope > 0.25f) return 0; // Reject if slope is too steep (greater than ~14 degrees)
    if (y < WATER_LEVEL + 0.5f) return 0; // Reject if below water level plus safety margin
    if (boulderCollides(x, z, 4.0f)) return 0; // Reject if collides with trees
    return 1; // Location is valid
}

// generateRandomBoulder: Creates a single boulder instance with random properties and placement.
// Contribution: This function generates individual boulders with random position, scale, rotation, shape seed, and color. It attempts placement until a valid location is found, ensuring realistic distribution.
static int generateRandomBoulder(Landscape* landscape, BoulderInstance* outBoulder) {
    float halfScale = LANDSCAPE_SCALE * 0.5f * 0.95f; // Calculate placement boundary (95% of terrain size)
    float x = -halfScale + randf() * LANDSCAPE_SCALE * 0.95f; // Random X position within bounds
    float z = -halfScale + randf() * LANDSCAPE_SCALE * 0.95f; // Random Z position within bounds
    float y = landscapeGetHeight(landscape, x, z); // Get terrain height at position
    if (!isValidBoulderLocation(landscape, x, z, y)) return 0; // Check if location is valid
    float scale = boulderRandomScale(); // Generate random scale
    float rotation = randf() * 360.0f; // Generate random rotation (0-360 degrees)
    unsigned int shapeSeed = rand(); // Generate random shape seed for procedural variation
    int colorIndex = rand() % 8; // Generate random color index (0-7)
    *outBoulder = (BoulderInstance){x, y, z, scale, rotation, shapeSeed, colorIndex}; // Create boulder instance
    return 1; // Success
}

// initBoulders: Initializes the entire boulder system with procedural placement across the landscape.
// Contribution: This function creates the complete boulder system by generating multiple boulders with valid placement. It uses a retry mechanism to ensure all boulders are placed successfully, even if some locations are invalid.
void initBoulders(Landscape* landscape) {
    freeBoulders(); // Clean up any existing boulders
    if (!landscape) return; // Early exit if landscape is not available
    boulders = (BoulderInstance*)malloc(sizeof(BoulderInstance) * NUM_BOULDERS); // Allocate boulder array
    numBoulders = 0; // Initialize boulder count
    int attempts = 0; // Track placement attempts
    while (numBoulders < NUM_BOULDERS && attempts < NUM_BOULDERS * 10) { // Continue until all boulders placed or max attempts reached
        attempts++; // Increment attempt counter
        BoulderInstance b; // Temporary boulder instance
        if (generateRandomBoulder(landscape, &b)) { // Try to generate a boulder
            boulders[numBoulders++] = b; // Add successful boulder to array
        }
    }
}

// computeNormal: Calculates surface normals for proper lighting calculations.
// Contribution: This function computes the normal vector for a triangular face using cross product of edge vectors. It ensures proper lighting by providing accurate surface orientation information to the rendering pipeline.
static void computeNormal(const float* v0, const float* v1, const float* v2, float* nx, float* ny, float* nz) {
    float ux = v1[0] - v0[0], uy = v1[1] - v0[1], uz = v1[2] - v0[2]; // Calculate first edge vector
    float vx = v2[0] - v0[0], vy = v2[1] - v0[1], vz = v2[2] - v0[2]; // Calculate second edge vector
    *nx = uy * vz - uz * vy; // Cross product X component
    *ny = uz * vx - ux * vz; // Cross product Y component
    *nz = ux * vy - uy * vx; // Cross product Z component
    float len = sqrtf((*nx)*(*nx) + (*ny)*(*ny) + (*nz)*(*nz)); // Calculate normal length
    if (len > 0.0001f) { // Check if normal is not zero
        *nx /= len; *ny /= len; *nz /= len; // Normalize the normal vector
    }
}

// drawBoulderFace: Renders a single triangular face with texture coordinates and normals.
// Contribution: This function renders individual triangular faces with proper lighting and texture mapping. It calculates face normals and sets up texture coordinates for realistic surface rendering.
static void drawBoulderFace(const float verts[28][3], const int face[3]) {
    float* v0 = (float*)verts[face[0]]; // Get first vertex pointer
    float* v1 = (float*)verts[face[1]]; // Get second vertex pointer
    float* v2 = (float*)verts[face[2]]; // Get third vertex pointer
    float nx, ny, nz; // Normal components
    computeNormal(v0, v1, v2, &nx, &ny, &nz); // Calculate face normal
    glNormal3f(nx, ny, nz); // Set OpenGL normal for lighting
    glTexCoord2f(v0[0]*0.5f+0.5f, v0[2]*0.5f+0.5f); glVertex3fv(v0); // Set texture coordinates and vertex for first point
    glTexCoord2f(v1[0]*0.5f+0.5f, v1[2]*0.5f+0.5f); glVertex3fv(v1); // Set texture coordinates and vertex for second point
    glTexCoord2f(v2[0]*0.5f+0.5f, v2[2]*0.5f+0.5f); glVertex3fv(v2); // Set texture coordinates and vertex for third point
}

// drawBoulderMesh: Renders the complete boulder mesh using triangle faces.
// Contribution: This function renders the entire boulder by drawing all triangular faces. It uses OpenGL immediate mode to create the complete polyhedral shape with proper face definitions.
static void drawBoulderMesh(const float verts[28][3], const int faces[52][3]) {
    glBegin(GL_TRIANGLES); // Start triangle rendering
    for (int f = 0; f < 52; ++f) { // Iterate through all faces
        drawBoulderFace(verts, faces[f]); // Draw each triangular face
    }
    glEnd(); // End triangle rendering
}

// setupBoulderTransform: Applies transformation matrix for boulder positioning and scaling.
// Contribution: This function sets up the OpenGL transformation matrix for boulder rendering. It applies translation, rotation, and scaling to position and orient each boulder correctly in 3D space.
static void setupBoulderTransform(float x, float y, float z, float scale, float rotation) {
    glPushMatrix(); // Save current matrix state
    glTranslatef(x, y, z); // Apply translation to boulder position
    glRotatef(rotation, 0, 1, 0); // Apply rotation around Y axis
    glScalef(scale, scale, scale); // Apply uniform scaling
}

// cleanupBoulderDraw: Restores OpenGL state after boulder rendering.
// Contribution: This function cleans up OpenGL state after boulder rendering by disabling textures and restoring the transformation matrix. It ensures the rendering state is consistent for subsequent operations.
static void cleanupBoulderDraw() {
    glDisable(GL_TEXTURE_2D); // Disable texture mapping
    glPopMatrix(); // Restore previous matrix state
}

// boulderDraw: Main rendering function that combines all boulder rendering steps.
// Contribution: This function orchestrates the complete boulder rendering process. It generates the procedural mesh, sets up transformations, applies shaders, renders the geometry, and cleans up state. This is the primary interface for rendering individual boulders.
void boulderDraw(float x, float y, float z, float scale, float rotation, unsigned int shapeSeed, int colorIndex) {
    float verts[28][3]; // Vertex array for this boulder instance
    boulderVertexNoise(verts, baseVerts, shapeSeed); // Generate procedural vertices

    setupBoulderTransform(x, y, z, scale, rotation); // Set up transformation matrix

    if (boulderShader) { // Check if shader is available
        useShader(boulderShader); // Activate boulder shader
        boulderShaderUniforms(colorIndex); // Set up shader uniforms
    }

    drawBoulderMesh(verts, faces); // Render the boulder mesh

    if (boulderShader) useShader(0); // Deactivate shader if it was used

    cleanupBoulderDraw(); // Clean up OpenGL state
}

// renderBoulders: Renders all boulders in the scene with their individual properties.
// Contribution: This function renders the entire boulder system by iterating through all boulder instances. It calls the individual boulder rendering function for each boulder with its specific properties, creating the complete boulder scene.
void renderBoulders() {
    for (int i = 0; i < numBoulders; ++i) { // Iterate through all boulders
        BoulderInstance* b = &boulders[i]; // Get current boulder instance
        boulderDraw(b->x, b->y, b->z, b->scale, b->rotation, b->shapeSeed, b->colorIndex); // Render this boulder
    }
}

// boulderShaderInit: Initializes the boulder shader program for advanced rendering.
// Contribution: This function loads and initializes the boulder shader program from vertex and fragment shader files. It enables advanced rendering features like texture mapping, lighting, and color variation for realistic boulder appearance.
void boulderShaderInit() {
    boulderShader = loadShader("shaders/boulder_shader.vert", "shaders/boulder_shader.frag"); // Load shader program
} 