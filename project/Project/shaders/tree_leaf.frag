/*
 * Tree Leaf Fragment Shader - Realistic Foliage Lighting and Texturing
 *
 * This fragment shader implements realistic tree leaf rendering with dynamic lighting,
 * leaf texture mapping, and multiple color variation systems. It combines procedural
 * leaf colors with texture details, applies per-pixel lighting calculations, and supports
 * both tree-type color variation and height-based color variation for realistic foliage.
 *
 * Key Features:
 * - Dynamic Lighting: Per-pixel lighting with ambient and diffuse components
 * - Tree-Type Color: Multiple leaf color schemes for different tree types (pine, maple, etc.)
 * - Height-Based Variation: Leaf color lightens with height (sun exposure effect)
 * - Texture Blending: Combines procedural colors with leaf texture details
 * - Realistic Shading: Proper normal-based lighting calculations
 *
 * Lighting Model:
 * - Ambient: Base illumination level for shadowed areas (10% intensity)
 * - Diffuse: Directional lighting based on surface normal and light direction (80% intensity)
 * - Combined: Blends ambient and diffuse for realistic foliage appearance
 *
 * Color System:
 * - Tree Types: 6 different leaf color schemes (green, bright green, yellow, orange, red, teal)
 * - Height Variation: Interpolates toward white based on height (sun exposure)
 * - Texture Blending: 80% mix between procedural color and leaf texture detail
 * - Final Color: Ambient + (diffuse * color * lightColor)
 */

#version 120

// Input variables from vertex shader
varying vec3 Normal; // Interpolated normal vector for lighting calculations
varying float Height; // Interpolated vertex height for sun exposure effects
varying vec2 TexCoord; // Interpolated texture coordinates for leaf mapping
varying vec3 WorldPos; // Interpolated world position for lighting calculations

// Uniform variables set by the application
uniform vec3 lightPos; // Light position in world space for dynamic lighting
uniform vec3 lightColor; // Light color and intensity from scene lighting
uniform sampler2D leafTex; // Leaf texture for surface detail mapping
uniform int leafColorIndex; // Index for selecting leaf color variation (0-5)

void main() {
    // Normalize the interpolated normal vector for accurate lighting calculations
    // Interpolation can make normals non-unit length, so normalization is essential
    vec3 N = normalize(Normal);
    
    // Calculate light direction from light position to current fragment position
    // This creates dynamic lighting that changes based on fragment position
    vec3 L = normalize(lightPos - WorldPos);
    
    // Calculate diffuse lighting intensity using dot product of normal and light direction
    // Clamp to [0,1] range and blend with ambient (0.2) for realistic foliage appearance
    // The 0.8 factor controls the strength of the diffuse component
    float intensity = max(dot(N, L), 0.0) * 0.8 + 0.2;
    
    // Select base leaf color based on the tree type for visual variation
    // Each tree type can have different leaf colors (pine, maple, birch, etc.)
    vec3 baseColor;
    if (leafColorIndex == 0) baseColor = vec3(0.13, 0.45, 0.13); // Dark green (pine)
    else if (leafColorIndex == 1) baseColor = vec3(0.5, 0.9, 0.3); // Bright green (spring)
    else if (leafColorIndex == 2) baseColor = vec3(0.9, 0.7, 0.3); // Yellow (autumn)
    else if (leafColorIndex == 3) baseColor = vec3(0.8, 0.4, 0.1); // Orange (autumn)
    else if (leafColorIndex == 4) baseColor = vec3(0.7, 0.2, 0.2); // Red (autumn)
    else baseColor = vec3(0.2, 0.6, 0.4); // Teal green (default)
    
    // Apply height-based color variation to simulate sun exposure
    // Higher leaves are more exposed to sunlight and appear lighter
    // Clamp height to [0,1] range and blend toward white (50% maximum)
    float h = clamp(Height, 0.0, 1.0);
    vec3 color = mix(baseColor, vec3(1.0, 1.0, 1.0), h * 0.5);
    
    // Sample the leaf texture at the current texture coordinates
    // This provides surface detail and natural leaf patterns
    vec3 texColor = texture2D(leafTex, TexCoord).rgb;
    
    // Blend procedural color with leaf texture color for final surface color
    // 80% mix emphasizes texture detail while maintaining procedural color variation
    color = mix(color, texColor, 0.8);
    
    // Define ambient lighting level for shadowed areas
    // This ensures leaves are never completely dark, even in shadows
    vec3 ambient = vec3(0.10, 0.10, 0.10);
    
    // Calculate final fragment color by combining ambient and diffuse lighting
    // Ambient provides base illumination, diffuse provides directional lighting
    // The result is a realistic leaf surface with proper lighting and natural color variation
    gl_FragColor = vec4(ambient + intensity * color * lightColor, 1.0);
} 