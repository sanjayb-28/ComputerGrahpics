/*
 * Original implementation by Sanjay Baskaran for CSCI 5229 Final Project.
 *
 * Shader pipeline set up based on: https://www.lighthouse3d.com/tutorials/glsl-tutorial/
 * 
 */

#version 120
attribute vec3 position;
attribute float swaySeed;
attribute float offsetX;
attribute float offsetY;
attribute float bladeHeight;
attribute float bladeWidth;
attribute float colorVar;
attribute float rotation;
uniform float time;
uniform float windStrength;
varying float vAlpha;
varying vec3 vNormal;
varying float vColorVar;
varying vec2 vTexCoord;
varying float vColorIndex;
void main() {
    float curve = 0.35 * sin(offsetY / bladeHeight * 1.57);
    float sway = sin(time * 1.5 + position.x * 0.2 + position.z * 0.3 + swaySeed * 6.28) * 0.2 * windStrength;
    float tipFactor = offsetY / bladeHeight;
    float swayX = sway * tipFactor;
    float twist = 0.15 * sin(offsetY * 6.0 / bladeHeight + swaySeed * 3.0);
    vec3 local = vec3(offsetX + twist, offsetY, curve);
    float c = cos(rotation);
    float s = sin(rotation);
    mat3 rotY = mat3(
        c, 0, -s,
        0, 1, 0,
        s, 0,  c
    );
    local = rotY * local;
    vec3 pos = position + local;
    pos.x += swayX;
    vAlpha = 1.0 - abs(sway) * 0.5;
    vNormal = normalize(rotY * vec3(0.0, 1.0, 0.0));
    vColorVar = colorVar;
    // Map vTexCoord for single triangle: (0,0), (1,0), (0.5,1)
    if (abs(offsetX) < 0.001 && abs(offsetY) < 0.001) {
        vTexCoord = vec2(0.0, 0.0); // base left
    } else if (abs(offsetX - bladeWidth) < 0.001 && abs(offsetY) < 0.001) {
        vTexCoord = vec2(1.0, 0.0); // base right
    } else {
        vTexCoord = vec2(0.5, 1.0); // tip
    }
    vColorIndex = floor(colorVar * 4.0 + 2.0) / 4.0;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
} 