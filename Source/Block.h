#pragma once

#include "Misc.h"
#include "Rendering.h"

enum class BlockType : uint8 {
    Empty,
    Grass,
    Dirt,
    Stone,
    Planks,
    StoneSlab,
    Brick,
    TNT,
    Cobblestone,
    Bedrock,
    Sand,
    Gravel,
    Wood,
    IronBlock,
    GoldBlock,
    DiamondBlock,
    Snow,
    Ice,
    Obsidian,
    Leaves,
    MossyCobblestone,
    Water,
    HalfSlab,
    Slab,
    Glass,
    Count,
//CobWeb,
//Flower_Red,
//Flower_Yellow,
//Flower_Blue,
//Sappling,
//Chest,
};
ENUMOPS(BlockType);

enum class Face : uint8 {
    Right,
    Left,
    Top,
    Bot,
    Back,
    Front,
    Count,
};
ENUMOPS(Face);

const uint32 defaultSpriteLocation = 254;
struct Block {
    uint32 m_spriteIndices[+Face::Count] = {
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
    float m_collisionHeight       = 1.0f;
    bool  m_translucent           = false;
    bool  m_seeThrough            = false;
    bool  m_collidable            = true;
    bool  m_sidesShouldBeRendered = false;
    bool  m_hasShading            = true;
    //Material material;
};

struct BlockSampler {
    BlockType blocks[+Face::Count] = {};
    GamePos m_baseBlockP = {};
    BlockType m_baseBlockType = BlockType::Empty;
    bool RegionGather(GamePos m_baseBlockP);
};

extern Block g_blocks[+BlockType::Count];

struct Camera;

#define DEBUG_CUBE_RENDERER 2
//
#if DEBUG_CUBE_RENDERER == 1
struct RenderCube { //assume player camera
    WorldPos    p;
    Color       color;
    Vec3        scale;
};
#endif
void AddCubeToRender(WorldPos p, Color color, float scale);
void AddCubeToRender(WorldPos p, Color color, Vec3  scale);
void RenderTransparentCubes(Camera* playerCamera, const int32 passCount, bool lastPass);
void RenderOpaqueCubes(     Camera* playerCamera,   const int32 passCount);
//
void AddBlockToRender(WorldPos p, float scale, BlockType block);
void AddBlockToRender(WorldPos p, Vec3 scale, BlockType block);
void RenderTransparentBlocks(Camera* playerCamera,   const int32 passCount, bool lastPass);
void RenderOpaqueBlocks(     Camera* playerCamera,   const int32 passCount);
//

void BlockInit();
Rect GetUVsFromIndex(uint8 index);
void Draw2DSquare(Rect rect, Color color);
