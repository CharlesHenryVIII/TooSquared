# TooSquared

## How to Build

### Need
- premake (https://premake.github.io/download)

### Building
* open cmd and navigate to the UATHelper directory
* Type and run: `C:/path/to/premake5.exe vs2022`
    * fill out the correct path to premake5 and/or change the version of visual studios
* open the VS solution
* Build/run from there

## Current TODO
- [ ] Look into possibly removing one of file paths currently needed
- [ ] cleanup?
- [ ] Add support for fonts
- [ ] Chunk LOD system

## Future TODO: 
 * Optimize block creation and deletion
 * Half height blocks
 * Gameplay??? (Conveyers, mining, etc?)
 * Modify see through opaque mip maps to not alpha blend

 * Directional and point lighting
 * Block wireframe on target
 * Move main loops into a single loop that switches on chunk state?
 * Render water correctly
 * Added perlin seeding
 * Convert from key inputs to action keys.  Do not trigger action keys when the g_cursorEngaged is false
 * Add bit to nAndConnectedVertices for whether the shader should apply lighting effects to blocks or not
 * Rename "DoThing" to "AsyncJobFunction" or something like that
 * Add Upload template type in vertex and index buffer objects
 * Remove the need to add SDL.dll to each building version
	* Build directly against the lib instead?

## Do Not TODO's:
 * Operator overloads on WorldPos and ChunkPos
 * Convert to GLFW to remove the dependancy on dlls


&nbsp;

 #### Complex Belts TODO:
 - [X] half height player collision
 - [X] half height item collision
 - [X] Saving information to disk
 - [X] Save and Load child blocks
 - [X] Render block with rotations
 - [X] Block item interactions
 - [X] Belt to belt interaction
 - [X] Animation
	
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
