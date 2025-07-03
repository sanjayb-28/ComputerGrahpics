/*
 * Particle system and transform feedback concepts were learned from:
 *   - https://learnopengl.com/In-Practice/2D-Game/Particles
 *   - https://www.ogldev.org/www/tutorial28/tutorial28.html
 *   - https://open.gl/feedback
 *
 * All code is my own original work.
 */

#include "CSCIx229.h"
#include "particles.h"
#include "shaders.h"
#include "landscape.h"

// Claude generated the apple platform specific macros
#ifdef __APPLE__
    #define VAO_BIND(vao) glBindVertexArrayAPPLE(vao)
    #define VAO_UNBIND() glBindVertexArrayAPPLE(0)
    #define VAO_GEN(count, arrays) glGenVertexArraysAPPLE(count, arrays)
    #define VAO_DELETE(count, arrays) glDeleteVertexArraysAPPLE(count, arrays)
    #define POINT_SPRITE_ON() glEnable(GL_POINT_SPRITE)
    #define POINT_SPRITE_OFF() glDisable(GL_POINT_SPRITE)
    #define TF_BEGIN() glBeginTransformFeedbackEXT(GL_POINTS)
    #define TF_END() glEndTransformFeedbackEXT()
    #define RASTER_DISCARD_ON() glEnable(GL_RASTERIZER_DISCARD_EXT)
    #define RASTER_DISCARD_OFF() glDisable(GL_RASTERIZER_DISCARD_EXT)
    #define TF_BIND_BUFFER(buffer) glBindBufferBaseEXT(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, buffer)
    #define TF_UNBIND_BUFFER() glBindBufferBaseEXT(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, 0)
    #define TF_SETUP(shader, count, varyings) glTransformFeedbackVaryingsEXT(shader, count, varyings, GL_INTERLEAVED_ATTRIBS_EXT)
#else
    #define VAO_BIND(vao) glBindVertexArray(vao)
    #define VAO_UNBIND() glBindVertexArray(0)
    #define VAO_GEN(count, arrays) glGenVertexArrays(count, arrays)
    #define VAO_DELETE(count, arrays) glDeleteVertexArrays(count, arrays)
    #define POINT_SPRITE_ON() 
    #define POINT_SPRITE_OFF() 
    #define TF_BEGIN() glBeginTransformFeedback(GL_POINTS)
    #define TF_END() glEndTransformFeedback()
    #define RASTER_DISCARD_ON() glEnable(GL_RASTERIZER_DISCARD)
    #define RASTER_DISCARD_OFF() glDisable(GL_RASTERIZER_DISCARD)
    #define TF_BIND_BUFFER(buffer) glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, buffer)
    #define TF_UNBIND_BUFFER() glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0)
    #define TF_SETUP(shader, count, varyings) glTransformFeedbackVaryings(shader, count, varyings, GL_INTERLEAVED_ATTRIBS)
#endif

static GLuint particleVBOs[2] = {0, 0};
static GLuint particleVAOs[2] = {0, 0};
static GLuint updateShader = 0;
static GLuint renderShader = 0;
static int curSrc = 0;
static GLuint heightmapTex = 0;

#define NUM_PARTICLES 20000
static float cloudHeight = 128.0f;
static float terrainMinX = -LANDSCAPE_SCALE * 0.5f;
static float terrainMaxX = LANDSCAPE_SCALE * 0.5f;
static float terrainMinZ = -LANDSCAPE_SCALE * 0.5f;
static float terrainMaxZ = LANDSCAPE_SCALE * 0.5f;

static GLint timeLoc = -1, dtLoc = -1, cloudHeightLoc = -1, restThresholdLoc = -1;
static GLint landscapeScaleLoc = -1, landscapeSizeLoc = -1;
static GLint terrainMinXLoc = -1, terrainMaxXLoc = -1, terrainMinZLoc = -1, terrainMaxZLoc = -1;
static GLint windLoc = -1, heightmapLoc = -1;

