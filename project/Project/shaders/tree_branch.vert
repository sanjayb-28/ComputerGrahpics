/*
 * Tree Branch Vertex Shader - Wooden Branch Surface Rendering
 *
 * This vertex shader processes tree branch geometry and prepares data for realistic wood
 * surface rendering. It handles normal transformation, texture coordinate passing, world
 * position calculation, and vertex transformation for the tree branch mesh. The shader
 * supports dynamic lighting, bark texture mapping, and height-based color variation for
 * realistic tree branch appearance.
 *
 * Key Functions:
 * - Normal Transformation: Converts object-space normals to world-space for proper lighting
 * - Texture Coordinate Passing: Provides UV coordinates for bark texture mapping
 * - World Position Calculation: Computes vertex positions in world space for lighting calculations
 * - Height Data: Passes vertex height for height-based bark color variation
 * - Vertex Transformation: Applies model-view-projection matrix for screen space positioning
 *
 * Input Attributes:
 * - gl_Vertex: Vertex position in object space
 * - gl_Normal: Vertex normal in object space
 * - gl_MultiTexCoord0: Primary texture coordinates for bark mapping
 *
 * Output Varyings:
 * - Normal: Transformed normal vector for lighting calculations
 * - Height: Vertex height for height-based bark color variation
 * - TexCoord: Texture coordinates for bark surface mapping
 * - WorldPos: World space position for lighting calculations
 */

#version 120

// Output variables passed to fragment shader
varying vec3 Normal; // Transformed normal vector for lighting
varying float Height; // Vertex height for height-based bark color variation
varying vec2 TexCoord; // Texture coordinates for bark surface
varying vec3 WorldPos; // World space position for lighting

void main() {
    // Transform normal from object space to world space using normal matrix
    // This ensures proper lighting calculations regardless of branch orientation
    Normal = normalize(gl_NormalMatrix * gl_Normal);
    
    // Extract vertex height (Y component) for height-based bark color variation
    // Higher parts of branches tend to have lighter bark colors
    Height = gl_Vertex.y;
    
    // Pass texture coordinates to fragment shader for bark surface mapping
    // These coordinates map the bark texture onto the branch surface
    TexCoord = gl_MultiTexCoord0.st;
    
    // Calculate world space position by applying model-view transformation
    // This position is used for lighting calculations in the fragment shader
    WorldPos = vec3(gl_ModelViewMatrix * gl_Vertex);
    
    // Transform vertex from object space to clip space for rendering
    // This is the final transformation that positions the vertex on screen
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
} 