#pragma once
#include "Math.h"
#include "Misc.h"
#include "Rendering.h"
#include "Computer.h"
#include "Block.h"

#include <memory>
#include <vector>
#include <unordered_map>




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

#define CHUNK_FLAG_ACTIVE       BIT(1)
#define CHUNK_FLAG_MODIFIED     BIT(2)
#define CHUNK_FLAG_DIRTY        BIT(3)
#define CHUNK_FLAG_TODELETE     BIT(4)

constexpr uint32 CHUNK_X = 16;
constexpr uint32 CHUNK_Y = 256;
constexpr uint32 CHUNK_Z = 16;

struct ChunkData {
    BlockType e[CHUNK_X][CHUNK_Y][CHUNK_Z] = {};
};

extern Vec3 faceNormals[+Face::Count];


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


constexpr uint32 MAX_CHUNKS = 20000;
typedef uint32 ChunkIndex;

struct RegionSampler {

    ChunkIndex neighbors[8] = {};
    ChunkIndex center = 0;
    ChunkPos centerP = {};
    ChunkPos neighborsP[8];

    bool GetBlock(BlockType& result, Vec3Int blockRelP);
    bool RegionGather(ChunkIndex i);
    void DecrimentRefCount();
    void IncrimentRefCount();
};

struct ChunkOctreeSet
{
    bool m_isBranch = false;
    union {
        struct Branch {
            int32           m_children[2][2][2] = {};
            Range<Vec3Int>  m_range = {};
        } branch;
        struct Block {
            BlockType m_block = BlockType::Empty;
            GamePos   m_position = {};
        } block;
    };
};
class ChunkOctree {

    bool Add(const GamePos& p, const BlockType blockType, int32 index)
    {
        //check array bounds
        if (index >= m_octrees.size())
        {
            assert(false);
            return false;
        }
        const auto& octree = m_octrees[index];

        //confirm octree is branch
        if (octree.m_isBranch)
        {
            if (p.p >= octree.branch.m_range.min && p.p <= octree.branch.m_range.max)
            {
                Vec3Int indices = {};
                Vec3Int center = octree.branch.m_range.Center();
                indices.x = p.p.x > center.x;
                indices.y = p.p.y > center.y;
                indices.z = p.p.z > center.z;
                const auto childIndex = octree.branch.m_children[indices.x][indices.y][indices.z];
                if (childIndex == 0)
                {
                    //child is empty (empty leaf), fill it and return success
                    ChunkOctreeSet octreeSet = {};
                    octreeSet.m_isBranch = false;
                    octreeSet.block.m_position = p;
                    octreeSet.block.m_block    = blockType;
                    m_octrees.push_back(octreeSet);
                    m_octrees[index].branch.m_children[indices.x][indices.y][indices.z] = int32(m_octrees.size()) - 1;
                    return true;
                }

                assert(childIndex < m_octrees.size());
                if (m_octrees[childIndex].m_isBranch)
                {
                    //child is branch
                    return Add(p, blockType, childIndex);
                }
                else if (m_octrees[childIndex].block.m_block == BlockType::Empty)
                {
                    //child is empty leaf
                    m_octrees[childIndex].block.m_position = p;
                    m_octrees[childIndex].block.m_block = blockType;
                    return true;
                }
                else
                {

                    //child is filled leaf, needs branch
                    ChunkOctreeSet tempFilledLeaf = m_octrees[childIndex];
                    m_octrees[childIndex] = {};
                    m_octrees[childIndex].m_isBranch = true;
                    m_octrees[childIndex].branch.m_range.min.x = (indices.x)  ? center.x + 1 : octree.branch.m_range.min.x;
                    m_octrees[childIndex].branch.m_range.min.y = (indices.y)  ? center.y + 1 : octree.branch.m_range.min.y;
                    m_octrees[childIndex].branch.m_range.min.z = (indices.z)  ? center.z + 1 : octree.branch.m_range.min.z;
                    m_octrees[childIndex].branch.m_range.max.x = (!indices.x) ? center.x : octree.branch.m_range.max.x;
                    m_octrees[childIndex].branch.m_range.max.y = (!indices.y) ? center.y : octree.branch.m_range.max.y;
                    m_octrees[childIndex].branch.m_range.max.z = (!indices.z) ? center.z : octree.branch.m_range.max.z;
                    bool succesfullyAddedTemp = Add(tempFilledLeaf.block.m_position, tempFilledLeaf.block.m_block, childIndex);
                    return Add(p, blockType, childIndex) && succesfullyAddedTemp;
                }
            }
        }
        else
            assert(false);

        return false;
    }

public:

