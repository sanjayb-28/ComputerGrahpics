#ifndef SOUND_H
#define SOUND_H

int soundInit(const char* filename);
void soundPlay(void);
void soundStop(void);
void soundCleanup(void);

#endif 