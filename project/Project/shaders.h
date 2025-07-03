#ifndef SHADERS_H
#define SHADERS_H

int loadShader(const char* vertexFile, const char* fragmentFile);

void useShader(int shader);

void deleteShader(int shader);

#endif