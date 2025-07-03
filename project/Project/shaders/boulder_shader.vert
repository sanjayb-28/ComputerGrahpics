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

void main() {
    Normal = normalize(gl_NormalMatrix * gl_Normal);
    Height = gl_Vertex.y;
    TexCoord = gl_MultiTexCoord0.st;
    WorldPos = vec3(gl_ModelViewMatrix * gl_Vertex);
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
} 