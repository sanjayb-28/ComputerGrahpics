/*
 * Fractal Tree Generation and Rendering System for Boulder Scene
 *
 * This component implements recursive, procedural generation and rendering of fractal trees using OpenGL.
 * It leverages custom shaders, recursive geometry, and efficient foliage rendering to create visually rich,
 * natural-looking trees. The system supports randomized branching, textured bark, and layered leaf clusters.
 *
 * Key Concepts:
 * - Fractal Recursion: Branches are generated recursively, with randomized angles and lengths for realism.
 * - Procedural Variation: Each tree can be seeded for unique structure and color.
 * - Efficient Foliage: Leaves are rendered in clusters/layers for performance, with color and lighting variation.
 * - Shader Integration: Custom GLSL shaders are used for both branches and leaves, supporting lighting and texturing.
 * - Resource Management: Shaders and textures are loaded and used efficiently.
 *
 * Function Roles:
 * - branchRandom: Generates deterministic pseudo-random values for branch variation.
 * - drawCylinderCap/drawCylinderY: Render branch geometry as textured cylinders with end caps.
 * - drawLeafSegment/drawLeafLayer/drawLeafCluster: Render efficient, layered leaf geometry with color and lighting.
 * - drawFractalBranches: Recursively generates the tree structure, switching to leaves at the base case.
 * - setupBranchShaderLighting/setupLeafShaderLighting: Pass OpenGL lighting to shaders.
 * - fractalTreeInit: Loads and initializes shaders for branches and leaves.
 * - fractalTreeDraw: Entry point for drawing a fractal tree at a given position, scale, and seed.
 */

#include "CSCIx229.h"
#include "fractal_tree.h"
#include "shaders.h"

// Shader handles for branches and leaves
static int branchShader = 0;
static int leafShader = 0;
extern GLuint barkTexture;
extern GLuint leafTexture;

// branchRandom: Generates a deterministic pseudo-random float in [-0.5, 0.5] for branch variation.
// Contribution: This function is essential for procedural variation in the fractal tree system. It ensures that each branch can have a unique, but repeatable, random offset in angle or length, based on the recursion depth, branch index, and a global tree seed. This enables every tree to look different while remaining deterministic for a given seed, which is crucial for both realism and reproducibility in procedural content.
static float branchRandom(int depth, int branch, unsigned int treeSeed)
{
    unsigned int seed = (depth * 73856093u) ^ (branch * 19349663u) ^ treeSeed; // Mix depth, branch, and seed for uniqueness
    seed = (seed ^ (seed >> 13)) * 1274126177u; // Further scramble bits for randomness
    return ((seed & 0xFFFF) / 65535.0f) - 0.5f; // Scale to [-0.5, 0.5] for use as an offset
}

// drawCylinderCap: Renders a circular cap (end) for a cylinder at height y.
// Contribution: This function is used to close off the ends of branch cylinders, making the geometry watertight and visually realistic. It is called by drawCylinderY for both the base and tip of each branch segment, ensuring that the tree's branches do not appear hollow when viewed from above or below.
static void drawCylinderCap(double radius, double y, int segments, int normalY) {
    glBegin(GL_TRIANGLE_FAN); // Start drawing a fan of triangles for the cap
    glNormal3d(0, normalY, 0); // Set normal for lighting (up or down depending on cap)
    glVertex3d(0, y, 0); // Center vertex of the cap
    for (int i = 0; i <= segments; ++i) {
        double angle = (normalY > 0 ? -i : i) * 2.0 * M_PI / segments; // Compute angle for this segment
        double x = cos(angle); // X position on circle
        double z = sin(angle); // Z position on circle
        glVertex3d(radius * x, y, radius * z); // Add vertex on circle edge
    }
    glEnd(); // End fan
}

