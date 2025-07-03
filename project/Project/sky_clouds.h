#ifndef ATMOSPHERIC_CLOUDS_H
#define ATMOSPHERIC_CLOUDS_H

typedef struct {
    float posX, posY, posZ;
    float radius;
    float opacity;
} AtmosphericCloud;

typedef struct {
    AtmosphericCloud cloudBank[88];
    int numClouds;
    float baseAltitude;
} AtmosphericCloudSystem;

AtmosphericCloudSystem* atmosphericCloudSystemCreate(float referenceAltitude);
void atmosphericCloudSystemRender(AtmosphericCloudSystem* system);
void atmosphericCloudSystemDestroy(AtmosphericCloudSystem* system);

#endif