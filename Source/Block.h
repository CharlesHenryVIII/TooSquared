#pragma once
#include "Math.h"
#include "Misc.h"
#include "Rendering.h"

#include <vector>

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
    Count,
};
ENUMOPS(BlockType);

//CobWeb,
//Flower_Red,
//Flower_Yellow,
//Flower_Blue,
//Sappling,
//Chest,

#define CHUNK_LOADING		0x0001
#define CHUNK_LOADED        0x0002
#define CHUNK_MODIFIED      0x0004
#define CHUNK_NOTUPLOADED	0x0008

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
    SDL_atomic_t flags = { CHUNK_MODIFIED | CHUNK_NOTUPLOADED };

    void SetBlocks();
	void BuildChunkVertices();
	void UploadChunk();
	void RenderChunk();
    Vec3Int BlockPosition();
};


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

struct Dirt : public Block {

    Dirt()
    {
        material.ambient = {  0.1f, 0.1f, 0.1f };
        material.diffuse = {  1.0f, 1.0f, 1.0f };
        material.specular = {     0.1f,   0.1f,   0.1f  };
        material.shininess =  32;//0;

        defaultSpriteLocation = 2;
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

Vec3Int ToChunkPosition(Vec3 p);
