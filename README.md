# TooSquared

### TODO: 
 * Optimize block creation and deletion
 * Half height blocks
 * Gameplay??? (Conveyers, mining, etc?)
 * Modify see through opaque mip maps to not alpha blend

 * Directional and point lighting
 * Block wireframe on target
 * Operator overloads on WorldPos and ChunkPos
 * Move main loops into a single loop that switches on chunk state?
 * Render water correctly
 * Added perlin seeding
 * Convert from key inputs to action keys.  Do not trigger action keys when the g_cursorEngaged is false
 * Add bit to nAndConnectedVertices for whether the shader should apply lighting effects to blocks or not
 * Rename "DoThing" to "AsyncJobFunction" or something like that

&nbsp;

 #### Complex Belts TODO:
 - [ ] Animation
 - [ ] Block item interactions
 - [ ] Belt to belt interaction
	
&nbsp;

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
