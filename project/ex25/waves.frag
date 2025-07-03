// Ocean Shader by afl_ext
// MIT License
// https://www.shadertoy.com/view/MdXyzX
// Algorith used verbatim, but refactored to improve formatting for readibility
#version 120

uniform float time;     // Time in seconds
const float camy=1.5;   // Camera elevation
const float depth=1.0;  // Wave depth
const float drag=0.28;  // How much waves pull on the water

#define ITERATIONS_RAYMARCH 12 // Wave iterations of raymarching
#define ITERATIONS_NORMAL   40 // Wave iterations when calculating normals

//
// Calculates wave value and its derivative in the wave direction
// This is a numerical noise function based on position in space, wave frequency and time
//
vec2 WaveDx(vec2 position,vec2 direction,float frequency,float timeshift)
{
   float x    = dot(direction,position)*frequency + timeshift;
   float wave = exp(sin(x)-1);
   float dwdx = -wave*cos(x);
   return vec2(wave,dwdx);
}

//
// Calculates waves by summing octaves of pseudo-random waves with various parameters
//
float CalcWave(vec2 position,int iterations)
{
   float theta     = 0; // Angle of wave direction (changes each octave)
   float frequency = 1; // Frequency of the wave (changes by non-integer harmonics)
   float timemult  = 2; // Time multiplier changes apparent frequency by octave
   float weight    = 1; // Weight reducing amplitude of each octave
   float valuesum  = 0; // Weighted sum of values
   float weightsum = 0; // Sum of weights for normalization
   for (int i=0;i<iterations;i++)
   {
      // Wave direction (intended to look random)
      vec2 p = vec2(sin(theta),cos(theta));
      // Calculate wave data for this octave
      vec2 res = WaveDx(position, p, frequency, time * timemult);
      // Shift position around proportional to wave drag and derivative of the wave
      position += p*res.y*weight*drag;
      // Accumulate this octave
      valuesum += res.x * weight;
      weightsum += weight;
      // Modify next octave parameters
      weight    *= 0.82;        // Reduce amplitude of higher frequency waves
      frequency *= 1.18;        // Increase frequency but not a harmonic
      timemult  *= 1.07;        // Move time faster to add chaotic waves
      theta     += 1232.399963; // Change wave direction to appear random
   }
   // Normalize
   return valuesum/weightsum;
}

//
// Ray march from top water layer boundary to low water layer boundary
// Returns distance to intersection
//
float RayMarchWater(vec3 camera,vec3 start,vec3 end,float depth)
{
   vec3 pos = start;
   vec3 dir = normalize(end-start);
   for(int i=0;i<64;i++)
   {
      // The height is from 0 to -depth
      float height = CalcWave(pos.xz,ITERATIONS_RAYMARCH)*depth - depth;
      // If the waves height almost nearly matches the ray height, assume its a hit and return the hit distance
      if (height+0.01 > pos.y) return distance(pos,camera);
      // Iterate forwards according to the height mismatch
      pos += dir*(pos.y-height);
   }
   // If hit was not registered, just assume hit the top layer,
   // This looks better at higher distances
   return distance(start,camera);
}

//
// Calculate normal as cross product of two tangent vectors
//
vec3 Normal(vec2 pos,float depth)
{
   vec3 a = vec3(pos.x     ,0,pos.y);
   vec3 b = vec3(pos.x-0.01,0,pos.y);
   vec3 c = vec3(pos.x     ,0,pos.y+0.01);
   a.y = depth*CalcWave(a.xz,ITERATIONS_NORMAL);
   b.y = depth*CalcWave(b.xz,ITERATIONS_NORMAL);
   c.y = depth*CalcWave(c.xz,ITERATIONS_NORMAL);
   return normalize(cross(a-b,a-c));
}

//
//  Rotation matrix
//    angle in radians
//    x,y,z must be pre-normalized
//
mat3 RotMat(float th,float x,float y,float z)
{
   float s = sin(th);
   float c = cos(th);
   return mat3((1-c)*x*x+c   , (1-c)*x*y+z*s , (1-c)*z*x-y*s ,
               (1-c)*x*y-z*s , (1-c)*y*y+c   , (1-c)*y*z+x*s ,
               (1-c)*z*x+y*s , (1-c)*y*z-x*s , (1-c)*z*z+c   );
}

//
// Ray-Plane intersection distance
//
float PlaneDist(vec3 origin,vec3 direction,float z)
{
   vec3 normal = vec3(0,1,0);
   return clamp(dot(vec3(0,z,0)-origin,normal)/dot(direction,normal),-1,999.999);
}