    bool                         m_isInitialized = false;
    std::vector <ChunkOctreeSet> m_octrees;
    void Init(ChunkPos baseChunkPosition)
    {
        Clear();
        m_octrees.reserve(30000);
        ChunkOctreeSet octree = {};
        octree.m_isBranch = true;
        octree.branch.m_range.min = ToGame(baseChunkPosition).p;
        octree.branch.m_range.max = octree.branch.m_range.min + Vec3Int({ CHUNK_X, CHUNK_Y, CHUNK_Z });
        octree.branch.m_range.max = octree.branch.m_range.max - 1;
        m_octrees.push_back(octree);
        m_isInitialized = true;
    }
    void Clear()
    {
        m_octrees.clear();
        std::vector<ChunkOctreeSet> swap3;
        m_octrees.swap(swap3);
        m_isInitialized = false;
    }

    bool Add(const GamePos& p, const BlockType blockType)
    {
        return Add(p, blockType, 0);
    }
};


typedef uint32 EntityID;
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

    //bool                      active[MAX_CHUNKS];
    ChunkData                   blocks[MAX_CHUNKS];
    ChunkPos                    p[MAX_CHUNKS] = {};
    std::vector<Vertex_Chunk>   opaqueFaceVertices[MAX_CHUNKS]      = {};
    std::vector<Vertex_Chunk>   translucentFaceVertices[MAX_CHUNKS] = {};
    VertexBuffer                opaqueVertexBuffer[MAX_CHUNKS]      = {};
    VertexBuffer                translucentVertexBuffer[MAX_CHUNKS] = {};
    uint32                      opaqueIndexCount[MAX_CHUNKS]        = {};
    uint32                      translucentIndexCount[MAX_CHUNKS]   = {};
    uint16                      height[MAX_CHUNKS]  = {};
    std::atomic<State>          state[MAX_CHUNKS]   = {};
    std::atomic<uint32>         flags[MAX_CHUNKS]   = {};
    std::atomic<int32>          refs[MAX_CHUNKS]    = {};
    std::vector<EntityID>       itemIDs[MAX_CHUNKS] = {};
    ComplexBlocks               complexBlocks[MAX_CHUNKS] = {};

    ChunkType                   chunkType[MAX_CHUNKS] = {};
    TerrainType                 terrainType[MAX_CHUNKS] = {};

    uint32 chunkCount = 0;
    std::unordered_map<uint64, ChunkIndex> chunkPosTable;
    std::vector<ChunkIndex> chunksLoadingBlocks;

    ChunkIndex highestActiveChunk;

    bool GetChunkFromPosition(ChunkIndex& result, ChunkPos p);
    void ClearChunk(ChunkIndex index);
    ChunkIndex AddChunk(ChunkPos position);
    void SetBlocks(ChunkIndex i);
    void BuildChunkVertices(RegionSampler region);
    void UploadChunk(ChunkIndex i);
    void RenderChunkOpaquePeel(ChunkIndex i);
    void RenderChunkTransparentPeel(ChunkIndex i);
    bool GetChunk(ChunkIndex& result, GamePos blockP);
    bool GetBlock(BlockType& blockType, const GamePos& blockP, ChunkIndex& chunkIndex);
    bool GetBlock(BlockType& blockType, const GamePos& blockP);
    bool SaveChunk(ChunkIndex i);
    bool LoadChunk(ChunkIndex i);
    bool Init();
    void ItemUpdate(float deltaTime);
    void Update(const ChunkPos& cameraPosition, int32 drawDistance, int32 fogDistance, MultiThreading& multiThreading);
    void CleanUp();
    void RenderChunkOpaqueChildren(const Camera* playerCamera, const int32 passCount);
};
extern ChunkArray* g_chunks;

