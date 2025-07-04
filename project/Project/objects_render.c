/*
 * objects_render.c - Modular Object Rendering System for Boulder Scene
 *
 * This file is responsible for rendering all procedural and placed objects in the Boulder-inspired 3D scene.
 * It provides functions to draw trees, rocks, shrubs, logs, and other landscape features, using OpenGL for real-time rendering.
 *
 * Key Concepts:
 * - Modular rendering: Each object type (tree, rock, shrub, log) has its own rendering function, making the system extensible and easy to maintain.
 * - OpenGL pipeline: Uses immediate mode (glBegin/glEnd) and modern OpenGL state management to draw geometry, set colors, and apply lighting.
 * - Integration: This module is called by the main scene rendering loop, and is responsible for drawing all non-terrain, non-sky objects.
 * - Performance: Designed for clarity and modularity, but can be optimized for batching or instancing if needed.
 *
 * This file is ideal for demoing modular graphics code, OpenGL rendering techniques, and the integration of procedural and placed objects in a real-time scene.
 */

#include "CSCIx229.h"
#include "objects_render.h"
#include "fractal_tree.h"
#include "boulder.h"

TreeInstance* treeInstances = NULL;
int numTrees = 0;

extern float treeSwayAngle;

// Claude generated this function, because i had no clue how to get slope at a point
static float getSlopeAt(Landscape* landscape, float x, float z) {
    // 1. Calculate normalized coordinates for landscape grid lookup
    // This scales the world coordinates (x, z) to the grid's index range.
    // LANDSCAPE_SCALE is the size of each grid cell in the world.
    // +0.5f centers the grid, so (x/LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1)
    // gives the index of the grid cell closest to (x, z).
    float nx = (x / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    float nz = (z / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);

    // 2. Convert normalized coordinates to integer indices
    // Clamp indices to ensure they are within the valid range of the landscape grid.
    int ix = (int)nx;
    int iz = (int)nz;
    if (ix < 0) ix = 0;
    if (iz < 0) iz = 0;
    if (ix >= LANDSCAPE_SIZE-1) ix = LANDSCAPE_SIZE-2;
    if (iz >= LANDSCAPE_SIZE-1) iz = LANDSCAPE_SIZE-2;

    // 3. Calculate the index in the landscape->normals array
    // The normals are stored in a 1D array, so we need to calculate the 1D index
    // from the 2D grid coordinates (ix, iz).
    int idx = iz * LANDSCAPE_SIZE + ix;

    // 4. Access the normal vector at the calculated index
    // The normals are stored as a 3-component vector (nx, ny, nz).
    // We only need the y-component for slope calculation.
    float* n = &landscape->normals[idx*3];

    // 5. Calculate the slope angle in radians
    // The normal vector's y-component represents the vertical slope.
    // acosf(fminf(fmaxf(n[1], -1.0f), 1.0f)) converts the y-component to an angle.
    // fminf(fmaxf(n[1], -1.0f), 1.0f) clamps the y-component between -1 and 1.
    // acosf(y) gives the angle in radians where y is the cosine of the angle.
    float slope = acosf(fminf(fmaxf(n[1], -1.0f), 1.0f));

    // 6. Convert slope angle to a normalized value (0 to 1)
    // This is done by dividing the angle in radians by pi.
    // The slope is typically between 0 and 1, where 0 is flat and 1 is steep.
    return slope / (float)M_PI;
}
// Claude code ends here

static int isValidTreeLocation(Landscape* landscape, float x, float z, const ObjectPlacementParams* params) {
    float y = landscapeGetHeight(landscape, x, z); // Get the height of the landscape at the (x, z) position.
    float slope = getSlopeAt(landscape, x, z);     // Get the normalized slope at the (x, z) position.
    if (y < params->minHeight) return 0;           // Reject locations below the minimum allowed height (e.g., underwater or too low).
    if (y > params->maxHeight) return 0;           // Reject locations above the maximum allowed height (e.g., mountain tops).
    if (slope < params->minSlope || slope > params->maxSlope) return 0; // Reject locations that are too flat or too steep for trees.
    if (y - WATER_LEVEL < params->minDistanceFromWater) return 0; // Reject locations too close to water (e.g., to avoid trees in lakes).
    return 1; // If all checks pass, this is a valid location for a tree.
}

static TreeInstance makeRandomTreeInstance(float x, float y, float z) {
    float scale = 1.8f + (rand()/(float)RAND_MAX) * 2.2f; // Randomize the tree's scale for natural size variation.
    int depth = 4 + rand() % 2;                           // Randomize the recursion depth for branch complexity.
    float rotation = (rand()/(float)RAND_MAX) * 360.0f;   // Randomize the tree's rotation for orientation diversity.
    unsigned int branchBias = rand();                     // Randomize the branch bias for unique branch shapes.
    int leafColorIndex = rand() % 5;                      // Randomize the leaf color index for seasonal/color variety.
    // Return a fully initialized TreeInstance struct with all randomized and provided parameters.
    return (TreeInstance){x, y, z, scale, depth, rotation, branchBias, leafColorIndex};
}

void freeLandscapeObjects() {
    // This function frees all dynamically allocated memory for tree instances.
    // It is called before reinitializing or when cleaning up the scene to avoid memory leaks.
    if (treeInstances) {                 // If the treeInstances array has been allocated...
        free(treeInstances);             // Free the memory for all tree instances.
        treeInstances = NULL;            // Set the pointer to NULL to avoid dangling references.
        numTrees = 0;                    // Reset the tree count to zero.
    }
}

void initLandscapeObjects(Landscape* landscape) {
    freeLandscapeObjects();              // Always free any existing objects before initializing new ones.
    if (!landscape) return;              // If the landscape is not valid, do nothing.
    // Set up the parameters for tree placement. These control where trees can be placed in the landscape.
    ObjectPlacementParams treeParams = {
        .minSlope = 0.0f,               // Minimum slope for tree placement (flat ground).
        .maxSlope = 0.35f,              // Maximum slope for tree placement (avoid steep hills).
        .minHeight = WATER_LEVEL + 1.5f,// Minimum height above water for tree placement.
        .maxHeight = LANDSCAPE_HEIGHT * 1.2f, // Maximum height for tree placement.
        .minDistanceFromWater = 1.0f,   // Minimum distance from water for tree placement.
        .density = 15                   // Number of grid cells along one axis (controls total tree density).
    };
    int grid = treeParams.density;      // The number of grid cells along one axis.
    int maxTrees = grid * grid;         // The maximum number of trees (one per grid cell).
    treeInstances = (TreeInstance*)malloc(sizeof(TreeInstance) * maxTrees); // Allocate memory for all possible tree instances.
    numTrees = 0;                       // Start with zero trees; we'll increment as we place them.
    float halfScale = LANDSCAPE_SCALE * 0.5f * 0.95f; // Half the landscape width, slightly reduced to avoid edge artifacts.
    float step = (LANDSCAPE_SCALE * 0.95f) / (float)grid; // Step size between grid cells, covering most of the landscape.
    // Place trees in a grid, but add random jitter to each position for natural distribution.
    for (int i = 0; i < grid; ++i) {
        for (int j = 0; j < grid; ++j) {
            float x = -halfScale + i * step + (rand()/(float)RAND_MAX - 0.5f) * step * 0.5f; // X position with random jitter.
            float z = -halfScale + j * step + (rand()/(float)RAND_MAX - 0.5f) * step * 0.5f; // Z position with random jitter.
            if (!isValidTreeLocation(landscape, x, z, &treeParams)) continue; // Skip if this location is not valid for a tree.
            float y = landscapeGetHeight(landscape, x, z); // Get the Y (height) at this position.
            treeInstances[numTrees++] = makeRandomTreeInstance(x, y, z); // Create and store a new tree instance at this location.
        }
        // claude code ends here
    }
}

static void renderTreeInstance(const TreeInstance* t) {
    glPushMatrix(); // Save the current transformation matrix. This allows us to apply local transformations for this tree only.
    glTranslatef(t->x, t->y, t->z); // Move the origin to the tree's position in world space (x, y, z).
    glRotatef(t->rotation, 0, 1, 0); // Rotate the tree around the Y axis for random orientation.
    float sway = treeSwayAngle + (t->branchBias % 360) * 0.01f; // Calculate the sway angle for this tree, adding a unique bias for variety.
    glRotatef(sway, 0, 0, 1); // Apply the sway as a rotation around the Z axis to animate the tree in the wind.
    fractalTreeDraw(0, 0, 0, t->scale, t->depth, t->branchBias, t->leafColorIndex); // Draw the tree using the fractal tree algorithm, with all its parameters.
    glPopMatrix(); // Restore the previous transformation matrix so subsequent objects are not affected.
}

void renderLandscapeObjects(Landscape* landscape) {
    if (!landscape || !treeInstances) return; // If there is no landscape or no trees, do nothing.
    for (int i = 0; i < numTrees; ++i) { // Loop through all tree instances.
        renderTreeInstance(&treeInstances[i]); // Render each tree at its unique position, scale, and orientation.
    }
    renderBoulders(); // Render all boulders in the scene (other object types can be added here as needed).
} 