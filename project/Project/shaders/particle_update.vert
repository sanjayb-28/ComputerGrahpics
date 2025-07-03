/*
 * Original implementation by Sanjay Baskaran for CSCI 5229 Final Project.
 *
 * Shader pipeline set up based on: https://www.lighthouse3d.com/tutorials/glsl-tutorial/
 * 
 */

#version 120

attribute vec3 pos, vel;
attribute float restTime, state;

uniform float dt, time, cloudHeight, landscapeScale, landscapeSize;
uniform sampler2D heightmap;
uniform vec2 wind;
uniform float terrainMinX, terrainMaxX, terrainMinZ, terrainMaxZ;

varying vec3 outPos, outVel;
varying float outRestTime, outState;

float getTerrainHeight(float x, float z) {
    vec2 uv = vec2((x / landscapeScale + 0.5) * (landscapeSize - 1.0), 
                   (z / landscapeScale + 0.5) * (landscapeSize - 1.0)) / (landscapeSize - 1.0);
    return texture2D(heightmap, uv).r;
}

void main() {
    vec3 newPos = pos, newVel = vel;
    float newRestTime = restTime, newState = state;
    float terrainY = getTerrainHeight(pos.x, pos.z);
    float margin = 1.0;

    if (state < 0.5) {
        newVel = vec3(wind.x, vel.y, wind.y);
        newPos = pos + newVel * dt;
        newPos.x = clamp(newPos.x, terrainMinX + margin, terrainMaxX - margin);
        newPos.z = clamp(newPos.z, terrainMinZ + margin, terrainMaxZ - margin);
        
        if (newPos.y <= terrainY) {
            newPos.y = terrainY;
            newVel = vec3(0.0);
            newRestTime = 0.0;
            newState = 1.0;
        }
    } else {
        newRestTime = restTime + dt;
        newVel = vec3(0.0);
        
        if (newRestTime > 5.0) {
            // Claude helped me generate the hash function
            float rx = terrainMinX + margin + (terrainMaxX - terrainMinX - 2.0 * margin) * fract(sin(pos.x * 12.9898) * 43758.5453);
            float rz = terrainMinZ + margin + (terrainMaxZ - terrainMinZ - 2.0 * margin) * fract(sin(pos.z * 78.233) * 43758.5453);
            newPos = vec3(rx, cloudHeight, rz);
            newVel = vec3(0.0, -10.0, 0.0);
            newRestTime = 0.0;
            newState = 0.0;
        }
    }

    outPos = newPos;
    outVel = newVel;
    outRestTime = newRestTime;
    outState = newState;
    gl_Position = vec4(newPos, 1.0);
}