struct SetBlocksJob : public Job {
    ChunkIndex chunk;
    void DoThing() override;
};

struct CreateVertices : public Job {
    RegionSampler region;
    void DoThing() override;
};

//TODO: Block Pos?
union VertexBlockCheck {
    struct { Vec3Int e0, e1, e2, e3, e4, e5, e6, e7; };
};

static const VertexBlockCheck vertexBlocksToCheck[+Face::Count] = {
    {//right +X
        Vec3Int({  0,  1,  0 }),//Vertex 0
                {  0,  0,  1 },

                {  0,  0,  1 }, //Vertex 1
                {  0, -1,  0 },
                {  0,  1,  0 }, //Vertex 2
                {  0,  0, -1 },

                {  0,  0, -1 }, //Vertex 3
                {  0, -1,  0 },
    },
    {//left -X
        Vec3Int({  0,  1,  0 }),//Vertex 0
                {  0,  0, -1 },

                {  0,  0, -1 }, //Vertex 1
                {  0, -1,  0 },
                {  0,  1,  0 }, //Vertex 2
                {  0,  0,  1 },

                {  0,  0,  1 }, //Vertex 3
                {  0, -1,  0 },
    },
    {//Top +Y
        Vec3Int({  1,  0,  0 }),//Vertex 0
                {  0,  0,  1 },

                {  1,  0,  0 }, //Vertex 1
                {  0,  0, -1 },
                { -1,  0,  0 }, //Vertex 2
                {  0,  0,  1 },

                { -1,  0,  0 }, //Vertex 3
                {  0,  0, -1 },
    },
    {//Bot -Y
        Vec3Int({ -1,  0,  0 }),//Vertex 0
                {  0,  0,  1 },

                { -1,  0,  0 }, //Vertex 1
                {  0,  0, -1 },
                {  1,  0,  0 }, //Vertex 2
                {  0,  0,  1 },

                {  1,  0,  0 }, //Vertex 3
                {  0,  0, -1 },
    },
    {//Front +Z
        Vec3Int({ -1,  0,  0 }),//Vertex 0
                {  0,  1,  0 },

                { -1,  0,  0 }, //Vertex 1
                {  0, -1,  0 },
                {  1,  0,  0 }, //Vertex 2
                {  0,  1,  0 },

                {  1,  0,  0 }, //Vertex 3
                {  0, -1,  0 },
    },
    {//Front -Z
        Vec3Int({  1,  0,  0 }),//Vertex 0
                {  0,  1,  0 },

                {  1,  0,  0 }, //Vertex 1
                {  0, -1,  0 },
                { -1,  0,  0 }, //Vertex 2
                {  0,  1,  0 },

                { -1,  0,  0 }, //Vertex 3
                {  0, -1,  0 },
    },
};


//Vec3Int Convert_GameToChunk(Vec3 p);
GamePos Convert_ChunkIndexToGame(ChunkIndex i);
GamePos Convert_BlockToGame(const ChunkIndex blockParentIndex, const Vec3Int blockP);
Vec3Int Convert_GameToBlock(ChunkPos& result, const GamePos& inputP);

struct Camera;
void PreOpaqueChunkRender(const Mat4& perspective, Camera* camera, uint32 passCount);
//void PreOpaqueChunkRender(const Mat4& perspective, Camera* camera);
void PreTransparentChunkRender(const Mat4& perspective, Camera* camera);

int64 PositionHash(ChunkPos p);
void AddBlock(      const GamePos& hitBlock, const BlockType block,         const ChunkIndex chunkIndex, const Vec3& forwardVector);
void RemoveBlock(   const GamePos& hitBlock, const BlockType currentBlock,  const ChunkIndex chunkIndex);
