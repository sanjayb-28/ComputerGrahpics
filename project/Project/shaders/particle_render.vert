/*
 * Particle Render Vertex Shader - Weather Particle Rendering
 *
 * This vertex shader handles the rendering of weather particles (snow/rain) in the scene.
 * It processes particle positions and transforms them for screen-space rendering.
 * The shader is designed for point sprite rendering, where each particle is rendered
 * as a single point that gets expanded into a sprite in the fragment shader.
 *
 * Key Features:
 * - Point Sprite Rendering: Transforms particle positions for point-based rendering
 * - Weather System Integration: Supports both snow and rain particle types
 * - Efficient Rendering: Minimal vertex processing for high particle counts
 * - Screen Space Transformation: Converts world positions to clip space
 *
 * Usage:
 * - Snow Particles: Rendered as snowflake sprites with procedural snowflake patterns
 * - Rain Particles: Rendered as simple point sprites for rain drops
 * - Weather Effects: Integrated with the dynamic weather system
 *
 * Input Attributes:
 * - pos: Particle position in world space
 *
 * Output:
 * - gl_Position: Transformed position in clip space
 * - gl_PointSize: Size of the point sprite (set to 1.0 for fragment shader control)
 */

#version 120

// Input attribute for particle position
attribute vec3 pos; // Particle position in world space

void main() {
    // Transform particle position from world space to clip space
    // This positions the particle correctly on screen for rendering
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
    
    // Set point size for sprite rendering
    // The fragment shader will handle the actual sprite appearance
    gl_PointSize = 1.0;
} 