// drawCylinderY: Renders a vertical cylinder (branch) with optional texture and end caps.
// Contribution: This function is the core geometry builder for all branch segments in the tree. It draws a cylinder with a specified base and top radius, applies bark texturing if available, and closes the ends with caps. It is called recursively by drawFractalBranches to build the entire tree skeleton.
static void drawCylinderY(double length, double baseRadius, double topRadius) {
    const int segments = 4; // Number of sides for the cylinder (low for stylized look)
    double angleStep = 2.0 * M_PI / segments; // Angle between segments
    if (barkTexture) {
        glActiveTexture(GL_TEXTURE0); // Activate texture unit 0
        glBindTexture(GL_TEXTURE_2D, barkTexture); // Bind bark texture
        GLint texLoc = glGetUniformLocation(branchShader, "barkTex"); // Get shader uniform location
        glUniform1i(texLoc, 0); // Set uniform to use texture unit 0
        glEnable(GL_TEXTURE_2D); // Enable texturing
    }
    glBegin(GL_TRIANGLE_STRIP); // Start drawing the cylinder sides
    for (int i = 0; i <= segments; ++i) {
        double angle = i * angleStep; // Current angle
        double x = cos(angle); // X position on circle
        double z = sin(angle); // Z position on circle
        glNormal3d(x, 0, z); // Set normal for lighting
        glTexCoord2f(i/(double)segments, 0.0); // Texture coordinate at base
        glVertex3d(baseRadius * x, 0, baseRadius * z); // Vertex at base
        glTexCoord2f(i/(double)segments, 1.0); // Texture coordinate at top
        glVertex3d(topRadius * x, length, topRadius * z); // Vertex at top
    }
    glEnd(); // End cylinder sides
    if (barkTexture) glDisable(GL_TEXTURE_2D); // Disable texturing
    drawCylinderCap(baseRadius, 0, segments, -1); // Draw bottom cap
    drawCylinderCap(topRadius, length, segments, 1); // Draw top cap
}

// drawLeafSegment: Renders a single segment of a leaf layer as a quad strip.
// Contribution: This function is called by drawLeafLayer to build up the geometry for a single horizontal layer of leaves. It adds color, normal, and texture variation for realism, and is designed for efficiency. By using a strip of quads, it avoids the performance cost of rendering thousands of individual leaves, while still providing a lush appearance.
static void drawLeafSegment(int i, int segments, float radius, float y, float layerSpacing, float heightPercent, unsigned int seed, int leafColorIndex) {
    float angle = (float)i / segments * 2.0f * M_PI; // Angle for this segment
    float radiusVar = 1.0f + ((float)rand() / RAND_MAX) * 0.2f - 0.1f; // Slight randomization of radius for natural look
    float x = cosf(angle) * radius * radiusVar; // X position of lower vertex
    float z = sinf(angle) * radius * radiusVar; // Z position of lower vertex
    float nx = cosf(angle); // Normal X
    float ny = 0.7f; // Normal Y (upwards bias for leaf orientation)
    float nz = sinf(angle); // Normal Z
    float nlen = sqrtf(nx*nx + ny*ny + nz*nz); // Normalize normal
    float shade = 0.8f + ((float)rand() / RAND_MAX) * 0.2f; // Random shade for color variation
    glColor3f(0.45f * shade, 0.75f * shade, 0.25f * shade); // Set color for lower vertex
    glNormal3f(nx/nlen, ny/nlen, nz/nlen); // Set normal for lighting
    glTexCoord2f(i/(float)segments, 0.0f); // Texture coordinate at base
    glVertex3f(x, y, z); // Lower vertex
    float upperRadius = radius * (0.95f - heightPercent * 0.1f); // Slightly shrink upper radius for taper
    float x2 = cosf(angle) * upperRadius * radiusVar; // X position upper
    float z2 = sinf(angle) * upperRadius * radiusVar; // Z position upper
    float y2 = y + layerSpacing; // Y position upper
    glColor3f(0.9f * shade, 0.7f * shade, 0.3f * shade); // Set color for upper vertex
    glNormal3f(nx/nlen, ny/nlen, nz/nlen); // Set normal for lighting
    glTexCoord2f(i/(float)segments, 1.0f); // Texture coordinate at top
    glVertex3f(x2, y2, z2); // Upper vertex
}

