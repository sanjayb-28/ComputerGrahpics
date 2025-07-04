/*
 * Tree Branch Fragment Shader - Realistic Bark Surface Lighting and Texturing
 *
 * This fragment shader implements realistic tree branch rendering with dynamic lighting,
 * bark texture mapping, and height-based color variation. It combines procedural bark
 * colors with texture details, applies per-pixel lighting calculations, and simulates
 * natural bark color variation based on branch height for realistic tree appearance.
 *
 * Key Features:
 * - Dynamic Lighting: Per-pixel lighting with ambient and diffuse components
 * - Height-Based Color: Bark color varies from dark brown at base to lighter brown at top
 * - Texture Blending: Combines procedural colors with bark texture details
 * - Realistic Shading: Proper normal-based lighting calculations
 * - Natural Variation: Simulates natural bark color variation in trees
 *
 * Lighting Model:
 * - Ambient: Base illumination level for shadowed areas (10% intensity)
 * - Diffuse: Directional lighting based on surface normal and light direction (80% intensity)
 * - Combined: Blends ambient and diffuse for realistic bark appearance
 *
 * Color System:
 * - Base Colors: Dark brown (0.35, 0.18, 0.05) to light brown (0.7, 0.45, 0.18)
 * - Height Variation: Interpolates between colors based on vertex height
 * - Texture Blending: 50% mix between procedural color and bark texture detail
 * - Final Color: Ambient + (diffuse * color * lightColor)
 */

#version 120

// Input variables from vertex shader
varying vec3 Normal; // Interpolated normal vector for lighting calculations
varying float Height; // Interpolated vertex height for bark color variation
varying vec2 TexCoord; // Interpolated texture coordinates for bark mapping
varying vec3 WorldPos; // Interpolated world position for lighting calculations

// Uniform variables set by the application
uniform vec3 lightPos; // Light position in world space for dynamic lighting
uniform vec3 lightColor; // Light color and intensity from scene lighting
uniform sampler2D barkTex; // Bark texture for surface detail mapping

void main() {
    // Normalize the interpolated normal vector for accurate lighting calculations
    // Interpolation can make normals non-unit length, so normalization is essential
    vec3 N = normalize(Normal);
    
    // Calculate light direction from light position to current fragment position
    // This creates dynamic lighting that changes based on fragment position
    vec3 L = normalize(lightPos - WorldPos);
    
    // Calculate diffuse lighting intensity using dot product of normal and light direction
    // Clamp to [0,1] range and blend with ambient (0.2) for realistic bark appearance
    // The 0.8 factor controls the strength of the diffuse component
    float intensity = max(dot(N, L), 0.0) * 0.8 + 0.2;
    
    // Create height-based bark color variation using linear interpolation
    // Dark brown at base (Height = 0) to lighter brown at top (Height = 1)
    // Clamp height to [0,1] range to ensure proper color interpolation
    vec3 color = mix(vec3(0.35, 0.18, 0.05), vec3(0.7, 0.45, 0.18), clamp(Height, 0.0, 1.0));
    
    // Sample the bark texture at the current texture coordinates
    // This provides surface detail and natural bark patterns
    vec3 barkColor = texture2D(barkTex, TexCoord).rgb;
    
    // Blend procedural color with bark texture color for final surface color
    // 50% mix creates a balance between procedural color and texture detail
    color = mix(color, barkColor, 0.5);
    
    // Define ambient lighting level for shadowed areas
    // This ensures bark is never completely dark, even in shadows
    vec3 ambient = vec3(0.10, 0.10, 0.10);
    
    // Calculate final fragment color by combining ambient and diffuse lighting
    // Ambient provides base illumination, diffuse provides directional lighting
    // The result is a realistic bark surface with proper lighting and natural color variation
    gl_FragColor = vec4(ambient + intensity * color * lightColor, 1.0);
} 