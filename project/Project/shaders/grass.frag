/*
 * Grass Fragment Shader - Realistic Grass Surface Lighting and Texturing
 *
 * This fragment shader implements realistic grass rendering with dynamic lighting,
 * color palette variation, texture mapping, and transparency effects. It combines
 * procedural grass colors with texture details, applies per-pixel lighting calculations,
 * and supports multiple grass color variations for natural landscape diversity.
 *
 * Key Features:
 * - Dynamic Lighting: Per-pixel lighting with ambient and diffuse components
 * - Color Palette: 4 different grass color schemes for natural variation
 * - Texture Blending: Combines procedural colors with grass texture details
 * - Transparency: Alpha blending for realistic grass blade appearance
 * - Realistic Shading: Proper normal-based lighting calculations
 * - Natural Variation: Procedural color variation within each palette
 *
 * Lighting Model:
 * - Ambient: Base illumination level for shadowed areas (30% intensity)
 * - Diffuse: Directional lighting based on surface normal and sun direction (70% intensity)
 * - Combined: Blends ambient and diffuse for realistic grass appearance
 *
 * Color System:
 * - Palette Colors: 4 grass color schemes (dark green, medium green, yellow-green, brown)
 * - Color Variation: Additional variation within each palette color
 * - Texture Blending: 40% mix between procedural color and grass texture detail
 * - Final Color: (baseColor * lighting) + (ambient * 0.7)
 */

#version 120

// Input variables from vertex shader
varying float vAlpha; // Alpha value for transparency effects
varying vec3 vNormal; // Interpolated normal vector for lighting calculations
varying float vColorVar; // Color variation factor for blade diversity
varying vec2 vTexCoord; // Interpolated texture coordinates for grass mapping
varying float vColorIndex; // Color palette index for blade variation

// Uniform variables set by the application
uniform vec3 sunDir; // Sun direction vector for lighting calculations
uniform vec3 ambient; // Ambient lighting color and intensity
uniform sampler2D grassTex; // Grass texture for surface detail mapping

void main() {
    // Define color palette for grass variation
    // Four different grass colors for natural landscape diversity
    vec3 palette[4];
    palette[0] = vec3(0.13, 0.45, 0.13); // Dark green (shaded areas)
    palette[1] = vec3(0.25, 0.55, 0.18); // Medium green (typical grass)
    palette[2] = vec3(0.60, 0.60, 0.18); // Yellow-green (sun-exposed)
    palette[3] = vec3(0.45, 0.32, 0.13); // Brown (dried grass)
    
    // Select base color from palette based on color index
    // Clamp to valid range and convert to integer for array indexing
    int idx = int(clamp(vColorIndex * 4.0, 0.0, 3.0));
    vec3 baseColor = palette[idx] + vColorVar * 0.18; // Add variation within palette
    
    // Sample the grass texture at the current texture coordinates
    // This provides surface detail and natural grass patterns
    vec4 texColor = texture2D(grassTex, vTexCoord);
    
    // Blend procedural color with grass texture color for final surface color
    // 40% mix creates a balance between procedural color and texture detail
    baseColor = mix(baseColor, texColor.rgb, 0.4);
    
    // Calculate diffuse lighting using dot product of normal and sun direction
    // Normalize both vectors for accurate lighting calculations
    float diff = max(dot(normalize(vNormal), normalize(sunDir)), 0.0);
    
    // Calculate final color by combining lighting with base color and ambient
    // 70% diffuse + 30% ambient creates realistic grass lighting
    vec3 color = baseColor * (0.7 * diff + 0.3) + ambient * 0.7;
    
    // Calculate final fragment color with transparency
    // Alpha combines vertex alpha, texture alpha, and 70% base transparency
    gl_FragColor = vec4(color, 0.7 * vAlpha * texColor.a);
} 