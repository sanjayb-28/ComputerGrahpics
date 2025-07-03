/*
 * Original implementation by Sanjay Baskaran for CSCI 5229 Final Project.
 *
 * Shader pipeline set up based on: https://www.lighthouse3d.com/tutorials/glsl-tutorial/
 * 
 */

#version 120

attribute vec3 pos;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
    gl_PointSize = 1.0;
} 