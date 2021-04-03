# TooSquared

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
 * Capture mouse
 * Curser/reticle
 * optimize block creation and deletion
 * Render water correctly
 * collision
 * improve chunk update code
 * Added perlin seeding









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

