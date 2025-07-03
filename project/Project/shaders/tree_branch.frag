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

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform sampler2D barkTex;

void main() {
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - WorldPos);
    float intensity = max(dot(N, L), 0.0) * 0.8 + 0.2;
    vec3 color = mix(vec3(0.35, 0.18, 0.05), vec3(0.7, 0.45, 0.18), clamp(Height, 0.0, 1.0));
    vec3 barkColor = texture2D(barkTex, TexCoord).rgb;
    color = mix(color, barkColor, 0.5);
    vec3 ambient = vec3(0.10, 0.10, 0.10);
    gl_FragColor = vec4(ambient + intensity * color * lightColor, 1.0);
} 