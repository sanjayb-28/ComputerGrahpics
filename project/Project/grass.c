/*
 * Grass Rendering and Animation System for Boulder Scene
 *
 * This component implements procedural generation, animation, and efficient rendering of grass blades
 * across the landscape. It leverages instanced rendering and custom shaders to create a visually rich,
 * dynamic field of grass that sways in the wind and responds to lighting. The system is designed for
 * performance and realism, using randomized geometry and per-blade attributes to avoid repetition.
 *
 * Key Concepts:
 * - Procedural Placement: Grass blades are distributed randomly, but only on plausible terrain (not too steep, not underwater).
 * - Per-Blade Variation: Each blade has unique height, width, color, and animation seed for natural variety.
 * - Instanced Rendering: All blades are packed into a single vertex buffer and drawn in one call for efficiency.
 * - Shader Animation: Swaying and lighting are handled in the vertex/fragment shaders using per-blade attributes.
 * - Resource Management: All OpenGL resources are properly allocated and freed.
 *
 * Function Roles:
 * - randomFloat: Utility for randomization, used throughout for natural variation.
 * - isValidGrassLocation: Ensures grass only appears on suitable terrain.
 * - generateGrassBlade: Creates a single blade with randomized geometry and attributes.
 * - generateGrassBlades: Populates the vertex buffer with many blades.
 * - setupGrassGL: Uploads data to the GPU and sets up OpenGL state.
 * - grassSystemInit: Orchestrates the full initialization process.
 * - setAttrib: Helper for binding vertex attributes in the shader.
 * - grassSystemRender: Handles all rendering, animation, and lighting for the grass.
 * - grassSystemCleanup: Frees all OpenGL and CPU resources.
 */

#include "CSCIx229.h"
#include "grass.h"
#include "shaders.h"

// Structure encoding all per-vertex and per-blade attributes for a grass blade.
typedef struct {
    float x, y, z;           // World-space position of the base of the blade
    float swaySeed;          // Random seed for unique swaying animation
    float offsetX, offsetY;  // Offset for triangle vertex (defines blade shape)
    float bladeHeight, bladeWidth; // Blade geometry
    float colorVar;          // Color variation for natural look
    float rotation;          // Random rotation for orientation
} GrassVertex;

// OpenGL handles and state for the grass system
static GLuint grassVBO = 0, grassVAO = 0;
static GLuint grassShader = 0;
static GLuint grassTex = 0;
static int grassCount = 0;

// randomFloat: Generates a random float between a and b.
// Used throughout the grass system to randomize blade positions, sizes, and animation seeds for natural variety.
static float randomFloat(float a, float b) {
    // Generate a random float in [a, b] using rand().
    return a + ((float)rand() / RAND_MAX) * (b - a);
}

