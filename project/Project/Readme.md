# Alpine Wilderness Simulator
### By: Sanjay Baskaran

An interactive 3D simulation of a natural landscape inspired by the Boulder area, featuring dynamic weather, day/night cycles, and rich environmental interactions.

Time Spent: 80 Hours

## Controls

### Camera Controls
- **WASD**: First-person movement (when in FPS mode)
- **Arrow Keys**: Orbit camera rotation (when in orbit mode)
- **Mouse**: Look around (FPS mode only)
- **1**: Switch to First-Person Camera
- **2**: Switch to Orbit Camera
- **Z/z**: Zoom in/out (orbit mode)

### Environment Controls
- **T**: Toggle time animation
- **K/L**: Decrease/Increase time speed
- **E**: Toggle weather (Fall/Winter)
- **N**: Toggle snow particles
- **B**: Toggle fog
- **M**: Toggle ambient sound
- **R**: Reset camera and time

### Display Controls
- **Q**: Toggle wireframe mode
- **A**: Toggle axes display (orbit mode)

## Key Features

### Procedural Terrain Generation
- Perlin noise-based heightmap with multiple octaves
- Dynamic terrain coloring based on slope and height
- Weather-based seasonal color blending (fall/winter)
- Animated water rendering with wave effects

### Weather System
- Real-time day/night cycle with smooth color transitions
- 20,000 GPU-based snow particles with physics
- Terrain collision and respawn system
- Wind effects and particle lifetime management

### Camera System
- Dual camera modes: First-person and free orbit
- Terrain collision detection and height following
- Boundary clamping to landscape limits

### Atmospheric Effects
- Procedural sky dome with animated sun and moon
- 88 volumetric clouds
- OpenGL fog system with time-based density

### Procedural Vegetation
- 500,000 instanced grass blades with wind animation
- Recursive fractal tree generation with sway effects
- 50 procedurally placed boulders with collision detection
- Grid-based object placement with density control

### Audio
- SDL2-based ambient forest sounds
- Loopable audio with play/stop controls

## Attribution and AI Usage

### External Sources
- **Perlin Noise Implementation:** University of Maryland CMSC425 course materials, Stack Overflow community solutions, GitHub procedural terrain generator examples
- **Camera Concepts:** LearnOpenGL Camera chapter, OpenGL-tutorial.org keyboard/mouse handling, NerdHut arcball camera implementation
- **Particle System Concepts:** LearnOpenGL 2D Game Particles tutorial, OGLDev transform feedback tutorial, Open.gl feedback documentation
- **Sphere Implementation:** Song Ho OpenGL sphere tutorial
- **Grass Rendering Structure:** LearnOpenGL Instancing tutorial
- **Fractal Recursion Concepts:** GitHub Tree-Generator project, YouTube fractal tree tutorials
- **Shader Pipeline:** Lighthouse3D GLSL tutorial
- **Implementation Methods:** SDL2V2 class examples

### Claude AI Assistance
- **Boulder System:** Noise functions, normal computation, mesh generation
- **Cloud System:** Main implementation based on Real-Time Rendering billboards chapter
- **Particle System:** Apple platform macros, random particle placement logic
- **Tree System:** Branch randomization function, leaf segment rendering optimization
- **Objects System:** Slope calculation function, grid-based placement algorithms
- **Sky System:** Lighting blending function for proper sun/moon transitions

### Copilot AI Assistance
- **Boulder System:** Mesh data arrays generation
- **Various Systems:** Code completion and optimization suggestions

### Original Contributions
- All particle logic, physics, and rendering (Top Feature)
- Tree structure, rendering pipeline, animation integration (Top Feature)
- Weather-based color blending between fall and winter seasons
- Terrain integration, collision detection, movement logic
- Sky system logic, celestial body positioning, rendering pipeline
- Cloud system integration and atmospheric effects
- Implementation, mesh generation, animation, placement logic
- Boulder generation logic, placement validation, rendering
- Object management, validation logic, rendering coordination
- Audio system integration and controls

## Building and Running

```bash
cd Project
make clean && make
./final
```