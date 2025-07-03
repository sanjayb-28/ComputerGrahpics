/*
 * Original implementation by Sanjay Baskaran for CSCI 5229 Final Project.
 *
 * Shader pipeline set up based on: https://www.lighthouse3d.com/tutorials/glsl-tutorial/
 *   
 */

#version 120

varying vec3 Normal;
varying float Height;
varying vec2 TexCoord;
varying vec3 WorldPos;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform int boulderColorIndex;
uniform sampler2D boulderTex;
uniform vec3 lightPos;

void main() {
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - WorldPos);
    float intensity = max(dot(N, L), 0.0) * 0.7 + 0.3;
    vec3 baseColor;
    if (boulderColorIndex == 0) baseColor = vec3(0.25, 0.23, 0.21);
    else if (boulderColorIndex == 1) baseColor = vec3(0.38, 0.36, 0.34);
    else if (boulderColorIndex == 2) baseColor = vec3(0.45, 0.44, 0.42);
    else if (boulderColorIndex == 3) baseColor = vec3(0.32, 0.29, 0.27);
    else baseColor = vec3(0.22, 0.20, 0.18);
    vec3 texColor = texture2D(boulderTex, TexCoord).rgb;
    vec3 color = mix(baseColor, texColor, 0.5);
    vec3 ambient = vec3(0.13, 0.13, 0.13);
    gl_FragColor = vec4(ambient + intensity * color * lightColor, 1.0);
} 