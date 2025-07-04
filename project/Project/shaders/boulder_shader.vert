/*
 * Boulder Vertex Shader - Procedural Rock Surface Rendering
 *
 * This vertex shader processes boulder geometry and prepares data for realistic rock surface
 * rendering. It handles normal transformation, texture coordinate passing, world position
 * calculation, and vertex transformation for the boulder mesh. The shader supports dynamic
 * lighting, texture mapping, and height-based effects for procedural boulder generation.
 *
 * Key Functions:
 * - Normal Transformation: Converts object-space normals to world-space for proper lighting
 * - Texture Coordinate Passing: Provides UV coordinates for rock texture mapping
 * - World Position Calculation: Computes vertex positions in world space for lighting calculations
 * - Height Data: Passes vertex height for potential height-based effects
 * - Vertex Transformation: Applies model-view-projection matrix for screen space positioning
 *
 * Input Attributes:
 * - gl_Vertex: Vertex position in object space
 * - gl_Normal: Vertex normal in object space
 * - gl_MultiTexCoord0: Primary texture coordinates
 *
 * Output Varyings:
 * - Normal: Transformed normal vector for lighting calculations
 * - Height: Vertex height for height-based effects
 * - TexCoord: Texture coordinates for rock surface mapping
 * - WorldPos: World space position for lighting calculations
 */

#version 120

// Output variables passed to fragment shader
varying vec3 Normal; // Transformed normal vector for lighting
varying float Height; // Vertex height for height-based effects
varying vec2 TexCoord; // Texture coordinates for rock surface
varying vec3 WorldPos; // World space position for lighting

void main() {
    // Transform normal from object space to world space using normal matrix
    // This ensures proper lighting calculations regardless of object transformations
    Normal = normalize(gl_NormalMatrix * gl_Normal);
    
    // Extract vertex height (Y component) for potential height-based effects
    // Could be used for snow accumulation, moss growth, or other height-dependent features
    Height = gl_Vertex.y;
    
    // Pass texture coordinates to fragment shader for rock surface mapping
    // These coordinates map the rock texture onto the boulder surface
    TexCoord = gl_MultiTexCoord0.st;
    
    // Calculate world space position by applying model-view transformation
    // This position is used for lighting calculations in the fragment shader
    WorldPos = vec3(gl_ModelViewMatrix * gl_Vertex);
    
    // Transform vertex from object space to clip space for rendering
    // This is the final transformation that positions the vertex on screen
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
} 