//
// Very barebones but fast atmosphere approximation
//   Lighter near horizon and sun, darker elsewhere
//
vec3 CheapSky(vec3 raydir,vec3 sundir)
{
   //  Skyblue reference color
   vec3 skyblue  = vec3(0.246,0.580,1.0);
   //  Clamp sun direction
   sundir.y = max(-0.07,sundir.y);
   float raymul  = 1/(raydir.y+0.1);
   //  Sun contribution to color
   float raysun  = pow(abs(dot(sundir,raydir)),2);
   vec3 suncolor = mix(vec3(1), max(vec3(0),1-skyblue), 1/(11*sundir.y+1));
   //  Sky contribution to color
   vec3 skycolor = max(vec3(0), skyblue*(suncolor - 0.0448*(raymul-6*sundir.y*sundir.y)));
   //  Rescale and add
   skycolor *= 0.24*(raysun+1)*(1+pow(1-raydir.y,3));
   suncolor *= 0.2*pow(max(0,dot(sundir,raydir)),8);
   return raymul*(skycolor + suncolor);
}

//
//  Get color of sky with sun
//
vec3 SkyColor(vec3 dir)
{
   //  Calculate where the sun should be, it will be moving around the sky
   vec3 sundir = normalize(vec3(sin(0.1*time),1,cos(0.1*time)));
   //  Sky color
   vec3 sky = 0.5*CheapSky(dir,sundir);
   //  Sun color
   float sun = 210*pow(max(0,dot(dir,sundir)),720);
   return sky + sun;
}

//
//  Remap color tones to paler blue
//    afl_ext shader: https://www.shadertoy.com/view/XsGfWV
//
vec4 aces_tonemap(vec3 color)
{
   mat3 m1 = mat3(
     1.19438,0.15200,0.05680,
     0.70916,1.81668,0.26766,
     0.09646,0.03132,1.67554
   );
   mat3 m2 = mat3(
     1.60475,-0.10208,-0.00327,
    -0.53108, 1.10813,-0.07276,
    -0.07367,-0.00605, 1.07602
   );
   vec3 v = m1*color;
   vec3 a = v*v+0.024578*v-0.000090537;
   vec3 b = 0.983729*v*v+0.4329510*v+0.238081;
   v = m2*(a/b);
   return vec4(pow(clamp(v,0,1),vec3(1/2.2)),1);
}

//
// Ray trace based on texture coordinates
//
void main()
{
   // Initialize ray direction for every pixel based on texture coordinates
   vec3 dir = normalize(vec3(gl_TexCoord[0].xy-0.9,1.5));
   // If dir.y is positive, render sky
   if(dir.y >= 0)
   {
      gl_FragColor = aces_tonemap(SkyColor(dir));
   }
   // If dir.y is negative, render water
   else
   {
      // Ray origin, moving around
      vec3 origin = vec3(time,camy,time);

      // Calculate where ray hits high (0) horizontal plane
      float dist = PlaneDist(origin,dir,0);
      vec3 hiHitPos = origin + dist*dir;
      // Calculate where ray hits low (-depth) horizontal plane
      dist = PlaneDist(origin,dir,-depth);
      // Calculate hit locations for high and low planes
      vec3 loHitPos = origin + dist*dir;

      // Raymarch water.  Will be between high and low planes
      dist = RayMarchWater(origin,hiHitPos,loHitPos,depth);
      vec3 waterHitPos = origin + dist*dir;

      // Calculate normal at the hit position
      vec3 N = Normal(waterHitPos.xz,depth);

      // Smooth the normal with distance to avoid disturbing high frequency noise
      N = mix(N, vec3(0,1,0) , 0.8*min(1,1.1*sqrt(0.01*dist)));

      // Reflect the ray and make sure it bounces up
      vec3 R = normalize(reflect(dir,N));
      R.y = abs(R.y);

      // Calculate the reflection and approximate subsurface scattering
      vec3 reflection = SkyColor(R);
      vec3 scattering = vec3(0.0293,0.0698,0.1717) * 0.1*((waterHitPos.y+depth)/depth+0.2);

      // Calculate fresnel coefficient
      float fresnel = 0.96*pow(1-max(0,dot(-N,dir)),5)+0.04;
      // Combine reflection and scattering and apply tonemap
      gl_FragColor = aces_tonemap(fresnel*reflection + (1-fresnel)*scattering);
   }
}
