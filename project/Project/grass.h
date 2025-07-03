#pragma once
#include "landscape.h"

void grassSystemInit(Landscape* landscape, float areaSize, int numBlades);
void grassSystemRender(float time, float windStrength, const float sunDir[3], const float ambient[3]);
void grassSystemCleanup(); 