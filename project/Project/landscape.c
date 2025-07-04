/*
 * landscape.c - Procedural Terrain Generation and Management for Boulder Scene
 *
 * This file implements the procedural landscape system for the Boulder-inspired 3D scene.
 * It is responsible for generating, storing, and providing access to the terrain heightmap, normals, and related data.
 *
 * Key Concepts:
 * - Procedural generation: Uses noise functions and algorithms to create a realistic, varied terrain heightmap.
 * - Heightmap: Stores the elevation of the terrain at each grid point, used for rendering, object placement, and collision.
 * - Normals: Computes surface normals for each grid cell, enabling correct lighting and slope calculations.
 * - Integration: The landscape system is used by rendering, object placement, and physics modules to query terrain properties.
 *
 * This file is ideal for demoing procedural terrain generation, heightmap manipulation, and the integration of terrain data in a real-time graphics project.
 */

#include "CSCIx229.h"
#include "landscape.h"

#define HEIGHTMAP_OFFSET_X 53.0f
#define HEIGHTMAP_OFFSET_Z 77.0f

// --- BEGIN DETAILED COMMENTARY FOR landscape.c ---

// lerp_f: Smoothly interpolates between two values using cosine interpolation for natural transitions.
// Used throughout the terrain generation pipeline for blending noise values and colors.
static float lerp_f(float a, float b, float t) {
    // Convert t (0 to 1) to an angle in radians for smooth interpolation.
    float theta = t * 3.1415927f;
    // Use cosine interpolation for a smooth, non-linear blend between a and b.
    float s = (1.0f - cosf(theta)) * 0.5f;
    // Interpolate between a and b using the smooth step value s.
    return a * (1.0f - s) + b * s;
}

