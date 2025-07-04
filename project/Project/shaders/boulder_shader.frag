/*
 * Boulder Fragment Shader - Realistic Rock Surface Lighting and Texturing
 *
 * This fragment shader implements realistic rock surface rendering with dynamic lighting,
 * texture mapping, and procedural color variation. It combines base rock colors with
 * texture details, applies per-pixel lighting calculations, and supports multiple rock
 * color variations for visual diversity in the procedural boulder system.
 *
 * Key Features:
 * - Dynamic Lighting: Per-pixel lighting with ambient and diffuse components
 * - Texture Blending: Combines procedural colors with rock texture details
 * - Color Variation: Multiple rock color schemes for visual diversity
 * - Height-Based Effects: Support for height-dependent surface properties
 * - Realistic Shading: Proper normal-based lighting calculations
 *
 * Lighting Model:
 * - Ambient: Base illumination level for shadowed areas
 * - Diffuse: Directional lighting based on surface normal and light direction
 * - Combined: Blends ambient and diffuse for realistic rock appearance
 *
 * Color System:
 * - Base Colors: 5 different rock color schemes (gray, brown, tan variations)
 * - Texture Blending: 50% mix between base color and texture detail
 * - Final Color: Ambient + (diffuse * color * lightColor)
 */

#version 120

// Input variables from vertex shader
varying vec3 Normal; // Interpolated normal vector for lighting calculations
varying float Height; // Interpolated vertex height for height-based effects
varying vec2 TexCoord; // Interpolated texture coordinates for surface mapping
varying vec3 WorldPos; // Interpolated world position for lighting calculations

// Uniform variables set by the application
uniform vec3 lightDir; // Light direction vector (currently unused, using lightPos instead)
uniform vec3 lightColor; // Light color and intensity from scene lighting
uniform int boulderColorIndex; // Index for selecting rock color variation (0-4)
uniform sampler2D boulderTex; // Rock surface texture for detail mapping
uniform vec3 lightPos; // Light position in world space for dynamic lighting

void main() {
    // Normalize the interpolated normal vector for accurate lighting calculations
    // Interpolation can make normals non-unit length, so normalization is essential
    vec3 N = normalize(Normal);
    
    // Calculate light direction from light position to current fragment position
    // This creates dynamic lighting that changes based on fragment position
    vec3 L = normalize(lightPos - WorldPos);
    
    // Calculate diffuse lighting intensity using dot product of normal and light direction
    // Clamp to [0,1] range and blend with ambient (0.3) for realistic rock appearance
    // The 0.7 factor controls the strength of the diffuse component
    float intensity = max(dot(N, L), 0.0) * 0.7 + 0.3;
    
    // Select base rock color based on the color index for visual variation
    // Each boulder can have a different base color to avoid visual repetition
    vec3 baseColor;
    if (boulderColorIndex == 0) baseColor = vec3(0.25, 0.23, 0.21); // Dark gray
    else if (boulderColorIndex == 1) baseColor = vec3(0.38, 0.36, 0.34); // Medium gray
    else if (boulderColorIndex == 2) baseColor = vec3(0.45, 0.44, 0.42); // Light gray
    else if (boulderColorIndex == 3) baseColor = vec3(0.32, 0.29, 0.27); // Brown-gray
    else baseColor = vec3(0.22, 0.20, 0.18); // Very dark gray (default)
    
    // Sample the rock texture at the current texture coordinates
    // This provides surface detail and variation to the base color
    vec3 texColor = texture2D(boulderTex, TexCoord).rgb;
    
    // Blend base color with texture color for final surface color
    // 50% mix creates a balance between procedural color and texture detail
    vec3 color = mix(baseColor, texColor, 0.5);
    
    // Define ambient lighting level for shadowed areas
    // This ensures rocks are never completely dark, even in shadows
    vec3 ambient = vec3(0.13, 0.13, 0.13);
    
    // Calculate final fragment color by combining ambient and diffuse lighting
    // Ambient provides base illumination, diffuse provides directional lighting
    // The result is a realistic rock surface with proper lighting and texturing
    gl_FragColor = vec4(ambient + intensity * color * lightColor, 1.0);
} 