/*
 * Grass rendering structure and instancing logic inspired by:
 *   https://learnopengl.com/Advanced-OpenGL/Instancing
 * Implementation, mesh generation, and animation are original.
 */

#include "CSCIx229.h"
#include "grass.h"
#include "shaders.h"

typedef struct {
    float x, y, z;
    float swaySeed;
    float offsetX, offsetY;
    float bladeHeight, bladeWidth;
    float colorVar;
    float rotation;
} GrassVertex;

static GLuint grassVBO = 0, grassVAO = 0;
static GLuint grassShader = 0;
static GLuint grassTex = 0;
static int grassCount = 0;

static float randomFloat(float a, float b) {
    return a + ((float)rand() / RAND_MAX) * (b - a);
}

// Original code based on the isValidTreeLocation function in objects_render.c
static int isValidGrassLocation(Landscape* landscape, float x, float z, float y) {
    if (y < WATER_LEVEL + 0.2f) return 0;
    float nx = (x / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    float nz = (z / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    int ix = (int)nx;
    int iz = (int)nz;
    if (ix < 0) ix = 0;
    if (iz < 0) iz = 0;
    if (ix >= LANDSCAPE_SIZE-1) ix = LANDSCAPE_SIZE-2;
    if (iz >= LANDSCAPE_SIZE-1) iz = LANDSCAPE_SIZE-2;
    float* normal = &landscape->normals[(iz * LANDSCAPE_SIZE + ix) * 3];
    float slope = acosf(fminf(fmaxf(normal[1], -1.0f), 1.0f));
    float slopeDeg = slope * (180.0f / M_PI);
    return slopeDeg <= 32.0f;
}

// Original code based on the initLandscapeObjects function in objects_render.c
static void generateGrassBlade(Landscape* landscape, float areaSize, GrassVertex* verts, int* bladeIdx) {
    float clampFactor = 0.98f;
    float halfScale = areaSize * 0.5f * clampFactor;
    float x = randomFloat(-halfScale, halfScale);
    float z = randomFloat(-halfScale, halfScale);
    float y = landscapeGetHeight(landscape, x, z);
    if (!isValidGrassLocation(landscape, x, z, y)) return;
    float swaySeed = randomFloat(0.0f, 1.0f);
    float bladeHeight = randomFloat(0.7f, 1.5f);
    float bladeWidth = randomFloat(0.05f, 0.13f);
    float colorVar = randomFloat(-0.08f, 0.08f);
    float rotation = randomFloat(0.0f, 2.0f * (float)M_PI);
    int colorIndex = rand() % 4;
    for (int v = 0; v < 3; ++v) {
        GrassVertex vert = {x, y, z, swaySeed, 0, 0, bladeHeight, bladeWidth, colorVar + colorIndex * 0.25f, rotation};
        switch (v) {
            case 0: vert.offsetX = 0; vert.offsetY = 0; break;
            case 1: vert.offsetX = bladeWidth; vert.offsetY = 0; break;
            case 2: vert.offsetX = bladeWidth/2; vert.offsetY = bladeHeight; break;
        }
        verts[(*bladeIdx)++] = vert;
    }
}

static void generateGrassBlades(Landscape* landscape, float areaSize, int numBlades, GrassVertex* data) {
    int bladeIdx = 0;
    for (int i = 0; i < numBlades; ++i) {
        generateGrassBlade(landscape, areaSize, data, &bladeIdx);
    }
}

static void setupGrassGL(GrassVertex* data, int numVerts) {
#ifdef __APPLE__
    glGenVertexArraysAPPLE(1, &grassVAO);
    glBindVertexArrayAPPLE(grassVAO);
#else
    glGenVertexArrays(1, &grassVAO);
    glBindVertexArray(grassVAO);
#endif
    glGenBuffers(1, &grassVBO);
    glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrassVertex) * numVerts, data, GL_STATIC_DRAW);
    grassShader = loadShader("shaders/grass.vert", "shaders/grass.frag");
    grassTex = LoadTexBMP("tex/leaf.bmp");
    free(data);
}

void grassSystemInit(Landscape* landscape, float areaSize, int numBlades) {
    grassCount = numBlades;
    GrassVertex* data = (GrassVertex*)malloc(sizeof(GrassVertex) * 3 * numBlades);
    generateGrassBlades(landscape, areaSize, numBlades, data);
    setupGrassGL(data, grassCount * 3);
}

static void setAttrib(GLuint shader, const char* name, int size, int stride, int offset) {
    GLint loc = glGetAttribLocation(shader, name);
    if (loc >= 0) {
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, size, GL_FLOAT, GL_FALSE, stride, (void*)(size_t)offset);
    }
}

void grassSystemRender(float time, float windStrength, const float sunDir[3], const float ambient[3]) {
    if (!grassShader || !grassVBO || !grassVAO) return;
    glUseProgram(grassShader);
    glUniform1f(glGetUniformLocation(grassShader, "time"), time);
    glUniform1f(glGetUniformLocation(grassShader, "windStrength"), windStrength);
    glUniform3fv(glGetUniformLocation(grassShader, "sunDir"), 1, sunDir);
    glUniform3fv(glGetUniformLocation(grassShader, "ambient"), 1, ambient);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grassTex);
    glUniform1i(glGetUniformLocation(grassShader, "grassTex"), 0);
#ifdef __APPLE__
    glBindVertexArrayAPPLE(grassVAO);
#else
    glBindVertexArray(grassVAO);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
    int stride = sizeof(GrassVertex);
    setAttrib(grassShader, "position", 3, stride, 0);
    setAttrib(grassShader, "swaySeed", 1, stride, sizeof(float)*3);
    setAttrib(grassShader, "offsetX", 1, stride, sizeof(float)*4);
    setAttrib(grassShader, "offsetY", 1, stride, sizeof(float)*5);
    setAttrib(grassShader, "bladeHeight", 1, stride, sizeof(float)*6);
    setAttrib(grassShader, "bladeWidth", 1, stride, sizeof(float)*7);
    setAttrib(grassShader, "colorVar", 1, stride, sizeof(float)*8);
    setAttrib(grassShader, "rotation", 1, stride, sizeof(float)*9);
    glDrawArrays(GL_TRIANGLES, 0, grassCount * 3);
    for (int i = 0; i < 8; ++i) glDisableVertexAttribArray(i);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
#ifdef __APPLE__
    glBindVertexArrayAPPLE(0);
#else
    glBindVertexArray(0);
#endif
    glUseProgram(0);
}

void grassSystemCleanup() {
    if (grassVBO) glDeleteBuffers(1, &grassVBO);
    if (grassVAO) {
#ifdef __APPLE__
        glDeleteVertexArraysAPPLE(1, &grassVAO);
#else
        glDeleteVertexArrays(1, &grassVAO);
#endif
    }
    grassVBO = 0;
    grassVAO = 0;
    grassShader = 0;
    if (grassTex) glDeleteTextures(1, &grassTex);
    grassTex = 0;
} 