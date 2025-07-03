#ifndef BOULDER_H
#define BOULDER_H

#include "landscape.h"

typedef struct {
    float x, y, z;
    float scale;
    float rotation;
    unsigned int shapeSeed;
    int colorIndex;
} BoulderInstance;

void freeBoulders(void);
void initBoulders(Landscape* landscape);
void renderBoulders(void);
void boulderDraw(float x, float y, float z, float scale, float rotation, unsigned int shapeSeed, int colorIndex);
void boulderShaderInit(void);

#endif