// hash2D: Generates a pseudo-random float in [-1, 1] from integer grid coordinates.
// Provides deterministic randomness for procedural noise, ensuring repeatable terrain features.
static float hash2D(int x, int y) {
    // Combine x and y into a single integer using a prime multiplier for better distribution.
    int m = x + y * 71;
    // Bitwise operations to further scramble the bits for pseudo-randomness.
    m = (m << 13) ^ m;
    // The following formula is a classic integer hash, producing a pseudo-random float in [-1, 1].
    return (1.0f - ((m * (m * m * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

// smoothHash2D: Produces a smoothly varying pseudo-random value at a grid point by blending neighboring hashes.
// This is a core building block for generating smooth, natural-looking terrain noise.
static float smoothHash2D(float x, float y) {
    // Compute a weighted sum of hash values at neighboring grid points for smooth interpolation.
    float c = (hash2D(x-1, y-1) + hash2D(x+1, y-1) + hash2D(x-1, y+1) + hash2D(x+1, y+1)) / 16.0f; // Diagonal neighbors, less weight.
    float s = (hash2D(x-1, y) + hash2D(x+1, y) + hash2D(x, y-1) + hash2D(x, y+1)) / 8.0f;           // Adjacent neighbors, medium weight.
    float ctr = hash2D(x, y) / 4.0f;                                                                // Center point, highest weight.
    // Return the weighted sum for a smoothly varying pseudo-random value.
    return c + s + ctr;
}

// interpolatedHash2D: Computes a smoothly interpolated noise value at any (x, y) position.
// Enables continuous, non-repetitive terrain by blending smoothHash2D values across the grid.
static float interpolatedHash2D(float x, float y) {
    // Integer part of x (grid cell coordinate).
    int ix = (int)x;
    // Fractional part of x (distance within cell).
    float fx = x - ix;
    // Integer part of y (grid cell coordinate).
    int iy = (int)y;
    // Fractional part of y (distance within cell).
    float fy = y - iy;
    // Value at (ix, iy)
    float v1 = smoothHash2D(ix, iy);
    // Value at (ix+1, iy)
    float v2 = smoothHash2D(ix + 1, iy);
    // Value at (ix, iy+1)
    float v3 = smoothHash2D(ix, iy + 1);
    // Value at (ix+1, iy+1)
    float v4 = smoothHash2D(ix + 1, iy + 1);
    // Interpolate along x for the bottom edge.
    float i1 = lerp_f(v1, v2, fx);
    // Interpolate along x for the top edge.
    float i2 = lerp_f(v3, v4, fx);
    // Interpolate along y between the two edges for final value.
    return lerp_f(i1, i2, fy);
}

// cubicStep: Performs cubic Hermite interpolation (smoothstep) between two values.
// Used for smooth transitions in color and water shading, avoiding harsh edges.
static float cubicStep(float a, float b, float x) {
    // Normalize x to [0, 1] between a and b.
    float t = (x - a) / (b - a);
    // Clamp t to [0, 1].
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    // Cubic Hermite interpolation (smoothstep).
    return t * t * (3.0f - 2.0f * t);
}

// getLandColors: Defines the canonical RGB colors for different terrain materials (grass, rock, sand, snow).
// Centralizes color definitions for consistent rendering and easy adjustment.
void getLandColors(float* grass, float* lightRock, float* darkRock, float* sand, float* snow) {
    // Assign RGB values for each terrain material type.
    grass[0] = 0.14f; grass[1] = 0.44f; grass[2] = 0.15f;
    lightRock[0] = 0.52f; lightRock[1] = 0.47f; lightRock[2] = 0.41f;
    darkRock[0] = 0.23f; darkRock[1] = 0.21f; darkRock[2] = 0.19f;
    sand[0] = 0.74f; sand[1] = 0.67f; sand[2] = 0.49f;
    snow[0] = 0.96f; snow[1] = 0.96f; snow[2] = 0.96f;
}

// buildHeightField: Generates the procedural heightmap for the landscape using fractal noise.
// This is the heart of terrain generation, combining multiple octaves of noise and applying a slope for realism.
static void buildHeightField(Landscape* land) {
    // Controls how much each octave contributes to the final noise (lower = smoother terrain).
    float persistence = 0.47f;
    // Number of noise octaves to sum for fractal detail.
    int octs = 4;
    // Loop over every row in the heightmap grid.
    for (int z = 0; z < LANDSCAPE_SIZE; z++) {
        // Loop over every column in the heightmap grid.
        for (int x = 0; x < LANDSCAPE_SIZE; x++) {
            float sum = 0, freq = 1.0f, amp = 1.0f, maxAmp = 0; // Initialize noise sum, frequency, amplitude, and normalization factor.
            for(int o = 0; o < octs; o++) { // For each octave...
                // Offset and scale the coordinates for this octave's frequency.
                float xf = ((float)x + HEIGHTMAP_OFFSET_X) * freq / LANDSCAPE_SIZE * 7.0f;
                float zf = ((float)z + HEIGHTMAP_OFFSET_Z) * freq / LANDSCAPE_SIZE * 7.0f;
                // Add the noise value for this octave, scaled by amplitude.
                sum += interpolatedHash2D(xf, zf) * amp;
                // Track the total amplitude for normalization.
                maxAmp += amp;
                // Reduce amplitude for the next octave.
                amp *= persistence;
                // Increase frequency for the next octave (higher detail).
                freq *= 2.0f;
            }
            // Normalize the sum so the result stays in a reasonable range.
            sum = sum / maxAmp;
            // Normalize x to [-1, 1] for east-west slope.
            float xNorm = ((float)x / LANDSCAPE_SIZE - 0.5f) * 2.0f;
            if(xNorm > 0) {
                // Add a slope to the east side of the map for realism.
                float hMult = 1.0f + xNorm * 1.3f;
                // Apply the slope multiplier.
                sum *= hMult;
            } else {
                // Lower the west side for valley effect.
                sum *= 0.6f;
            }
            // Store the final height value in the elevation data.
            land->elevationData[z * LANDSCAPE_SIZE + x] = sum * LANDSCAPE_HEIGHT * 1.18f;
        }
    }
}

// computeNormals: Calculates per-vertex normals for the terrain mesh based on triangle geometry.
// Essential for correct lighting, shading, and slope-based effects in the terrain rendering pipeline.
static void computeNormals(Landscape* land) {
    // Zero out the normals array before accumulating face normals.
    memset(land->normals, 0, land->vertexCount * 3 * sizeof(float));
    // Loop over every triangle in the mesh (each set of 3 indices forms a triangle).
    for (int i = 0; i < land->indexCount; i += 3) {
        // Index of the first vertex of the triangle.
        unsigned int i1 = land->indices[i];
        // Index of the second vertex.
        unsigned int i2 = land->indices[i+1];
        // Index of the third vertex.
        unsigned int i3 = land->indices[i+2];
        // Pointer to the first vertex's position (x, y, z).
        float* v1 = &land->vertices[i1*3];
        // Pointer to the second vertex's position.
        float* v2 = &land->vertices[i2*3];
        // Pointer to the third vertex's position.
        float* v3 = &land->vertices[i3*3];
        // Compute two edge vectors of the triangle: u = v2 - v1, v = v3 - v1.
        float ux = v2[0] - v1[0];
        float uy = v2[1] - v1[1];
        float uz = v2[2] - v1[2];
        float vx = v3[0] - v1[0];
        float vy = v3[1] - v1[1];
        float vz = v3[2] - v1[2];
        // Compute the cross product of the edge vectors to get the face normal.
        float nx = uy*vz - uz*vy;
        float ny = uz*vx - ux*vz;
        float nz = ux*vy - uy*vx;
        // Add the face normal to each vertex normal (accumulating for smooth shading).
        land->normals[i1*3 + 0] += nx;
        land->normals[i1*3 + 1] += ny;
        land->normals[i1*3 + 2] += nz;
        land->normals[i2*3 + 0] += nx;
        land->normals[i2*3 + 1] += ny;
        land->normals[i2*3 + 2] += nz;
        land->normals[i3*3 + 0] += nx;
        land->normals[i3*3 + 1] += ny;
        land->normals[i3*3 + 2] += nz;
    }
    // Normalize all vertex normals to unit length for correct lighting.
    for (int i = 0; i < land->vertexCount; i++) {
        float* n = &land->normals[i*3];
        // Compute the length of the normal vector.
        float len = sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        if (len > 0) {
            // Normalize x, y, z components.
            n[0] /= len;
            n[1] /= len;
            n[2] /= len;
        }
    }
}

// landscapeRender: Renders the terrain mesh with color blending based on slope, height, and weather.
// Integrates procedural color logic, lighting, and OpenGL drawing to visualize the generated landscape.
void landscapeRender(Landscape* land, int weatherType) {
    if (!land) return; // If the landscape pointer is null, do nothing.
    float noSpec[] = {0.0f, 0.0f, 0.0f, 1.0f}; // No specular reflection for terrain (matte look).
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpec); // Set the material's specular property for all faces.
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0f);   // Set the material's shininess (low for rough terrain).
    float grass[3], lightRock[3], darkRock[3], sand[3], snow[3]; // Arrays to hold the base colors for different terrain types.
    getLandColors(grass, lightRock, darkRock, sand, snow); // Fill the color arrays with predefined values.
    glBegin(GL_TRIANGLES); // Begin drawing triangles for the terrain mesh.
    for(int i = 0; i < land->indexCount; i++) { // Loop over every index in the mesh.
        int idx = land->indices[i]; // Get the vertex index for this triangle corner.
        float h = land->vertices[idx * 3 + 1]; // Get the height (y) of this vertex.
        float normY = land->normals[idx * 3 + 1]; // Get the y-component of the normal (used for slope).
        float color[3]; // Array to hold the final color for this vertex.
        if (weatherType == 1) { // If it's winter weather...
            float slope = 1.0f - normY; // Slope is higher when the normal is less vertical.
            float rockFac = (slope - 0.19f) / 0.41f; // Blend factor for snow vs. rock based on slope.
            rockFac = fmax(0.0f, fmin(1.0f, rockFac)); // Clamp blend factor to [0, 1].
            for(int c = 0; c < 3; c++) {
                color[c] = snow[c] * (1.0f - rockFac) + darkRock[c] * rockFac; // Blend snow and rock colors.
            }
        } else { // Otherwise, use fall/normal blending.
            float slope = 1.0f - normY; // Slope is higher when the normal is less vertical.
            float hAboveWater = h - WATER_LEVEL; // Height above water for beach blending.
            float darkFac = (slope - 0.28f) / 0.32f; // Blend factor for light vs. dark rock.
            darkFac = fmax(0.0f, fmin(1.0f, darkFac)); // Clamp to [0, 1].
            float rock[3];
            for(int c=0; c<3; c++) {
                rock[c] = lightRock[c] * (1.0f - darkFac) + darkRock[c] * darkFac; // Blend light and dark rock colors.
            }
            float grassFac = (slope - 0.13f) / 0.23f; // Blend factor for grass vs. rock.
            grassFac = fmax(0.0f, fmin(1.0f, grassFac)); // Clamp to [0, 1].
            float base[3];
            for(int c=0; c<3; c++) {
                base[c] = grass[c] * (1.0f - grassFac) + rock[c] * grassFac; // Blend grass and rock colors.
            }
            float beach = 2.1f; // Height range for beach blending.
            if(hAboveWater < beach && hAboveWater > -1.0f) { // If near the water level...
                float beachFac = hAboveWater / beach; // Blend factor for sand vs. base color.
                beachFac = fmax(0.0f, fmin(1.0f, beachFac)); // Clamp to [0, 1].
                for(int c = 0; c < 3; c++) {
                    color[c] = sand[c] * (1-beachFac) + base[c] * beachFac; // Blend sand and base color.
                }
            } else {
                for(int c = 0; c < 3; c++) {
                    color[c] = base[c]; // Use the base color (grass/rock blend).
                }
            }
        }
        glColor3fv(color); // Set the current color for this vertex.
        glNormal3fv(&land->normals[idx * 3]); // Set the normal for lighting calculations.
        glVertex3fv(&land->vertices[idx * 3]); // Specify the vertex position.
    }
    glEnd(); // End drawing triangles.
}

// fillVerticesAndUVs: Populates the vertex position and texture coordinate arrays for the terrain mesh.
// Converts grid-based heightmap data into world-space geometry and UVs for rendering and texturing.
static void fillVerticesAndUVs(Landscape* land) {
    // Loop over every grid point in the heightmap.
    for (int z = 0; z < LANDSCAPE_SIZE; z++) {
        for (int x = 0; x < LANDSCAPE_SIZE; x++) {
            int idx = z * LANDSCAPE_SIZE + x;
            // Compute world-space x coordinate for this vertex.
            land->vertices[idx*3 + 0] = ((float)x/LANDSCAPE_SIZE - 0.5f) * LANDSCAPE_SCALE;
            // Set y coordinate from the elevation data.
            land->vertices[idx*3 + 1] = land->elevationData[idx];
            // Compute world-space z coordinate for this vertex.
            land->vertices[idx*3 + 2] = ((float)z/LANDSCAPE_SIZE - 0.5f) * LANDSCAPE_SCALE;
            // Set texture coordinate u (horizontal).
            land->texCoords[idx*2 + 0] = (float)x/LANDSCAPE_SIZE;
            // Set texture coordinate v (vertical).
            land->texCoords[idx*2 + 1] = (float)z/LANDSCAPE_SIZE;
        }
    }
}

// fillIndices: Constructs the index buffer for the terrain mesh, defining how vertices form triangles.
// Enables efficient rendering of the landscape as a triangle mesh using OpenGL.
static void fillIndices(Landscape* land) {
    // The grid is LANDSCAPE_SIZE x LANDSCAPE_SIZE, so there are LANDSCAPE_SIZE-1 quads per row/col.
    int grid = LANDSCAPE_SIZE - 1;
    int idx = 0;
    // Loop over every quad in the grid.
    for (int z = 0; z < grid; z++) {
        for (int x = 0; x < grid; x++) {
            // Compute the four corner indices of the quad.
            int tl = z * LANDSCAPE_SIZE + x;       // Top-left
            int tr = tl + 1;                       // Top-right
            int bl = (z + 1) * LANDSCAPE_SIZE + x; // Bottom-left
            int br = bl + 1;                       // Bottom-right
            // First triangle of the quad (tl, bl, tr)
            land->indices[idx++] = tl;
            land->indices[idx++] = bl;
            land->indices[idx++] = tr;
            // Second triangle of the quad (tr, bl, br)
            land->indices[idx++] = tr;
            land->indices[idx++] = bl;
            land->indices[idx++] = br;
        }
    }
}

// landscapeRenderWater: Renders the animated water surface with time-of-day color blending and wave simulation.
// Demonstrates dynamic environmental effects and integrates with the terrain for realism.
void landscapeRenderWater(float waterLevel, Landscape* land, float dayTime) {
    // Set the size of the water plane to match the landscape.
    float waterSize = LANDSCAPE_SCALE;
    // Number of segments for the water mesh (higher = smoother waves).
    int segs = 64;
    // Size of each segment.
    float segSize = waterSize / segs;
    // Specular highlight for water (shiny surface).
    float spec[4] = {1.0f, 1.0f, 1.0f, 0.3f};
    // Enable blending for water transparency.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Compute the time of day as a fraction (0.0 = midnight, 1.0 = next midnight).
    float t = dayTime / 24.0f;
    // Define color stops for water at different times of day.
    const int NUM_COLS = 6;
    float tPoints[6] = {0.0f, 0.25f, 0.4f, 0.6f, 0.75f, 1.0f};
    float cols[6][4] = {
        {0.02f, 0.02f, 0.1f, 0.9f},
        {0.3f, 0.2f, 0.3f, 0.9f},
        {0.2f, 0.3f, 0.5f, 0.9f},
        {0.2f, 0.3f, 0.5f, 0.9f},
        {0.3f, 0.2f, 0.3f, 0.9f},
        {0.02f, 0.02f, 0.1f, 0.9f}
    };
    // Find which two color stops the current time falls between.
    int i;
    for(i = 0; i < NUM_COLS-1; i++) {
        if(t >= tPoints[i] && t <= tPoints[i+1]) break;
    }
    // Compute the blend factor between the two color stops.
    float segPos = (t - tPoints[i]) / (tPoints[i+1] - tPoints[i]);
    float blend = cubicStep(0.0f, 1.0f, segPos);
    float wColor[4];
    // Interpolate between the two color stops for the current water color.
    for(int j = 0; j < 4; j++) {
        wColor[j] = cols[i][j] * (1.0f - blend) + cols[i+1][j] * blend;
    }
    // Get the current time in seconds for animated waves.
    float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    glPushMatrix();
    // Move the water plane to the correct height.
    glTranslatef(0, waterLevel, 0);
    // Set the water's specular properties.
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);
    glBegin(GL_QUADS);
    // Loop over every quad in the water mesh.
    for(int i = -segs/2; i < segs/2; i++) {
        for(int j = -segs/2; j < segs/2; j++) {
            // Compute the four corners of the quad.
            float x1 = i * segSize;
            float x2 = x1 + segSize;
            float z1 = j * segSize;
            float z2 = z1 + segSize;
            // Compute animated wave heights for each corner.
            float waveF = 0.021f;
            float waveA = 0.052f;
            float y1 = sin(x1*waveF + z1*waveF + now) * waveA;
            float y2 = sin(x2*waveF + z1*waveF + now) * waveA;
            float y3 = sin(x2*waveF + z2*waveF + now) * waveA;
            float y4 = sin(x1*waveF + z2*waveF + now) * waveA;
            // Add a small color variation based on wave height for realism.
            float cVar = (y1 + 0.05f) * 0.05f;
            float fColor[4];
            for(int c = 0; c < 4; c++) {
                fColor[c] = wColor[c] + (c < 3 ? cVar : 0);
            }
            // Set the color and draw the quad.
            glColor4fv(fColor);
            glVertex3f(x1, y1, z1);
            glVertex3f(x2, y2, z1);
            glVertex3f(x2, y3, z2);
            glVertex3f(x1, y4, z2);
        }
    }
    glEnd();
    glPopMatrix();
    glDisable(GL_BLEND);
}

// landscapeGetHeight: Returns the interpolated terrain height at any (x, z) world coordinate.
// Enables smooth object placement, collision, and physics by providing continuous height queries.
float landscapeGetHeight(Landscape* land, float x, float z) {
    // Convert world-space x coordinate to normalized grid-space (0 to LANDSCAPE_SIZE-1)
    float nx = (x / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    // Convert world-space z coordinate to normalized grid-space (0 to LANDSCAPE_SIZE-1)
    float nz = (z / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    // Get the integer grid cell coordinates (lower-left corner of the cell)
    int x0 = (int)nx;
    int z0 = (int)nz;
    // Clamp x0 and z0 to valid grid range to avoid out-of-bounds access
    if (x0 < 0) x0 = 0;
    if (z0 < 0) z0 = 0;
    if (x0 >= LANDSCAPE_SIZE-1) x0 = LANDSCAPE_SIZE-2;
    if (z0 >= LANDSCAPE_SIZE-1) z0 = LANDSCAPE_SIZE-2;
    // Compute the fractional part within the grid cell (for interpolation)
    float fx = nx - x0;
    float fz = nz - z0;
    // Sample the four corner heights of the grid cell
    float h00 = land->elevationData[z0 * LANDSCAPE_SIZE + x0];       // Lower-left
    float h10 = land->elevationData[z0 * LANDSCAPE_SIZE + (x0+1)];   // Lower-right
    float h01 = land->elevationData[(z0+1) * LANDSCAPE_SIZE + x0];   // Upper-left
    float h11 = land->elevationData[(z0+1) * LANDSCAPE_SIZE + (x0+1)]; // Upper-right
    // Interpolate along the x direction for the bottom and top edges of the cell
    float h0 = h00 * (1-fx) + h10 * fx; // Bottom edge (z0)
    float h1 = h01 * (1-fx) + h11 * fx; // Top edge (z0+1)
    // Interpolate along the z direction between the two edges for the final height
    return h0 * (1-fz) + h1 * fz; // Bilinear interpolation for smooth height
}

// landscapeDestroy: Frees all memory associated with a Landscape object.
// Ensures proper resource management and prevents memory leaks in the terrain system.
void landscapeDestroy(Landscape* land) {
    // Free all dynamically allocated memory for the landscape.
    if (land) {
        if (land->elevationData) free(land->elevationData); // Free heightmap data.
        if (land->vertices) free(land->vertices);           // Free vertex positions.
        if (land->normals) free(land->normals);             // Free normals.
        if (land->texCoords) free(land->texCoords);         // Free texture coordinates.
        if (land->indices) free(land->indices);             // Free mesh indices.
        free(land);                                         // Free the Landscape struct itself.
    }
}

// landscapeMix: Utility for linear interpolation between two values.
// Used for blending colors, heights, and other properties in the terrain system.
float landscapeMix(float a, float b, float t) {
    // Linear interpolation between a and b by t.
    return a * (1.0f - t) + b * t;
}

// landscapeGetSnowBlend: Calculates a blend factor for snow coverage based on height and slope.
// Supports dynamic snow rendering and seasonal effects in the terrain.
float landscapeGetSnowBlend(float h, float s) {
    // Define the height range where snow starts and ends.
    float snowStart = 13.0f;
    float snowEnd = 24.0f;
    // Maximum slope for snow to accumulate.
    float maxSlope = 0.61f;
    // Add a small noise factor for natural variation.
    float n = sin(h * 0.29f + s * 2.1f) * 0.13f;
    // Compute blend factor based on height.
    float hFac = (h - snowStart) / (snowEnd - snowStart);
    hFac = hFac < 0 ? 0 : (hFac > 1 ? 1 : hFac);
    // Compute blend factor based on slope.
    float sFac = (maxSlope - s) / maxSlope + n;
    sFac = sFac < 0 ? 0 : (sFac > 1 ? 1 : sFac);
    // Return the product for final snow blend.
    return hFac * sFac;
}

// landscapeCreate: Allocates and initializes a new Landscape object, generating all geometry and data.
// Orchestrates the entire procedural terrain pipeline, returning a ready-to-render landscape.
Landscape* landscapeCreate() {
    // Allocate memory for the Landscape struct.
    Landscape* land = (Landscape*)malloc(sizeof(Landscape));
    if (!land) return NULL;
    // Allocate memory for the heightmap, vertices, normals, texture coordinates, and indices.
    land->elevationData = (float*)malloc(sizeof(float) * LANDSCAPE_SIZE * LANDSCAPE_SIZE);
    land->vertices = (float*)malloc(sizeof(float) * LANDSCAPE_SIZE * LANDSCAPE_SIZE * 3);
    land->normals = (float*)malloc(sizeof(float) * LANDSCAPE_SIZE * LANDSCAPE_SIZE * 3);
    land->texCoords = (float*)malloc(sizeof(float) * LANDSCAPE_SIZE * LANDSCAPE_SIZE * 2);
    land->indices = (unsigned int*)malloc(sizeof(unsigned int) * (LANDSCAPE_SIZE - 1) * (LANDSCAPE_SIZE - 1) * 6);
    land->vertexCount = LANDSCAPE_SIZE * LANDSCAPE_SIZE;
    land->indexCount = (LANDSCAPE_SIZE - 1) * (LANDSCAPE_SIZE - 1) * 6;
    // Check for allocation failure and clean up if necessary.
    if (!land->elevationData || !land->vertices || !land->normals || !land->texCoords || !land->indices) {
        landscapeDestroy(land);
        return NULL;
    }
    // Build the procedural heightmap.
    buildHeightField(land);
    // Fill the vertex and texture coordinate arrays.
    fillVerticesAndUVs(land);
    // Fill the index array for mesh triangles.
    fillIndices(land);
    // Compute normals for lighting.
    computeNormals(land);
    // Return the fully constructed landscape.
    return land;
}
// --- END DETAILED COMMENTARY FOR landscape.c ---