/*
 * Particle Update Vertex Shader - Weather Particle Physics Simulation
 *
 * This vertex shader implements a sophisticated particle physics system for weather
 * effects (snow/rain). It handles particle movement, terrain collision detection,
 * wind effects, and particle lifecycle management. The shader simulates realistic
 * weather particle behavior including falling, accumulation, and regeneration.
 *
 * Key Features:
 * - Particle Physics: Realistic gravity and wind effects on particles
 * - Terrain Collision: Heightmap-based collision detection with landscape
 * - Wind Simulation: Dynamic wind effects on particle movement
 * - Particle Lifecycle: Falling, accumulation, and regeneration cycle
 * - Boundary Constraints: Keeps particles within valid terrain area
 * - Heightmap Integration: Uses terrain height data for collision detection
 *
 * Particle States:
 * - State 0: Falling particles affected by wind and gravity
 * - State 1: Accumulated particles on terrain surface
 * - Regeneration: Particles respawn at cloud height after accumulation time
 *
 * Physics System:
 * - Gravity: Constant downward acceleration (-10.0 units/s²)
 * - Wind: Dynamic horizontal wind forces affecting particle movement
 * - Collision: Terrain height detection with margin for realistic accumulation
 * - Boundaries: Clamping to terrain boundaries to prevent particle escape
 *
 * Input Attributes:
 * - pos: Current particle position in world space
 * - vel: Current particle velocity vector
 * - restTime: Time particle has been accumulated on terrain
 * - state: Particle state (0 = falling, 1 = accumulated)
 *
 * Uniform Variables:
 * - dt: Delta time for physics integration
 * - time: Current simulation time
 * - cloudHeight: Height where particles spawn
 * - landscapeScale/Size: Terrain dimensions for coordinate conversion
 * - heightmap: Terrain height texture for collision detection
 * - wind: Current wind vector affecting particle movement
 * - terrainBounds: Terrain boundary constraints
 */

#version 120

// Input attributes for particle properties
attribute vec3 pos; // Current particle position in world space
attribute vec3 vel; // Current particle velocity vector
attribute float restTime; // Time particle has been accumulated on terrain
attribute float state; // Particle state (0 = falling, 1 = accumulated)

// Uniform variables for physics simulation
uniform float dt; // Delta time for physics integration
uniform float time; // Current simulation time
uniform float cloudHeight; // Height where particles spawn
uniform float landscapeScale; // Terrain scale factor
uniform float landscapeSize; // Terrain grid size
uniform sampler2D heightmap; // Terrain height texture
uniform vec2 wind; // Current wind vector (X and Z components)
uniform float terrainMinX, terrainMaxX; // Terrain X boundaries
uniform float terrainMinZ, terrainMaxZ; // Terrain Z boundaries

// Output variables for particle state update
varying vec3 outPos; // Updated particle position
varying vec3 outVel; // Updated particle velocity
varying float outRestTime; // Updated rest time
varying float outState; // Updated particle state

// Function to sample terrain height at given world coordinates
// Converts world coordinates to texture coordinates for heightmap sampling
float getTerrainHeight(float x, float z) {
    // Convert world coordinates to normalized texture coordinates
    vec2 uv = vec2((x / landscapeScale + 0.5) * (landscapeSize - 1.0), 
                   (z / landscapeScale + 0.5) * (landscapeSize - 1.0)) / (landscapeSize - 1.0);
    
    // Sample heightmap and return terrain height
    return texture2D(heightmap, uv).r;
}

void main() {
    // Initialize new particle state variables
    vec3 newPos = pos; // New position
    vec3 newVel = vel; // New velocity
    float newRestTime = restTime; // New rest time
    float newState = state; // New state
    
    // Get terrain height at current particle position
    float terrainY = getTerrainHeight(pos.x, pos.z);
    float margin = 1.0; // Collision margin for realistic accumulation
    
    // Handle falling particles (state 0)
    if (state < 0.5) {
        // Apply wind forces to velocity (horizontal components only)
        newVel = vec3(wind.x, vel.y, wind.y);
        
        // Integrate position using velocity and delta time
        newPos = pos + newVel * dt;
        
        // Clamp position to terrain boundaries with margin
        newPos.x = clamp(newPos.x, terrainMinX + margin, terrainMaxX - margin);
        newPos.z = clamp(newPos.z, terrainMinZ + margin, terrainMaxZ - margin);
        
        // Check for terrain collision
        if (newPos.y <= terrainY) {
            // Particle has hit terrain - accumulate it
            newPos.y = terrainY; // Set to terrain height
            newVel = vec3(0.0); // Stop particle movement
            newRestTime = 0.0; // Reset rest time
            newState = 1.0; // Change to accumulated state
        }
    } 
    // Handle accumulated particles (state 1)
    else {
        // Increment rest time for accumulated particles
        newRestTime = restTime + dt;
        newVel = vec3(0.0); // Keep accumulated particles stationary
        
        // Check if particle should regenerate (after 5 seconds)
        if (newRestTime > 5.0) {
            // Generate new random position within terrain bounds
            // Uses hash function for pseudo-random but deterministic positioning
            float rx = terrainMinX + margin + (terrainMaxX - terrainMinX - 2.0 * margin) * fract(sin(pos.x * 12.9898) * 43758.5453);
            float rz = terrainMinZ + margin + (terrainMaxZ - terrainMinZ - 2.0 * margin) * fract(sin(pos.z * 78.233) * 43758.5453);
            
            // Respawn particle at cloud height with downward velocity
            newPos = vec3(rx, cloudHeight, rz);
            newVel = vec3(0.0, -10.0, 0.0); // Initial downward velocity
            newRestTime = 0.0; // Reset rest time
            newState = 0.0; // Change back to falling state
        }
    }
    
    // Output updated particle state
    outPos = newPos;
    outVel = newVel;
    outRestTime = newRestTime;
    outState = newState;
    
    // Set vertex position for rendering (not used for physics, but required)
    gl_Position = vec4(newPos, 1.0);
}