void particleSystemInit(float terrainScale, float terrainHeight) {
    updateShader = loadShader("shaders/particle_update.vert", NULL);
    renderShader = loadShader("shaders/particle_render.vert", "shaders/particle_render.frag");
    
    glBindAttribLocation(updateShader, 0, "pos");
    glBindAttribLocation(updateShader, 1, "vel");
    glBindAttribLocation(updateShader, 2, "restTime");
    glBindAttribLocation(updateShader, 3, "state");
    
    const char* varyings[] = { "outPos", "outVel", "outRestTime", "outState" };
    TF_SETUP(updateShader, 4, varyings);
    glLinkProgram(updateShader);
    
    VAO_GEN(2, particleVAOs);
    glGenBuffers(2, particleVBOs);


    // claude generated this function for random particle placement
    Particle* particles = (Particle*)malloc(NUM_PARTICLES * sizeof(Particle));
    for (int i = 0; i < NUM_PARTICLES; ++i) {
        float x = terrainMinX + ((float)rand() / RAND_MAX) * (terrainMaxX - terrainMinX);
        float z = terrainMinZ + ((float)rand() / RAND_MAX) * (terrainMaxZ - terrainMinZ);
        float y = cloudHeight + ((float)rand() / RAND_MAX) * 20.0f;
        float vx = ((float)rand() / RAND_MAX - 0.5f) * 4.0f;
        float vy = -8.0f - ((float)rand() / RAND_MAX) * 4.0f;
        float vz = ((float)rand() / RAND_MAX - 0.5f) * 4.0f;
        particles[i].x = x; particles[i].y = y; particles[i].z = z;
        particles[i].vx = vx; particles[i].vy = vy; particles[i].vz = vz;
        particles[i].restTime = 0.0f; particles[i].state = 0.0f;
    }
    // claude code ends here
    
    for (int b = 0; b < 2; ++b) {
        VAO_BIND(particleVAOs[b]);
        glBindBuffer(GL_ARRAY_BUFFER, particleVBOs[b]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * sizeof(Particle), particles, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, x));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, vx));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, restTime));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, state));
    }
    VAO_UNBIND();
    free(particles);
}

void particleSystemUpdate(float dt) {
    int src = curSrc;
    int dst = 1 - curSrc;
    
    glUseProgram(updateShader);
    
    if (timeLoc == -1) {
        timeLoc = glGetUniformLocation(updateShader, "time");
        dtLoc = glGetUniformLocation(updateShader, "dt");
        cloudHeightLoc = glGetUniformLocation(updateShader, "cloudHeight");
        restThresholdLoc = glGetUniformLocation(updateShader, "restThreshold");
        landscapeScaleLoc = glGetUniformLocation(updateShader, "landscapeScale");
        landscapeSizeLoc = glGetUniformLocation(updateShader, "landscapeSize");
        terrainMinXLoc = glGetUniformLocation(updateShader, "terrainMinX");
        terrainMaxXLoc = glGetUniformLocation(updateShader, "terrainMaxX");
        terrainMinZLoc = glGetUniformLocation(updateShader, "terrainMinZ");
        terrainMaxZLoc = glGetUniformLocation(updateShader, "terrainMaxZ");
        windLoc = glGetUniformLocation(updateShader, "wind");
        heightmapLoc = glGetUniformLocation(updateShader, "heightmap");
    }
    
    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    glUniform1f(timeLoc, time);
    glUniform1f(dtLoc, dt);
    glUniform1f(cloudHeightLoc, cloudHeight);
    glUniform1f(restThresholdLoc, 5.0f);
    glUniform1f(landscapeScaleLoc, LANDSCAPE_SCALE);
    glUniform1f(landscapeSizeLoc, LANDSCAPE_SIZE);
    glUniform1f(terrainMinXLoc, terrainMinX);
    glUniform1f(terrainMaxXLoc, terrainMaxX);
    glUniform1f(terrainMinZLoc, terrainMinZ);
    glUniform1f(terrainMaxZLoc, terrainMaxZ);
    glUniform2f(windLoc, 1.0f, 0.5f);
    glUniform1i(heightmapLoc, 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightmapTex);
    
    VAO_BIND(particleVAOs[src]);
    TF_BIND_BUFFER(particleVBOs[dst]);
    RASTER_DISCARD_ON();
    TF_BEGIN();
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
    TF_END();
    RASTER_DISCARD_OFF();
    TF_UNBIND_BUFFER();
    VAO_UNBIND();
    glUseProgram(0);
    
    curSrc = dst;
}

void particleSystemRender() {
    glUseProgram(renderShader);
    POINT_SPRITE_ON();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    VAO_BIND(particleVAOs[curSrc]);
    glPointSize(30.0f);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
    VAO_UNBIND();
    
    POINT_SPRITE_OFF();
    glDisable(GL_BLEND);
    glUseProgram(0);
}

void particleSystemCleanup() {
    VAO_DELETE(2, particleVAOs);
    glDeleteBuffers(2, particleVBOs);
}

void particleSystemSetEnabled(int enabled) {
}

void particleSystemUploadHeightmap(float* elevationData) {
    if (!heightmapTex) {
        glGenTextures(1, &heightmapTex);
        glBindTexture(GL_TEXTURE_2D, heightmapTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glBindTexture(GL_TEXTURE_2D, heightmapTex);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 128, 128, 0, GL_RED, GL_FLOAT, elevationData);
}