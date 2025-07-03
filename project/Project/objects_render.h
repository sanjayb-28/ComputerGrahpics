#ifndef OBJECTS_RENDER_H
#define OBJECTS_RENDER_H

#include "landscape.h"

typedef struct {
    float x, y, z;
    float scale;
    int depth;
    float rotation;
    unsigned int branchBias;
    int leafColorIndex;
} TreeInstance;

typedef struct {
    float minSlope, maxSlope;
    float minHeight, maxHeight;
    float minDistanceFromWater;
    int density;
} ObjectPlacementParams;

extern TreeInstance* treeInstances;
extern int numTrees;

void freeLandscapeObjects(void);
void initLandscapeObjects(Landscape* landscape);
void renderLandscapeObjects(Landscape* landscape);

#endif