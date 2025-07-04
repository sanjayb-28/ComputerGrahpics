/*
 * Grass Vertex Shader - Procedural Grass Blade Animation and Rendering
 *
 * This vertex shader implements a sophisticated grass rendering system with procedural
 * blade generation, wind animation, and dynamic geometry deformation. It creates realistic
 * grass blades with individual swaying motion, curvature, twisting, and wind effects.
 * Each grass blade is procedurally generated with unique properties for natural variation.
 *
 * Key Features:
 * - Procedural Blade Generation: Creates individual grass blades with unique properties
 * - Wind Animation: Realistic swaying motion based on time and wind strength
 * - Blade Curvature: Natural curve along the blade length for realistic appearance
 * - Twisting Effects: Rotation around the blade axis for natural variation
 * - Individual Variation: Each blade has unique sway seed, color, and dimensions
 * - Dynamic Alpha: Transparency based on sway intensity for realistic rendering
 *
 * Animation System:
 * - Wind Sway: Sinusoidal motion based on time, position, and individual seed
 * - Tip Factor: Sway intensity increases toward blade tip
 * - Curvature: Natural bend along blade length using sine function
 * - Twisting: Rotation around Y-axis for natural blade variation
 *
 * Input Attributes:
 * - position: Base position of grass blade in world space
 * - swaySeed: Unique seed for individual blade animation
 * - offsetX/Y: Local position within the blade (0 to bladeWidth/Height)
 * - bladeHeight/Width: Dimensions of the individual blade
 * - colorVar: Color variation factor for blade diversity
 * - rotation: Individual blade rotation around Y-axis
 *
 * Uniform Variables:
 * - time: Current time for animation
 * - windStrength: Wind intensity multiplier
 */

#version 120

// Input attributes for individual grass blade properties
attribute vec3 position; // Base position of grass blade in world space
attribute float swaySeed; // Unique seed for individual blade animation variation
attribute float offsetX; // Local X position within blade (0 to bladeWidth)
attribute float offsetY; // Local Y position within blade (0 to bladeHeight)
attribute float bladeHeight; // Height of the individual grass blade
attribute float bladeWidth; // Width of the individual grass blade
attribute float colorVar; // Color variation factor for blade diversity
attribute float rotation; // Individual blade rotation around Y-axis

// Uniform variables for animation and effects
uniform float time; // Current time for wind animation
uniform float windStrength; // Wind intensity multiplier

// Output variables passed to fragment shader
varying float vAlpha; // Alpha value for transparency effects
varying vec3 vNormal; // Transformed normal vector for lighting
varying float vColorVar; // Color variation passed to fragment shader
varying vec2 vTexCoord; // Texture coordinates for grass surface
varying float vColorIndex; // Color palette index for blade variation

void main() {
    // Calculate natural blade curvature using sine function
    // Creates realistic bend along the blade length (35% maximum curve)
    float curve = 0.35 * sin(offsetY / bladeHeight * 1.57);
    
    // Calculate wind-induced swaying motion
    // Combines time, position, and individual seed for unique animation
    // Wind strength controls the intensity of the sway effect
    float sway = sin(time * 1.5 + position.x * 0.2 + position.z * 0.3 + swaySeed * 6.28) * 0.2 * windStrength;
    
    // Calculate tip factor for progressive sway (more sway at blade tip)
    // This creates realistic motion where blade tips move more than bases
    float tipFactor = offsetY / bladeHeight;
    float swayX = sway * tipFactor;
    
    // Calculate blade twisting effect for natural variation
    // Creates rotation around the blade axis based on height and seed
    float twist = 0.15 * sin(offsetY * 6.0 / bladeHeight + swaySeed * 3.0);
    
    // Build local blade position with curvature, twisting, and offsets
    vec3 local = vec3(offsetX + twist, offsetY, curve);
    
    // Calculate rotation matrix for individual blade orientation
    float c = cos(rotation);
    float s = sin(rotation);
    mat3 rotY = mat3(
        c, 0, -s, // X component rotation
        0, 1, 0,  // Y component (unchanged)
        s, 0,  c  // Z component rotation
    );
    
    // Apply rotation to local blade coordinates
    local = rotY * local;
    
    // Calculate final world position by combining base position and local coordinates
    vec3 pos = position + local;
    pos.x += swayX; // Apply wind sway to final position
    
    // Calculate alpha for transparency based on sway intensity
    // More sway creates more transparency for realistic grass appearance
    vAlpha = 1.0 - abs(sway) * 0.5;
    
    // Transform normal vector using the same rotation matrix
    // Ensures proper lighting calculations for rotated blades
    vNormal = normalize(rotY * vec3(0.0, 1.0, 0.0));
    
    // Pass color variation to fragment shader
    vColorVar = colorVar;
    
    // Generate texture coordinates based on position within blade
    // Creates proper UV mapping for grass texture
    if (abs(offsetX) < 0.001 && abs(offsetY) < 0.001) {
        vTexCoord = vec2(0.0, 0.0); // Base left corner
    } else if (abs(offsetX - bladeWidth) < 0.001 && abs(offsetY) < 0.001) {
        vTexCoord = vec2(1.0, 0.0); // Base right corner
    } else {
        vTexCoord = vec2(0.5, 1.0); // Blade tip
    }
    
    // Calculate color palette index for blade variation
    // Maps color variation to discrete palette indices
    vColorIndex = floor(colorVar * 4.0 + 2.0) / 4.0;
    
    // Transform final position to clip space for rendering
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
} 