// isValidGrassLocation: Determines if a given (x, z, y) position is suitable for grass placement.
// Checks for water level and slope, ensuring grass only appears on plausible terrain.
static int isValidGrassLocation(Landscape* landscape, float x, float z, float y) {
    // Reject locations below water level (with a small margin).
    if (y < WATER_LEVEL + 0.2f) return 0;
    // Convert world coordinates to grid indices for the landscape.
    float nx = (x / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    float nz = (z / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    int ix = (int)nx;
    int iz = (int)nz;
    // Clamp indices to valid range to avoid out-of-bounds.
    if (ix < 0) ix = 0;
    if (iz < 0) iz = 0;
    if (ix >= LANDSCAPE_SIZE-1) ix = LANDSCAPE_SIZE-2;
    if (iz >= LANDSCAPE_SIZE-1) iz = LANDSCAPE_SIZE-2;
    // Get the normal vector at this location to compute slope.
    float* normal = &landscape->normals[(iz * LANDSCAPE_SIZE + ix) * 3];
    // Slope is the angle between the normal and the vertical axis.
    float slope = acosf(fminf(fmaxf(normal[1], -1.0f), 1.0f));
    float slopeDeg = slope * (180.0f / M_PI);
    // Only allow grass on slopes less than or equal to 32 degrees.
    return slopeDeg <= 32.0f;
}

// generateGrassBlade: Generates a single grass blade at a random valid location, with randomized geometry and color.
// Populates the GrassVertex array with the blade's triangle vertices, encoding all per-blade attributes for animation and shading.
static void generateGrassBlade(Landscape* landscape, float areaSize, GrassVertex* verts, int* bladeIdx) {
    float clampFactor = 0.98f; // Avoid placing blades at the very edge of the area.
    float halfScale = areaSize * 0.5f * clampFactor; // Calculate the actual placement radius
    // Randomly choose a position within the allowed area.
    float x = randomFloat(-halfScale, halfScale);
    float z = randomFloat(-halfScale, halfScale);
    // Query the terrain height at this position.
    float y = landscapeGetHeight(landscape, x, z);
    // Only proceed if the location is valid for grass.
    if (!isValidGrassLocation(landscape, x, z, y)) return;
    // Randomize per-blade attributes for animation and appearance.
    float swaySeed = randomFloat(0.0f, 1.0f); // Unique animation phase
    float bladeHeight = randomFloat(0.7f, 1.5f); // Vary blade height
    float bladeWidth = randomFloat(0.05f, 0.13f); // Vary blade width
    float colorVar = randomFloat(-0.08f, 0.08f); // Subtle color variation
    float rotation = randomFloat(0.0f, 2.0f * (float)M_PI); // Random orientation
    int colorIndex = rand() % 4; // Discrete color band for extra variety
    // Each blade is a triangle (3 vertices), with offsets defining its shape.
    for (int v = 0; v < 3; ++v) {
        GrassVertex vert = {x, y, z, swaySeed, 0, 0, bladeHeight, bladeWidth, colorVar + colorIndex * 0.25f, rotation}; // Initialize vertex with base values
        switch (v) {
            case 0: vert.offsetX = 0; vert.offsetY = 0; break; // Base left
            case 1: vert.offsetX = bladeWidth; vert.offsetY = 0; break; // Base right
            case 2: vert.offsetX = bladeWidth/2; vert.offsetY = bladeHeight; break; // Tip
        }
        verts[(*bladeIdx)++] = vert; // Store the vertex in the buffer.
    }
}

// generateGrassBlades: Generates multiple grass blades by repeatedly calling generateGrassBlade.
// Fills the vertex buffer with a dense, randomized field of grass for instanced rendering.
static void generateGrassBlades(Landscape* landscape, float areaSize, int numBlades, GrassVertex* data) {
    int bladeIdx = 0; // Track the current position in the vertex buffer
    // Attempt to generate the requested number of blades.
    for (int i = 0; i < numBlades; ++i) {
        generateGrassBlade(landscape, areaSize, data, &bladeIdx);
    }
}

// setupGrassGL: Initializes OpenGL buffers, vertex arrays, shaders, and textures for grass rendering.
// Uploads all blade geometry to the GPU and prepares the system for efficient instanced drawing.
static void setupGrassGL(GrassVertex* data, int numVerts) {
#ifdef __APPLE__
    glGenVertexArraysAPPLE(1, &grassVAO); // Create vertex array object for macOS
    glBindVertexArrayAPPLE(grassVAO); // Bind it for use
#else
    glGenVertexArrays(1, &grassVAO); // Create vertex array object for other platforms
    glBindVertexArray(grassVAO); // Bind it for use
#endif
    // Create and bind the vertex buffer for all grass blades.
    glGenBuffers(1, &grassVBO); // Generate buffer object
    glBindBuffer(GL_ARRAY_BUFFER, grassVBO); // Bind as array buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrassVertex) * numVerts, data, GL_STATIC_DRAW); // Upload vertex data to GPU
    // Load the custom grass shader and texture.
    grassShader = loadShader("shaders/grass.vert", "shaders/grass.frag"); // Load vertex and fragment shaders
    grassTex = LoadTexBMP("tex/leaf.bmp"); // Load grass blade texture
    // Free the CPU-side data after uploading to GPU.
    free(data); // Release memory since data is now on GPU
}

// grassSystemInit: Entry point for creating the grass system.
// Allocates memory, generates all blades, and sets up OpenGL state for rendering animated grass.
void grassSystemInit(Landscape* landscape, float areaSize, int numBlades) {
    grassCount = numBlades; // Store the total number of blades
    // Allocate space for all blade vertices (3 per blade).
    GrassVertex* data = (GrassVertex*)malloc(sizeof(GrassVertex) * 3 * numBlades); // Allocate memory for all vertices
    // Generate all blades and fill the buffer.
    generateGrassBlades(landscape, areaSize, numBlades, data); // Populate the vertex buffer
    // Upload to GPU and set up OpenGL state.
    setupGrassGL(data, grassCount * 3); // Initialize OpenGL resources
}

// setAttrib: Helper for binding vertex attribute pointers in the shader program.
// Ensures all per-blade and per-vertex data is correctly mapped for the grass vertex shader.
static void setAttrib(GLuint shader, const char* name, int size, int stride, int offset) {
    // Query the attribute location in the shader.
    GLint loc = glGetAttribLocation(shader, name); // Get attribute location by name
    if (loc >= 0) { // Only set if attribute exists in shader
        // Enable and set the attribute pointer.
        glEnableVertexAttribArray(loc); // Enable this attribute
        glVertexAttribPointer(loc, size, GL_FLOAT, GL_FALSE, stride, (void*)(size_t)offset); // Set attribute pointer
    }
}

// grassSystemRender: Renders all grass blades with animation and lighting.
// Sets shader uniforms, binds buffers and textures, and issues the draw call for instanced grass.
void grassSystemRender(float time, float windStrength, const float sunDir[3], const float ambient[3]) {
    // Early out if the system is not initialized.
    if (!grassShader || !grassVBO || !grassVAO) return; // Check if OpenGL resources are ready
    // Use the custom grass shader program.
    glUseProgram(grassShader); // Activate the grass shader
    // Set animation and lighting uniforms for the shader.
    glUniform1f(glGetUniformLocation(grassShader, "time"), time); // Pass current time for animation
    glUniform1f(glGetUniformLocation(grassShader, "windStrength"), windStrength); // Pass wind strength
    glUniform3fv(glGetUniformLocation(grassShader, "sunDir"), 1, sunDir); // Pass sun direction for lighting
    glUniform3fv(glGetUniformLocation(grassShader, "ambient"), 1, ambient); // Pass ambient light
    // Bind the grass texture to texture unit 0.
    glActiveTexture(GL_TEXTURE0); // Activate texture unit 0
    glBindTexture(GL_TEXTURE_2D, grassTex); // Bind grass texture
    glUniform1i(glGetUniformLocation(grassShader, "grassTex"), 0); // Tell shader to use texture unit 0
#ifdef __APPLE__
    glBindVertexArrayAPPLE(grassVAO); // Bind VAO for macOS
#else
    glBindVertexArray(grassVAO); // Bind VAO for other platforms
#endif
    // Bind the vertex buffer and set all attribute pointers.
    glBindBuffer(GL_ARRAY_BUFFER, grassVBO); // Bind vertex buffer
    int stride = sizeof(GrassVertex); // Calculate stride between vertices
    setAttrib(grassShader, "position", 3, stride, 0); // Set position attribute (x, y, z)
    setAttrib(grassShader, "swaySeed", 1, stride, sizeof(float)*3); // Set sway seed attribute
    setAttrib(grassShader, "offsetX", 1, stride, sizeof(float)*4); // Set X offset attribute
    setAttrib(grassShader, "offsetY", 1, stride, sizeof(float)*5); // Set Y offset attribute
    setAttrib(grassShader, "bladeHeight", 1, stride, sizeof(float)*6); // Set blade height attribute
    setAttrib(grassShader, "bladeWidth", 1, stride, sizeof(float)*7); // Set blade width attribute
    setAttrib(grassShader, "colorVar", 1, stride, sizeof(float)*8); // Set color variation attribute
    setAttrib(grassShader, "rotation", 1, stride, sizeof(float)*9); // Set rotation attribute
    // Draw all blades as triangles (instanced rendering).
    glDrawArrays(GL_TRIANGLES, 0, grassCount * 3); // Draw all vertices as triangles
    // Disable all attribute arrays after drawing.
    for (int i = 0; i < 8; ++i) glDisableVertexAttribArray(i); // Disable all attributes
    // Unbind resources to clean up state.
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind vertex buffer
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
#ifdef __APPLE__
    glBindVertexArrayAPPLE(0); // Unbind VAO for macOS
#else
    glBindVertexArray(0); // Unbind VAO for other platforms
#endif
    glUseProgram(0); // Deactivate shader program
}

// grassSystemCleanup: Releases all OpenGL and CPU resources used by the grass system.
// Ensures proper cleanup of buffers, textures, and shaders to prevent memory/resource leaks.
void grassSystemCleanup() {
    // Delete the vertex buffer if it exists.
    if (grassVBO) glDeleteBuffers(1, &grassVBO); // Delete vertex buffer object
    // Delete the vertex array object if it exists.
    if (grassVAO) {
#ifdef __APPLE__
        glDeleteVertexArraysAPPLE(1, &grassVAO); // Delete VAO for macOS
#else
        glDeleteVertexArrays(1, &grassVAO); // Delete VAO for other platforms
#endif
    }
    grassVBO = 0; // Reset handle
    grassVAO = 0; // Reset handle
    grassShader = 0; // Reset handle
    // Delete the grass texture if it exists.
    if (grassTex) glDeleteTextures(1, &grassTex); // Delete texture
    grassTex = 0; // Reset handle
} 