# TooSquared

###TODO: 
 * Optimize block creation and deletion
 * Saving chunks to disk
 * imGUI
 * UI

 * Directional, point
 * Block wireframe on target
 * Operator overloads on WorldPos and ChunkPos
 * Move main loops into a single loop that switches on chunk state?
 * Curser/reticle
 * Render water correctly
 * Added perlin seeding
 * move physics off of transform and onto seperate physics object
	* Rigid Body includes mass, drag, angular drag, uses gravity, kinematic, interpolation, collision detection { Unity Game Engine }







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
