// ---------------------------------------------
// landscape.h - Terrain data structures and procedural terrain API
// ---------------------------------------------

#pragma once

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

<<<<<<< Updated upstream
// --- Global scene parameters ---
/* Water level used for rendering and environmental effects */
extern float waterLevel;
=======
#define WATER_LEVEL -4.0f
>>>>>>> Stashed changes

// --- Terrain data structures ---
/* Main landscape structure containing all mesh and terrain data 
   Used to store the procedurally generated terrain */
typedef struct {
    float* elevationData;   // Height values for the terrain
    float* vertices;        // 3D vertex positions (x,y,z triplets)
    float* normals;         // Normal vectors for lighting
    float* texCoords;       // Texture coordinates for material mapping
    unsigned int* indices;  // Triangle indices for rendering
    int vertexCount;        // Total number of vertices
    int indexCount;         // Total number of indices
} Landscape;

/* Utility vector for 3D operations on the landscape */
typedef struct {
    float x, y, z;
} LandscapeVec3;

// --- Rendering resources ---
/* Texture and shader handles used for terrain rendering */
extern GLuint grassTexture;
extern GLuint rockTexture;
extern GLuint sandTexture;
<<<<<<< Updated upstream
extern int terrainShader;
=======
extern GLuint boulderTexture;
extern GLuint barkTexture;
extern GLuint leafTexture;
>>>>>>> Stashed changes

// --- Landscape API ---
/* Core terrain generation and rendering functions */
Landscape* landscapeCreate();                  // Create and initialize terrain
void landscapeGenerateHeightMap(Landscape* landscape);  // Generate elevation data
void landscapeCalculateNormals(Landscape* landscape);   // Compute surface normals
void landscapeRender(Landscape* landscape, int weatherType);  // Render terrain with materials
void landscapeDestroy(Landscape* landscape);    // Free memory resources
float landscapeGetHeight(Landscape* landscape, float x, float z);  // Sample height at any point
void landscapeRenderWater(float waterLevel, Landscape* landscape, float dayTime);  // Render water surface
float landscapeGetSnowBlend(float height, float slope);  // Calculate snow coverage
float landscapeSmoothStep(float edge0, float edge1, float x);  // Smooth transition function

// --- Landscape mesh parameters ---
/* Constants defining the terrain dimensions and scale */
#define LANDSCAPE_SIZE 128       // Grid resolution
#define LANDSCAPE_SCALE 200.0f   // World space width/depth
#define LANDSCAPE_HEIGHT 50.0f   // Maximum terrain