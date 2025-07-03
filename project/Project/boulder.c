/*
 * Shader pipeline and OpenGL integration based on:
 *   - https://www.lighthouse3d.com/tutorials/glsl-tutorial/
 *   - https://learnopengl.com/shaders
 *   - Real-Time Rendering, polygonal techniques
 *
 * Copilot was used to generate mesh data arrays.
 * All other code is my own original work, except the section marked as AI which is specifically for the boulder structure.
 */
 
#include "CSCIx229.h"
#include "boulder.h"
#include "objects_render.h"
#include "landscape.h"
#include "shaders.h"

#define NUM_BOULDERS 50

static BoulderInstance* boulders = NULL;
static int numBoulders = 0;
static int boulderShader = 0;

extern TreeInstance* treeInstances;
extern int numTrees;
extern GLuint boulderTexture;

static float randf() { return rand() / (float)RAND_MAX; }

void freeBoulders() {
    if (boulders) {
        free(boulders);
        boulders = NULL;
        numBoulders = 0;
    }
}

static int boulderCollides(float x, float z, float minDist) {
    for (int t = 0; t < numTrees; ++t) {
        float dx = x - treeInstances[t].x;
        float dz = z - treeInstances[t].z;
        float dist2 = dx*dx + dz*dz;
        float minTreeDist = minDist + treeInstances[t].scale * 0.5f;
        if (dist2 < minTreeDist * minTreeDist) {
            return 1;
        }
    }
    return 0;
}

static float boulderRandomScale() {
    float a = randf();
    float b = randf();
    float c = randf();
    return 1.2f + a * 2.8f + b * c * 1.2f;
}

// Claude generated this
static float boulderNoise(unsigned int shapeSeed, int i, int j) {
    return (sinf(shapeSeed * 0.13f + i * 1.7f + j * 2.3f) + cosf(shapeSeed * 0.21f + i * 2.1f + j * 1.3f)) * 0.18f * randf();
}

static void boulderVertexNoise(float verts[28][3], const float baseVerts[28][3], unsigned int shapeSeed) {
    for (int i = 0; i < 28; ++i) {
        for (int j = 0; j < 3; ++j) {
            verts[i][j] = baseVerts[i][j] + boulderNoise(shapeSeed, i, j);
        }
    }
}
// end of Claude generated code

static const float baseVerts[28][3] = {
    {0.0f, 1.0f, 0.0f}, {0.8f, 0.6f, 0.2f}, {0.5f, 0.5f, -0.9f}, {-0.7f, 0.7f, -0.6f},
    {-1.0f, 0.5f, 0.4f}, {0.0f, -0.1f, 1.1f}, {1.1f, -0.2f, -0.3f}, {0.4f, -0.8f, -1.0f},
    {-0.8f, -0.6f, -0.8f}, {-1.0f, -0.7f, 0.6f}, {0.6f, 0.2f, 0.8f}, {-0.5f, 0.1f, 1.0f},
    {1.0f, 0.1f, 0.5f}, {1.2f, -0.5f, 0.2f}, {0.7f, -0.7f, 0.7f}, {-0.2f, -1.0f, 0.2f},
    {-0.9f, -0.9f, -0.2f}, {-0.3f, -0.8f, 0.9f}, {0.3f, 0.7f, 0.7f}, {0.9f, -0.3f, 0.9f},
    {-0.6f, 0.3f, 1.0f}, {1.1f, 0.3f, -0.7f}, {-1.1f, 0.2f, -0.5f}, {0.2f, -0.9f, -0.7f},
    {-0.7f, -0.8f, 0.3f}, {0.8f, -0.7f, -0.6f}, {-0.3f, 0.9f, 0.2f}, {0.5f, -0.5f, 1.0f}
};

static const int faces[52][3] = {
    {0,1,2},{0,2,3},{0,3,4},{0,4,1},{1,10,12},{1,12,2},{2,12,7},{2,7,3},{3,7,8},{3,8,4},{4,8,9},{4,9,1},
    {1,9,11},{1,11,10},{5,10,11},{5,11,9},{5,9,8},{5,8,7},{5,7,13},{5,13,14},{5,14,10},{10,14,12},{12,14,13},{12,13,7},
    {6,12,13},{6,13,7},{6,7,2},{6,2,12},{6,12,10},{6,10,15},{6,15,16},{6,16,7},{17,18,19},{17,19,20},{17,20,21},{17,21,18},
    {18,22,23},{18,23,19},{19,23,24},{19,24,20},{20,24,25},{20,25,21},{21,25,26},{21,26,18},{18,26,22},{22,26,25},{22,25,23},{23,25,24}
};

static void boulderShaderUniforms(int colorIndex) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, boulderTexture);
    GLint texLoc = glGetUniformLocation(boulderShader, "boulderTex");
    glUniform1i(texLoc, 0);
    glEnable(GL_TEXTURE_2D);
    float lightPos[4], diffuse[4];
    glGetLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glGetLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    GLint lightColorLoc = glGetUniformLocation(boulderShader, "lightColor");
    glUniform3fv(lightColorLoc, 1, diffuse);
    GLint colorIndexLoc = glGetUniformLocation(boulderShader, "boulderColorIndex");
    glUniform1i(colorIndexLoc, colorIndex);
    GLint lightPosLoc = glGetUniformLocation(boulderShader, "lightPos");
    glUniform3fv(lightPosLoc, 1, lightPos);
}