// drawLeafLayer: Renders a single horizontal layer of leaves as a triangle strip.
// Contribution: This function builds up a full ring of leaves at a given height, calling drawLeafSegment for each segment. It is called multiple times by drawLeafCluster to create a multi-layered, volumetric leaf cluster. This approach balances visual density with rendering efficiency.
static void drawLeafLayer(float y, float layerSpacing, float baseRadius, float heightPercent, int segments, unsigned int seed, int leafColorIndex) {
    float radius = baseRadius * (1.0f - powf(heightPercent - 0.3f, 2.0f)) * 1.8f; // Compute radius for this layer, with a bulge in the middle
    glBegin(GL_TRIANGLE_STRIP); // Start drawing the layer
    for (int i = 0; i <= segments; i++) {
        drawLeafSegment(i, segments, radius, y, layerSpacing, heightPercent, seed, leafColorIndex); // Draw each segment
    }
    glEnd(); // End layer
}

// drawLeafCluster: Renders a full cluster of leaves as multiple stacked layers.
// Contribution: This function is called at the tips of branches to create the tree's foliage. It handles texturing, color, and lighting for the entire cluster, and uses drawLeafLayer to build up a volumetric, efficient representation of a leafy canopy.
static void drawLeafCluster(float height, float baseRadius, int layers, int segments, unsigned int seed, int leafColorIndex) {
    float layerSpacing = height / layers; // Vertical spacing between layers
    float startHeight = 0.0f; // Starting Y position
    srand(seed); // Seed random for repeatable variation
    if (leafTexture) {
        glActiveTexture(GL_TEXTURE0); // Activate texture unit 0
        glBindTexture(GL_TEXTURE_2D, leafTexture); // Bind leaf texture
        GLint texLoc = glGetUniformLocation(leafShader, "leafTex"); // Get shader uniform location
        glUniform1i(texLoc, 0); // Set uniform to use texture unit 0
        glEnable(GL_TEXTURE_2D); // Enable texturing
        GLint colorIndexLoc = glGetUniformLocation(leafShader, "leafColorIndex"); // Get color index uniform
        glUniform1i(colorIndexLoc, leafColorIndex); // Set color index for shader
    }
    for (int layer = 0; layer < layers; layer++) {
        float heightPercent = (float)layer / layers; // Fractional height for this layer
        float y = startHeight + layer * layerSpacing; // Y position for this layer
        drawLeafLayer(y, layerSpacing, baseRadius, heightPercent, segments, seed, leafColorIndex); // Draw the layer
    }
    if (leafTexture) glDisable(GL_TEXTURE_2D); // Disable texturing
}

// drawFractalBranches: Recursively generates and renders the tree's branches and leaves.
// Contribution: This is the heart of the fractal tree system. It recursively builds the tree structure, drawing a branch at each level and splitting into sub-branches. At the base case (depth 0), it draws a leaf cluster. This function enables the self-similar, natural appearance of the tree and allows for deep procedural variation.
static void drawFractalBranches(int depth, double length, double baseRadius, double topRadius, int drawLeaves, unsigned int treeSeed, int leafColorIndex) {
    if (depth == 0) {
        if (drawLeaves) {
            drawLeafCluster(length * 0.8f, 0.5f, 5, 8, treeSeed, leafColorIndex); // Draw leaves at branch tip
        }
        return;
    }
    glColor3f(0.55, 0.27, 0.07); // Set branch color (brown)
    drawCylinderY(length, baseRadius, topRadius); // Draw the branch segment
    glPushMatrix(); // Save current transform
    glTranslated(0, length, 0); // Move to branch tip
    int numBranches = 2; // Number of sub-branches
    double baseAzimuth = branchRandom(depth, 0, treeSeed) * 360.0; // Random base azimuth
    double offset = 90.0 + fabs(branchRandom(depth, 1, treeSeed)) * 90.0; // Random offset
    double azimuths[2] = {baseAzimuth, baseAzimuth + offset}; // Azimuths for sub-branches
    double elevations[2] = {30.0, -30.0}; // Elevations for sub-branches
    for (int i = 0; i < numBranches; ++i) {
        glPushMatrix(); // Save transform for this branch
        glRotated(azimuths[i], 0, 1, 0); // Rotate around Y for azimuth
        glRotated(elevations[i], 1, 0, 0); // Rotate around X for elevation
        drawFractalBranches(depth-1, length*0.7, baseRadius*0.7, topRadius*0.7, drawLeaves, treeSeed, leafColorIndex); // Recurse
        glPopMatrix(); // Restore transform
    }
    glPopMatrix(); // Restore transform
}

