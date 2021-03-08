#pragma once
#include "Math.h"
#include "Misc.h"
#include "Rendering.h"
#include "Computer.h"

#include <memory>
#include <vector>
#include <unordered_map>


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

//#define CHUNK_LOADING_VERTEX    0x0001
//#define CHUNK_LOADED_BLOCKS     0x0002
//#define CHUNK_LOADING_BLOCKS    0x0004
//#define CHUNK_LOADED_VERTEX     0x0008
//#define CHUNK_UPLOADED          0x0010
#define CHUNK_TODELETE          0x0020
#define CHUNK_RESCAN_BLOCKS     0x0040
#define CHUNK_RESCANING_BLOCKS  0x0080

constexpr uint32 CHUNK_X = 16;
constexpr uint32 CHUNK_Y = 256;
constexpr uint32 CHUNK_Z = 16;

struct ChunkData {
    BlockType e[CHUNK_X][CHUNK_Y][CHUNK_Z] = {};
};

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
    Vec3 p = {};
    Material material;
    BlockType t = BlockType::Empty;
    uint32 defaultSpriteLocation = 254;
    uint32 spriteLocation[static_cast<uint32>(Face::Count)] = {
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
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

constexpr uint32 MAX_CHUNKS = 12000;
typedef uint32 ChunkIndex;
struct ChunkArray
{
    enum State {
        Unloaded,
        BlocksLoading,
        BlocksLoaded,
        VertexLoading,
        VertexLoaded,
        Uploaded,
    };

    bool                                    active[MAX_CHUNKS];
    ChunkData                               blocks[MAX_CHUNKS];
    Vec3Int                                 p[MAX_CHUNKS];
    std::vector<Vertex_Chunk>               faceVertices[MAX_CHUNKS];
    VertexBuffer                            vertexBuffer[MAX_CHUNKS] = {};
    uint32                                  uploadedIndexCount[MAX_CHUNKS];
    uint16                                  flags[MAX_CHUNKS];
    uint32                                  chunkCount = 0;
    std::atomic<State>                      state[MAX_CHUNKS];
    std::unordered_map<uint64, ChunkIndex>  chunkPosTable;

    bool GetChunkFromPosition(ChunkIndex& result, Vec3Int p);
    void ClearChunk(ChunkIndex index);
    ChunkIndex AddChunk(Vec3Int position);
    void SetBlocks(ChunkIndex i);
    void BuildChunkVertices(ChunkIndex i, ChunkIndex* neighbors);
    void UploadChunk(ChunkIndex i);
    void RenderChunk(ChunkIndex i);
    bool GetBlock(BlockType& result, ChunkIndex i, Vec3Int loc, bool crossChunkBoundary);
    bool GetChunk(ChunkIndex& result, Vec3Int blockP);

};
extern ChunkArray* g_chunks;

struct SetBlocks : public Job {
    ChunkIndex chunk;
    void DoThing() override;
};

struct CreateVertices : public Job {
    ChunkIndex chunk;
    ChunkIndex neighbors[8];
    void DoThing() override;
};

Vec3Int Convert_GameToChunk(Vec3 p);
Vec3Int Convert_ChunkIndexToGame(ChunkIndex i);
Vec3Int Convert_BlockToGame(ChunkIndex blockParentIndex, Vec3Int blockP);
bool Convert_GameToBlock(ChunkIndex& result, Vec3Int& outputP, Vec3Int inputP);

void SetBlockSprites();
void PreChunkRender();
void DrawBlock(Vec3 p, Color color, Vec3 scale);
void DrawBlock(Vec3 p, Color color, float scale);

int64 PositionHash(Vec3Int p);
