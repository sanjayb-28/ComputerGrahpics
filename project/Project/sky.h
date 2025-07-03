#ifndef SKY_H
#define SKY_H

typedef struct {
    float position[3];           
    float size;                  
    float brightness;            
    float color[4];              
} SkyObject;

typedef struct {
    SkyObject sun;               
    SkyObject moon;              
} SkySystem;

void skySystemInitialize(SkySystem* sky);

void skySystemAdvance(SkySystem* sky, float dayTime);

void skySystemRender(SkySystem* sky, float dayTime);

void skySystemApplyLighting(SkySystem* sky);

#endif