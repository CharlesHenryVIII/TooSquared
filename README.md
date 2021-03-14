# TooSquared

* Flat Vertex Attribute
* singertons/simpletons/simplexingtons????

###TODO: 
 * Properly handle chunk location
 * Moving Lighting/Sun
 * Directional, point, clamped/ambient
 * Octave Noise
 * Combined Noise
 * Raycasts
 * Block wireframe on target
 * Block place and removal
 * Operator overloads on WorldPos and ChunkPos
 * Move main loops into a single loop that switches on chunk state?
 

 Upload chunk position
 upload block index in place of the vertex position
 find the vertex position based on the chunk position and the block index


 1. Frustum culling
 2. Renderable sorting
 3. neighbor sampling
 4.? Sun???










#### Noise Solutions:
Mountains:
FBM;
blockRatio /= 100;
Max 10
H = 1
Ocataves = 32
f = 1
a = 1
t = 0

Mountains 2:
FBM;
blockRatio /= 100;
Max 10
H = 1.5
Ocataves = 16
f = 1
a = 1
t = 0

