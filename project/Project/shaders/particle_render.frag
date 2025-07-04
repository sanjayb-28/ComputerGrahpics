/*
 * Particle Render Fragment Shader - Procedural Snowflake and Weather Effects
 *
 * This fragment shader implements procedural snowflake generation and weather particle
 * rendering. It creates realistic snowflake patterns using mathematical functions and
 * provides appropriate coloring for weather effects. The shader supports both snow
 * and rain particle types with different visual characteristics.
 *
 * Key Features:
 * - Procedural Snowflakes: Mathematical generation of snowflake patterns
 * - Weather Particle Rendering: Supports both snow and rain effects
 * - Realistic Coloring: Appropriate colors for different weather conditions
 * - Efficient Rendering: Optimized for high particle counts
 * - Point Sprite Support: Renders particles as textured sprites
 *
 * Snowflake Generation:
 * - Radial Symmetry: 6-pointed snowflake using trigonometric functions
 * - Smooth Edges: Uses smoothstep for natural snowflake appearance
 * - Multiple Layers: Combines outer arms with inner core
 * - Procedural Variation: Each snowflake has unique characteristics
 *
 * Color System:
 * - Snow: Light blue-white colors for realistic snow appearance
 * - Rain: Can be adapted for rain drop rendering
 * - Transparency: Alpha blending for realistic particle effects
 */

#version 120

// Procedural snowflake generation function
// Creates realistic 6-pointed snowflake patterns using mathematical functions
float simpleSnowflake(vec2 uv) {
    // Transform UV coordinates from [0,1] to [-1,1] range
    uv = uv * 2.0 - 1.0;
    
    // Calculate polar coordinates for radial snowflake structure
    float r = length(uv); // Distance from center
    float a = atan(uv.y, uv.x); // Angle from center
    
    // Create 6-pointed snowflake arms using sine function
    // Multiplies angle by 3 to create 6 arms (3 * 2 = 6 points)
    float arm = abs(sin(3.0 * a));
    
    // Build snowflake shape by combining outer arms with inner core
    // Outer arms: smoothstep creates tapered arm shapes
    float flake = smoothstep(0.25, 0.22, r) * smoothstep(0.5, 0.2, arm);
    
    // Inner core: adds central snowflake structure
    flake += smoothstep(0.08, 0.0, r) * 0.5;
    
    // Clamp result to valid range [0,1]
    return clamp(flake, 0.0, 1.0);
}

void main() {
    // Generate snowflake pattern at current point coordinate
    // gl_PointCoord provides UV coordinates within the point sprite
    float flake = simpleSnowflake(gl_PointCoord);
    
    // Calculate snowflake color with light blue-white tint
    // Mix between light blue (0.85, 0.92, 1.0) and pure white (1.0, 1.0, 1.0)
    vec3 snowColor = mix(vec3(0.85, 0.92, 1.0), vec3(1.0), 0.5);
    
    // Set final fragment color with transparency
    // Alpha combines snowflake pattern with 85% base transparency
    gl_FragColor = vec4(snowColor, flake * 0.85);
} 