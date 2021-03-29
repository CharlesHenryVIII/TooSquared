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
    Snow,
    Ice,
    Obsidian,
    Leaves,
    MossyCobblestone,
    Water,
    Count,
//CobWeb,
//Flower_Red,
//Flower_Yellow,
//Flower_Blue,
//Sappling,
//Chest,

};
ENUMOPS(BlockType);

enum class BiomeType : Uint8 {
    //None,
    Grassland,
    Desert,
    Savanna,
    TropicalRainforest,
    Woodland,
    SeasonalForest,
    TemperateRainforest,
    BorealForest,
    Tundra,
    Ice,
    Count
};
ENUMOPS(BiomeType);

enum class BiomeTemp : Uint8 {
    Coldest,
    Colder,
    Cold,
    Hot,
    Hotter,
    Hottest,
    Count,
};
ENUMOPS(BiomeTemp);

enum class BiomeMoist : Uint8 {
    Dryest,
    Dryer,
    Dry,
    Wet,
    Wetter,
    Wettest,
    Count,
};
ENUMOPS(BiomeMoist);

enum class TerrainType : Uint8 {
    Plains,
    Hills,
    Mountains,
    Count,
};
ENUMOPS(TerrainType);

enum class ChunkType : Uint8 {
    Ocean,
    Coastal,
    Inland,
    Count,
};
ENUMOPS(ChunkType);

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

//NOTE: Wrong do later if needed
//struct BlockPos {
//    union {
//        struct { int32 x, y, z; };
//
//        Vec2Int xy;
//        int32 e[3];
//    };
//    ChunkPos ToWorld()
//    {
//        ChunkPos result = { static_cast<int32>(x) / static_cast<int32>(CHUNK_X),
//                              static_cast<int32>(y) / static_cast<int32>(CHUNK_Y),
//                              static_cast<int32>(z) / static_cast<int32>(CHUNK_Z) };
//        return result;
//    }
//
//};


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

constexpr uint32 MAX_CHUNKS = 20000;
typedef uint32 ChunkIndex;

struct RegionSampler {

    ChunkIndex neighbors[8] = {};
    ChunkIndex center = 0;
    ChunkPos centerP;
    ChunkPos neighborsP[8];

    bool GetBlock(BlockType& result, Vec3Int blockRelP);
    bool RegionGather(ChunkIndex i);
    void DecrimentRefCount();
    void IncrimentRefCount();
};

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
    ChunkPos                                p[MAX_CHUNKS] = {};
    std::vector<Vertex_Chunk>               faceVertices[MAX_CHUNKS] = {};
    VertexBuffer                            vertexBuffer[MAX_CHUNKS] = {};
    uint32                                  uploadedIndexCount[MAX_CHUNKS] = {};
    uint16                                  flags[MAX_CHUNKS] = {};
    uint32                                  chunkCount = 0;
    uint16                                  height[MAX_CHUNKS] = {};
    std::atomic<State>                      state[MAX_CHUNKS] = {};
    std::unordered_map<uint64, ChunkIndex>  chunkPosTable;
    std::atomic<int32>                      refs[MAX_CHUNKS] = {};
    ChunkType                               chunkType[MAX_CHUNKS] = {};
    TerrainType                             terrainType[MAX_CHUNKS] = {};

    bool GetChunkFromPosition(ChunkIndex& result, ChunkPos p);
    void ClearChunk(ChunkIndex index);
    ChunkIndex AddChunk(ChunkPos position);
    void SetBlocks(ChunkIndex i);
    void BuildChunkVertices(RegionSampler region);
    void UploadChunk(ChunkIndex i);
    void RenderChunk(ChunkIndex i);
    bool GetChunk(ChunkIndex& result, GamePos blockP);

};
extern ChunkArray* g_chunks;

struct SetBlocks : public Job {
    ChunkIndex chunk;
    void DoThing() override;
};

struct CreateVertices : public Job {
    RegionSampler region;
    void DoThing() override;
};


//Vec3Int Convert_GameToChunk(Vec3 p);
GamePos Convert_ChunkIndexToGame(ChunkIndex i);
GamePos Convert_BlockToGame(ChunkIndex blockParentIndex, Vec3Int blockP);
Vec3Int Convert_GameToBlock(ChunkPos& result, GamePos inputP);

void SetBlockSprites();
void PreChunkRender(const Mat4& perspective);
void DrawBlock(WorldPos p, Color color, Vec3 scale, const Mat4& perspective);
void DrawBlock(WorldPos p, Color color, float scale, const Mat4& perspective);

int64 PositionHash(ChunkPos p);
bool RayVsChunk(const Ray& ray, ChunkIndex chunkIndex, GamePos& block, float& distance, Vec3& normal);
void SetBlock(GamePos hitBlock, Vec3 hitNormal, BlockType setBlockType);
