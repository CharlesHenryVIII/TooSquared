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
    float m_collisionHeight = 1.0f;
    bool  m_transparent     = false;
    bool  m_collidable      = true;
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

void BlockInit();
Rect GetUVsFromIndex(uint8 index);
void DrawBlock(const Mat4& mat, Color color, float scale, Camera* camera, Texture::T textureType, BlockType blockType);
void DrawBlock(const Mat4& mat, Color color, Vec3 scale, Camera* camera, Texture::T textureType, BlockType blockType);
void DrawCube(WorldPos p, Color color, Vec3  scale, Camera* camera, Texture::T textureType = Texture::T::Plain, BlockType blockType = BlockType::Empty);
void DrawCube(WorldPos p, Color color, float scale, Camera* camera, Texture::T textureType = Texture::T::Plain, BlockType blockType = BlockType::Empty);
void Draw2DSquare(Rect rect, Color color);
