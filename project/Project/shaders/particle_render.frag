/*
 * Original implementation by Sanjay Baskaran for CSCI 5229 Final Project.
 *
 * Shader pipeline set up based on: https://www.lighthouse3d.com/tutorials/glsl-tutorial/
 * 
 */

#version 120

// Claude generated the snowflake structure
float simpleSnowflake(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    float r = length(uv), a = atan(uv.y, uv.x);
    float arm = abs(sin(3.0 * a));
    float flake = smoothstep(0.25, 0.22, r) * smoothstep(0.5, 0.2, arm);
    flake += smoothstep(0.08, 0.0, r) * 0.5;
    return clamp(flake, 0.0, 1.0);
}

void main() {
    float flake = simpleSnowflake(gl_PointCoord);
    gl_FragColor = vec4(mix(vec3(0.85, 0.92, 1.0), vec3(1.0), 0.5), flake * 0.85);
} 