// Original code based on the initLandscapeObjects function in objects_render.c
static int isValidBoulderLocation(Landscape* landscape, float x, float z, float y) {
    float nx = (x / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    float nz = (z / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    int ix = (int)nx;
    int iz = (int)nz;
    if (ix < 0) ix = 0;
    if (iz < 0) iz = 0;
    if (ix >= LANDSCAPE_SIZE-1) ix = LANDSCAPE_SIZE-2;
    if (iz >= LANDSCAPE_SIZE-1) iz = LANDSCAPE_SIZE-2;
    int idx = iz * LANDSCAPE_SIZE + ix;
    float* n = &landscape->normals[idx*3];
    float slope = acosf(fminf(fmaxf(n[1], -1.0f), 1.0f)) / (float)M_PI;
    if (slope > 0.25f) return 0;
    if (y < WATER_LEVEL + 0.5f) return 0;
    if (boulderCollides(x, z, 4.0f)) return 0;
    return 1;
}

// Original code based on the initLandscapeObjects function in objects_render.c
static int generateRandomBoulder(Landscape* landscape, BoulderInstance* outBoulder) {
    float halfScale = LANDSCAPE_SCALE * 0.5f * 0.95f;
    float x = -halfScale + randf() * LANDSCAPE_SCALE * 0.95f;
    float z = -halfScale + randf() * LANDSCAPE_SCALE * 0.95f;
    float y = landscapeGetHeight(landscape, x, z);
    if (!isValidBoulderLocation(landscape, x, z, y)) return 0;
    float scale = boulderRandomScale();
    float rotation = randf() * 360.0f;
    unsigned int shapeSeed = rand();
    int colorIndex = rand() % 8;
    *outBoulder = (BoulderInstance){x, y, z, scale, rotation, shapeSeed, colorIndex};
    return 1;
}

void initBoulders(Landscape* landscape) {
    freeBoulders();
    if (!landscape) return;
    boulders = (BoulderInstance*)malloc(sizeof(BoulderInstance) * NUM_BOULDERS);
    numBoulders = 0;
    int attempts = 0;
    while (numBoulders < NUM_BOULDERS && attempts < NUM_BOULDERS * 10) {
        attempts++;
        BoulderInstance b;
        if (generateRandomBoulder(landscape, &b)) {
            boulders[numBoulders++] = b;
        }
    }
}

// Claude generated this, code and implementation idea for the boulder shape given by claude
static void computeNormal(const float* v0, const float* v1, const float* v2, float* nx, float* ny, float* nz) {
    float ux = v1[0] - v0[0], uy = v1[1] - v0[1], uz = v1[2] - v0[2];
    float vx = v2[0] - v0[0], vy = v2[1] - v0[1], vz = v2[2] - v0[2];
    *nx = uy * vz - uz * vy;
    *ny = uz * vx - ux * vz;
    *nz = ux * vy - uy * vx;
    float len = sqrtf((*nx)*(*nx) + (*ny)*(*ny) + (*nz)*(*nz));
    if (len > 0.0001f) {
        *nx /= len; *ny /= len; *nz /= len;
    }
}

static void drawBoulderFace(const float verts[28][3], const int face[3]) {
    float* v0 = (float*)verts[face[0]];
    float* v1 = (float*)verts[face[1]];
    float* v2 = (float*)verts[face[2]];
    float nx, ny, nz;
    computeNormal(v0, v1, v2, &nx, &ny, &nz);
    glNormal3f(nx, ny, nz);
    glTexCoord2f(v0[0]*0.5f+0.5f, v0[2]*0.5f+0.5f); glVertex3fv(v0);
    glTexCoord2f(v1[0]*0.5f+0.5f, v1[2]*0.5f+0.5f); glVertex3fv(v1);
    glTexCoord2f(v2[0]*0.5f+0.5f, v2[2]*0.5f+0.5f); glVertex3fv(v2);
}
// end of Claude generated code

static void drawBoulderMesh(const float verts[28][3], const int faces[52][3]) {
    glBegin(GL_TRIANGLES);
    for (int f = 0; f < 52; ++f) {
        drawBoulderFace(verts, faces[f]);
    }
    glEnd();
}

static void setupBoulderTransform(float x, float y, float z, float scale, float rotation) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotation, 0, 1, 0);
    glScalef(scale, scale, scale);
}

static void cleanupBoulderDraw() {
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void boulderDraw(float x, float y, float z, float scale, float rotation, unsigned int shapeSeed, int colorIndex) {
    float verts[28][3];
    boulderVertexNoise(verts, baseVerts, shapeSeed);

    setupBoulderTransform(x, y, z, scale, rotation);

    if (boulderShader) {
        useShader(boulderShader);
        boulderShaderUniforms(colorIndex);
    }

    drawBoulderMesh(verts, faces);

    if (boulderShader) useShader(0);

    cleanupBoulderDraw();
}

void renderBoulders() {
    for (int i = 0; i < numBoulders; ++i) {
        BoulderInstance* b = &boulders[i];
        boulderDraw(b->x, b->y, b->z, b->scale, b->rotation, b->shapeSeed, b->colorIndex);
    }
}

void boulderShaderInit() {
    boulderShader = loadShader("shaders/boulder_shader.vert", "shaders/boulder_shader.frag");
} 