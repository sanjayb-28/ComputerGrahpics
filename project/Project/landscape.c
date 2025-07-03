/*
 * Perlin noise implementation and terrain generation concepts adapted from:
 *   - https://www.cs.umd.edu/class/spring2018/cmsc425/Lects/lect13-2d-perlin.pdf
 *   - https://stackoverflow.com/questions/47837968/how-to-make-a-smoother-perlin-noise-generator
 *   - https://github.com/stanislawfortonski/Procedural-Terrain-Generator-OpenGL/tree/master
 *
 * Most landscape components (heightmap generation, normal computation, terrain rendering, 
 * water rendering, height queries) are directly adapted from these resources using Claude
 * to focus development efforts on objects and effects systems.
 *
 * Original contributions: Weather-based color blending between fall and winter seasons.
 */

#include "CSCIx229.h"
#include "landscape.h"

#define HEIGHTMAP_OFFSET_X 53.0f
#define HEIGHTMAP_OFFSET_Z 77.0f

static float lerp_f(float a, float b, float t) {
    float theta = t * 3.1415927f;
    float s = (1.0f - cosf(theta)) * 0.5f;
    return a * (1.0f - s) + b * s;
}

static float hash2D(int x, int y) {
    int m = x + y * 71;
    m = (m << 13) ^ m;
    return (1.0f - ((m * (m * m * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

static float smoothHash2D(float x, float y) {
    float c = (hash2D(x-1, y-1) + hash2D(x+1, y-1) + hash2D(x-1, y+1) + hash2D(x+1, y+1)) / 16.0f;
    float s = (hash2D(x-1, y) + hash2D(x+1, y) + hash2D(x, y-1) + hash2D(x, y+1)) / 8.0f;
    float ctr = hash2D(x, y) / 4.0f;
    return c + s + ctr;
}

static float interpolatedHash2D(float x, float y) {
    int ix = (int)x;
    float fx = x - ix;
    int iy = (int)y;
    float fy = y - iy;
    float v1 = smoothHash2D(ix, iy);
    float v2 = smoothHash2D(ix + 1, iy);
    float v3 = smoothHash2D(ix, iy + 1);
    float v4 = smoothHash2D(ix + 1, iy + 1);
    float i1 = lerp_f(v1, v2, fx);
    float i2 = lerp_f(v3, v4, fx);
    return lerp_f(i1, i2, fy);
}

static float cubicStep(float a, float b, float x) {
    float t = (x - a) / (b - a);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

void getLandColors(float* grass, float* lightRock, float* darkRock, float* sand, float* snow) {
    grass[0] = 0.14f; grass[1] = 0.44f; grass[2] = 0.15f;
    lightRock[0] = 0.52f; lightRock[1] = 0.47f; lightRock[2] = 0.41f;
    darkRock[0] = 0.23f; darkRock[1] = 0.21f; darkRock[2] = 0.19f;
    sand[0] = 0.74f; sand[1] = 0.67f; sand[2] = 0.49f;
    snow[0] = 0.96f; snow[1] = 0.96f; snow[2] = 0.96f;
}

static void buildHeightField(Landscape* land) {
    float persistence = 0.47f;
    int octs = 4;
    for (int z = 0; z < LANDSCAPE_SIZE; z++) {
        for (int x = 0; x < LANDSCAPE_SIZE; x++) {
            float sum = 0, freq = 1.0f, amp = 1.0f, maxAmp = 0;
            for(int o = 0; o < octs; o++) {
                float xf = ((float)x + HEIGHTMAP_OFFSET_X) * freq / LANDSCAPE_SIZE * 7.0f;
                float zf = ((float)z + HEIGHTMAP_OFFSET_Z) * freq / LANDSCAPE_SIZE * 7.0f;
                sum += interpolatedHash2D(xf, zf) * amp;
                maxAmp += amp;
                amp *= persistence;
                freq *= 2.0f;
            }
            sum = sum / maxAmp;
            float xNorm = ((float)x / LANDSCAPE_SIZE - 0.5f) * 2.0f;
            if(xNorm > 0) {
                float hMult = 1.0f + xNorm * 1.3f;
                sum *= hMult;
            } else {
                sum *= 0.6f;
            }
            land->elevationData[z * LANDSCAPE_SIZE + x] = sum * LANDSCAPE_HEIGHT * 1.18f;
        }
    }
}

static void computeNormals(Landscape* land) {
    memset(land->normals, 0, land->vertexCount * 3 * sizeof(float));
    for (int i = 0; i < land->indexCount; i += 3) {
        unsigned int i1 = land->indices[i];
        unsigned int i2 = land->indices[i+1];
        unsigned int i3 = land->indices[i+2];
        float* v1 = &land->vertices[i1*3];
        float* v2 = &land->vertices[i2*3];
        float* v3 = &land->vertices[i3*3];
        float ux = v2[0] - v1[0];
        float uy = v2[1] - v1[1];
        float uz = v2[2] - v1[2];
        float vx = v3[0] - v1[0];
        float vy = v3[1] - v1[1];
        float vz = v3[2] - v1[2];
        float nx = uy*vz - uz*vy;
        float ny = uz*vx - ux*vz;
        float nz = ux*vy - uy*vx;
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
    for (int i = 0; i < land->vertexCount; i++) {
        float* n = &land->normals[i*3];
        float len = sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        if (len > 0) {
            n[0] /= len;
            n[1] /= len;
            n[2] /= len;
        }
    }
}

// Original code: Weather-based color blending between fall and winter seasons
void landscapeRender(Landscape* land, int weatherType) {
    if (!land) return;
    float noSpec[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0f);
    float grass[3], lightRock[3], darkRock[3], sand[3], snow[3];
    getLandColors(grass, lightRock, darkRock, sand, snow);
    glBegin(GL_TRIANGLES);
    // original code
    for(int i = 0; i < land->indexCount; i++) {
        int idx = land->indices[i];
        float h = land->vertices[idx * 3 + 1];
        float normY = land->normals[idx * 3 + 1];
        float color[3];
        if (weatherType == 1) {
            // Original code: Winter snow/rock blending
            float slope = 1.0f - normY;
            float rockFac = (slope - 0.19f) / 0.41f;
            rockFac = fmax(0.0f, fmin(1.0f, rockFac));
            for(int c = 0; c < 3; c++) {
                color[c] = snow[c] * (1.0f - rockFac) + darkRock[c] * rockFac;
            }
        } else {
            float slope = 1.0f - normY;
            float hAboveWater = h - WATER_LEVEL;
            float darkFac = (slope - 0.28f) / 0.32f;
            darkFac = fmax(0.0f, fmin(1.0f, darkFac));
            float rock[3];
            for(int c=0; c<3; c++) {
                rock[c] = lightRock[c] * (1.0f - darkFac) + darkRock[c] * darkFac;
            }
            float grassFac = (slope - 0.13f) / 0.23f;
            grassFac = fmax(0.0f, fmin(1.0f, grassFac));
            float base[3];
            for(int c=0; c<3; c++) {
                base[c] = grass[c] * (1.0f - grassFac) + rock[c] * grassFac;
            }
            float beach = 2.1f;
            if(hAboveWater < beach && hAboveWater > -1.0f) {
                float beachFac = hAboveWater / beach;
                beachFac = fmax(0.0f, fmin(1.0f, beachFac));
                for(int c = 0; c < 3; c++) {
                    color[c] = sand[c] * (1-beachFac) + base[c] * beachFac;
                }
            } else {
                for(int c = 0; c < 3; c++) {
                    color[c] = base[c];
                }
            }
        }
        // end of original code
        glColor3fv(color);
        glNormal3fv(&land->normals[idx * 3]);
        glVertex3fv(&land->vertices[idx * 3]);
    }
    glEnd();
}

static void fillVerticesAndUVs(Landscape* land) {
    for (int z = 0; z < LANDSCAPE_SIZE; z++) {
        for (int x = 0; x < LANDSCAPE_SIZE; x++) {
            int idx = z * LANDSCAPE_SIZE + x;
            land->vertices[idx*3 + 0] = ((float)x/LANDSCAPE_SIZE - 0.5f) * LANDSCAPE_SCALE;
            land->vertices[idx*3 + 1] = land->elevationData[idx];
            land->vertices[idx*3 + 2] = ((float)z/LANDSCAPE_SIZE - 0.5f) * LANDSCAPE_SCALE;
            land->texCoords[idx*2 + 0] = (float)x/LANDSCAPE_SIZE;
            land->texCoords[idx*2 + 1] = (float)z/LANDSCAPE_SIZE;
        }
    }
}

static void fillIndices(Landscape* land) {
    int grid = LANDSCAPE_SIZE - 1;
    int idx = 0;
    for (int z = 0; z < grid; z++) {
        for (int x = 0; x < grid; x++) {
            int tl = z * LANDSCAPE_SIZE + x;
            int tr = tl + 1;
            int bl = (z + 1) * LANDSCAPE_SIZE + x;
            int br = bl + 1;
            land->indices[idx++] = tl;
            land->indices[idx++] = bl;
            land->indices[idx++] = tr;
            land->indices[idx++] = tr;
            land->indices[idx++] = bl;
            land->indices[idx++] = br;
        }
    }
}

void landscapeRenderWater(float waterLevel, Landscape* land, float dayTime) {
    float waterSize = LANDSCAPE_SCALE;
    int segs = 64;
    float segSize = waterSize / segs;
    float spec[4] = {1.0f, 1.0f, 1.0f, 0.3f};
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float t = dayTime / 24.0f;
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
    int i;
    for(i = 0; i < NUM_COLS-1; i++) {
        if(t >= tPoints[i] && t <= tPoints[i+1]) break;
    }
    float segPos = (t - tPoints[i]) / (tPoints[i+1] - tPoints[i]);
    float blend = cubicStep(0.0f, 1.0f, segPos);
    float wColor[4];
    for(int j = 0; j < 4; j++) {
        wColor[j] = cols[i][j] * (1.0f - blend) + cols[i+1][j] * blend;
    }
    float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    glPushMatrix();
    glTranslatef(0, waterLevel, 0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);
    glBegin(GL_QUADS);
    for(int i = -segs/2; i < segs/2; i++) {
        for(int j = -segs/2; j < segs/2; j++) {
            float x1 = i * segSize;
            float x2 = x1 + segSize;
            float z1 = j * segSize;
            float z2 = z1 + segSize;
            float waveF = 0.021f;
            float waveA = 0.052f;
            float y1 = sin(x1*waveF + z1*waveF + now) * waveA;
            float y2 = sin(x2*waveF + z1*waveF + now) * waveA;
            float y3 = sin(x2*waveF + z2*waveF + now) * waveA;
            float y4 = sin(x1*waveF + z2*waveF + now) * waveA;
            float cVar = (y1 + 0.05f) * 0.05f;
            float fColor[4];
            for(int c = 0; c < 4; c++) {
                fColor[c] = wColor[c] + (c < 3 ? cVar : 0);
            }
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

float landscapeGetHeight(Landscape* land, float x, float z) {
    float nx = (x / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    float nz = (z / LANDSCAPE_SCALE + 0.5f) * (LANDSCAPE_SIZE - 1);
    int x0 = (int)nx;
    int z0 = (int)nz;
    if (x0 < 0) x0 = 0;
    if (z0 < 0) z0 = 0;
    if (x0 >= LANDSCAPE_SIZE-1) x0 = LANDSCAPE_SIZE-2;
    if (z0 >= LANDSCAPE_SIZE-1) z0 = LANDSCAPE_SIZE-2;
    float fx = nx - x0;
    float fz = nz - z0;
    float h00 = land->elevationData[z0 * LANDSCAPE_SIZE + x0];
    float h10 = land->elevationData[z0 * LANDSCAPE_SIZE + (x0+1)];
    float h01 = land->elevationData[(z0+1) * LANDSCAPE_SIZE + x0];
    float h11 = land->elevationData[(z0+1) * LANDSCAPE_SIZE + (x0+1)];
    float h0 = h00 * (1-fx) + h10 * fx;
    float h1 = h01 * (1-fx) + h11 * fx;
    return h0 * (1-fz) + h1 * fz;
}

void landscapeDestroy(Landscape* land) {
    if (land) {
        if (land->elevationData) free(land->elevationData);
        if (land->vertices) free(land->vertices);
        if (land->normals) free(land->normals);
        if (land->texCoords) free(land->texCoords);
        if (land->indices) free(land->indices);
        free(land);
    }
}

float landscapeMix(float a, float b, float t) {
    return a * (1.0f - t) + b * t;
}

float landscapeGetSnowBlend(float h, float s) {
    float snowStart = 13.0f;
    float snowEnd = 24.0f;
    float maxSlope = 0.61f;
    float n = sin(h * 0.29f + s * 2.1f) * 0.13f;
    float hFac = (h - snowStart) / (snowEnd - snowStart);
    hFac = hFac < 0 ? 0 : (hFac > 1 ? 1 : hFac);
    float sFac = (maxSlope - s) / maxSlope + n;
    sFac = sFac < 0 ? 0 : (sFac > 1 ? 1 : sFac);
    return hFac * sFac;
}

Landscape* landscapeCreate() {
    Landscape* land = (Landscape*)malloc(sizeof(Landscape));
    if (!land) return NULL;
    land->elevationData = (float*)malloc(sizeof(float) * LANDSCAPE_SIZE * LANDSCAPE_SIZE);
    land->vertices = (float*)malloc(sizeof(float) * LANDSCAPE_SIZE * LANDSCAPE_SIZE * 3);
    land->normals = (float*)malloc(sizeof(float) * LANDSCAPE_SIZE * LANDSCAPE_SIZE * 3);
    land->texCoords = (float*)malloc(sizeof(float) * LANDSCAPE_SIZE * LANDSCAPE_SIZE * 2);
    land->indices = (unsigned int*)malloc(sizeof(unsigned int) * (LANDSCAPE_SIZE - 1) * (LANDSCAPE_SIZE - 1) * 6);
    land->vertexCount = LANDSCAPE_SIZE * LANDSCAPE_SIZE;
    land->indexCount = (LANDSCAPE_SIZE - 1) * (LANDSCAPE_SIZE - 1) * 6;
    if (!land->elevationData || !land->vertices || !land->normals || !land->texCoords || !land->indices) {
        landscapeDestroy(land);
        return NULL;
    }
    buildHeightField(land);
    fillVerticesAndUVs(land);
    fillIndices(land);
    computeNormals(land);
    return land;
}