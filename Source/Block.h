#pragma once
#include "Math.h"
#include "Misc.h"
#include "Rendering.h"

#include <vector>

enum class BlockType : uint8 {
    Empty,
    Grass,
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
    Count,
};
ENUMOPS(BlockType);

//CobWeb,
//Flower_Red,
//Flower_Yellow,
//Flower_Blue,
//Sappling,
//Chest,


constexpr uint32 CHUNK_X = 16;
constexpr uint32 CHUNK_Y = 256;
constexpr uint32 CHUNK_Z = 16;

struct Chunk {
    BlockType arr[CHUNK_X][CHUNK_Y][CHUNK_Z] = {};
    Vec3Int p = {};
	std::vector<Vertex> faceVertices;
    std::vector<uint32> indices;
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;

    void SetBlockTypes()
    {
        BlockType options[] = {
            BlockType::Empty,
            BlockType::Grass,
            BlockType::Stone,
            BlockType::IronBlock,
        };
        for (int32 x = 0; x < CHUNK_X; x++)
        {
			for (int32 y = 0; y < CHUNK_Y; y++)
			{
                for (int32 z = 0; z < CHUNK_Z; z++)
                {
                    int32 random = RandomUI(0, arrsize(options));
                    arr[x][y][z] = options[random];
				}
			}
        }
    }
};

void UploadChunk(Chunk* chunk);
void BuildChunkVertices(Chunk* chunk);
void RenderChunk(Chunk* chunk);

enum class Face : uint32 {
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
    Vec3 p = {};
    Material material;
    BlockType t = BlockType::Empty;
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
        material.ambient = {  0.1f, 0.1f, 0.1f };
        material.diffuse = {  1.0f, 1.0f, 1.0f };
        material.specular = {     0.1f,   0.1f,   0.1f  };
        material.shininess =  32;//0;

        defaultSpriteLocation = 3;
        spriteLocation[+Face::Top] = 0;
        spriteLocation[+Face::Bot] = 2;
    }
};

struct Stone : public Block {
    Stone()
    {
        material.ambient = { 0.02f, 0.02f, 0.02f };
        material.diffuse = { 1.0f, 1.0f, 1.0f };
        material.specular = { 0.4f, 0.4f,  0.4f };
        material.shininess = 32;//0.78125f;

        defaultSpriteLocation = 1;
    }
};

struct IronBlock : public Block {
    IronBlock()
    {
        material.ambient = {0.19225f, 0.19225f, 0.19225f };
        material.diffuse = {0.50754f, 0.50754f, 0.50754f };
        material.specular = {0.508273f, 0.508273f, 0.508273f };
        material.shininess = 32;//0.4f;

        defaultSpriteLocation = 22;
    }
};

struct FireBlock : public Block {
    FireBlock()
    {
        material.ambient = { 1.0f, 1.0f, 1.0f };
        material.diffuse = { 1.0f, 1.0f, 1.0f };
        material.specular = {   0,    0,    0 };
        material.shininess = 32;

        defaultSpriteLocation = 252;
    }
};