// setupBranchShaderLighting: Passes OpenGL light position and color to the branch shader.
// Contribution: This function ensures that the branch shader receives the correct lighting information from the OpenGL context, enabling per-pixel lighting and shading effects on the branches. It is called before rendering branches with the branch shader.
static void setupBranchShaderLighting() {
    float lightPos[4]; // Array for light position
    float diffuse[4]; // Array for diffuse color
    glGetLightfv(GL_LIGHT0, GL_POSITION, lightPos); // Get light position from OpenGL
    glGetLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); // Get diffuse color from OpenGL
    GLint lightColorLoc = glGetUniformLocation(branchShader, "lightColor"); // Shader uniform location
    glUniform3fv(lightColorLoc, 1, diffuse); // Pass diffuse color
    GLint lightPosLoc = glGetUniformLocation(branchShader, "lightPos"); // Shader uniform location
    glUniform3fv(lightPosLoc, 1, lightPos); // Pass light position
}

// setupLeafShaderLighting: Passes OpenGL light position and color to the leaf shader.
// Contribution: This function ensures that the leaf shader receives the correct lighting information from the OpenGL context, enabling per-pixel lighting and shading effects on the leaves. It is called before rendering leaves with the leaf shader.
static void setupLeafShaderLighting() {
    float lightPos[4]; // Array for light position
    float diffuse[4]; // Array for diffuse color
    glGetLightfv(GL_LIGHT0, GL_POSITION, lightPos); // Get light position from OpenGL
    glGetLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); // Get diffuse color from OpenGL
    GLint leafLightColorLoc = glGetUniformLocation(leafShader, "lightColor"); // Shader uniform location
    glUniform3fv(leafLightColorLoc, 1, diffuse); // Pass diffuse color
    GLint leafLightPosLoc = glGetUniformLocation(leafShader, "lightPos"); // Shader uniform location
    glUniform3fv(leafLightPosLoc, 1, lightPos); // Pass light position
}

// fractalTreeInit: Loads and initializes the branch and leaf shaders for the fractal tree system.
// Contribution: This function is called once at startup to load and compile the GLSL shaders for branches and leaves. It ensures that the rendering pipeline is ready for drawing fractal trees with advanced shading and texturing.
void fractalTreeInit() {
    branchShader = loadShader("shaders/tree_branch.vert", "shaders/tree_branch.frag"); // Load branch shader
    leafShader = loadShader("shaders/tree_leaf.vert", "shaders/tree_leaf.frag"); // Load leaf shader
}

// fractalTreeDraw: Entry point for drawing a fractal tree at (x, y, z) with given scale, depth, seed, and color.
// Contribution: This is the main interface for the rest of the application to render a fractal tree. It sets up the model transformation, activates the appropriate shaders, passes lighting information, and draws both the branches and leaves by calling drawFractalBranches twice (once for each shader). This function ties together all the components of the fractal tree system.
void fractalTreeDraw(double x, double y, double z, double scale, int depth, unsigned int treeSeed, int leafColorIndex)
{
    glPushMatrix(); // Save current transform
    glTranslated(x, y, z); // Move to tree base
    glScaled(scale, scale, scale); // Scale the tree
    useShader(branchShader); // Use branch shader
    setupBranchShaderLighting(); // Pass lighting to shader
    drawFractalBranches(depth , 1.0, 0.12, 0.08, 0, treeSeed, leafColorIndex); // Draw branches only
    useShader(0); // Disable shader
    useShader(leafShader); // Use leaf shader
    setupLeafShaderLighting(); // Pass lighting to shader
    drawFractalBranches(depth, 1.0, 0.12, 0.08, 1, treeSeed, leafColorIndex); // Draw leaves only
    useShader(0); // Disable shader
    glPopMatrix(); // Restore transform
}