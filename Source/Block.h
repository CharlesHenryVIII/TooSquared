#pragma once
#include "Math.h"
#include "Misc.h"
#include "Rendering.h"

enum class BlockType : uint32 {
    Invalid,
    Grass,
    Stone,
    Planks,
    StoneSlab,
    Brick,
    TNT,
    CobWeb,
    Flower_Red,
    Flower_Yellow,
    Flower_Blue,
    Sappling,
    Cobblestone,
    Bedrock,
    Sand,
    Gravel,
    Wood,
    Count,
    IronBlock,
    GoldBlock,
    DiamondBlock,
    Chest,
};
ENUMOPS(BlockType);

enum class Face : uint32 {
	Front,
	Back,
	Bot,
	Top,
	Right,
	Left,
	Count,
};
ENUMOPS(Face);

struct Block {
    Vec3 p = {};
    float reflection = 0;
    BlockType t = BlockType::Invalid;
    uint32 defaultSpriteLocation = 254;
    uint32 spriteLocation[static_cast<uint32>(Face::Count)] = {
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;

	void Render();
};

struct Grass : public Block {
    Grass()
    {
        defaultSpriteLocation = 3;
        spriteLocation[+Face::Top] = 0;
        spriteLocation[+Face::Bot] = 2;
    }
};

struct Stone : public Block {
    Stone()
    {
        reflection = 0.5f;
        defaultSpriteLocation = 1;
    }
};

struct IronBlock : public Block {
    IronBlock()
    {
        reflection = 1.0f;
        defaultSpriteLocation = 22;
    }
};

struct FireBlock : public Block {
    FireBlock()
    {
        reflection = 1.0f;
        defaultSpriteLocation = 252;
    }
};
