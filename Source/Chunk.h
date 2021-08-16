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

#define OCTREE 1
struct ChunkOctreeSet
{
    bool            m_isBranch    = false;
    int32           m_branch_children[2][2][2] = {};
    Range<Vec3Int>  m_branch_range = {};

    BlockType       m_block       = BlockType::Empty;
    Vec3Int         m_position    = {};
};
struct ChunkOctree {

    bool                         m_isInitialized = false;
    std::vector <ChunkOctreeSet> m_octrees;
    void Init()
    {
        m_octrees.clear();
        ChunkOctreeSet octree;
        octree.m_isBranch = true;
        octree.m_branch_range.min = {};
        octree.m_branch_range.max = { CHUNK_X, CHUNK_Y, CHUNK_Z };
        octree.m_branch_range.max = octree.m_branch_range.max - 1;
        m_octrees.push_back(octree);
        m_isInitialized = true;
    }
    bool Add(const Vec3Int& p, const BlockType blockType, int32 index = 0)
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
            if (p >= octree.m_branch_range.min && p <= octree.m_branch_range.max)
            {
                Vec3Int indices = {};
                Vec3Int center = octree.m_branch_range.Center();
                indices.x = p.x > center.x;
                indices.y = p.y > center.y;
                indices.z = p.z > center.z;
                const auto childIndex = octree.m_branch_children[indices.x][indices.y][indices.z];
                if (childIndex == 0)
                {
                    //child is empty (empty leaf), fill it and return success
                    ChunkOctreeSet octreeSet = {};
                    octreeSet.m_isBranch = false;
                    octreeSet.m_position = p;
                    octreeSet.m_block    = blockType;
                    m_octrees.push_back(octreeSet);
                    m_octrees[index].m_branch_children[indices.x][indices.y][indices.z] = int32(m_octrees.size()) - 1;
                    return true;
                }

                assert(childIndex < m_octrees.size());
                if (m_octrees[childIndex].m_isBranch)
                {
                    //child is branch
                    return Add(p, blockType, childIndex);
                }
                else if (m_octrees[childIndex].m_block == BlockType::Empty)
                {
                    //child is empty leaf
                    m_octrees[childIndex].m_position = p;
                    m_octrees[childIndex].m_block = blockType;
                    return true;
                }
                else
                {

                    //child is filled leaf, needs branch
                    ChunkOctreeSet tempFilledLeaf = m_octrees[childIndex];
                    m_octrees[childIndex] = {};
                    m_octrees[childIndex].m_isBranch = true;
#if 1
                    m_octrees[childIndex].m_branch_range.min.x = (indices.x)  ? center.x + 1 : octree.m_branch_range.min.x;
                    m_octrees[childIndex].m_branch_range.min.y = (indices.y)  ? center.y + 1 : octree.m_branch_range.min.y;
                    m_octrees[childIndex].m_branch_range.min.z = (indices.z)  ? center.z + 1 : octree.m_branch_range.min.z;
                    m_octrees[childIndex].m_branch_range.max.x = (!indices.x) ? center.x : octree.m_branch_range.max.x;
                    m_octrees[childIndex].m_branch_range.max.y = (!indices.y) ? center.y : octree.m_branch_range.max.y;
                    m_octrees[childIndex].m_branch_range.max.z = (!indices.z) ? center.z : octree.m_branch_range.max.z;
#else
                    Vec3Int halfDiff = (octree.m_branch_range.max - octree.m_branch_range.min) / 2.0f;
                    child.m_branch_range.min = octree.m_branch_range.min + HadamardProduct(indices, Ceiling(halfDiff));
                    boxbox.min = box.min + HadamardProduct(offsets, Ceiling(halfDiff));
                    boxbox.max = box.max - HadamardProduct(Vec3({ 1.0f, 1.0f, 1.0f }) - offsets, Ceiling(halfDiff));
#endif
                    bool succesfullyAddedTemp = Add(tempFilledLeaf.m_position, tempFilledLeaf.m_block, childIndex);
                    return Add(p, blockType, childIndex) && succesfullyAddedTemp;
                }
            }
        }
        else
            assert(false);

        return false;
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
    ChunkOctree                 octree[MAX_CHUNKS]  = {};
    uint16                      height[MAX_CHUNKS]  = {};
    std::atomic<State>          state[MAX_CHUNKS]   = {};
    std::atomic<uint32>         flags[MAX_CHUNKS]   = {};
    std::atomic<int32>          refs[MAX_CHUNKS]    = {};
    std::vector<EntityID>       itemIDs[MAX_CHUNKS] = {};

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
    void RenderOpaqueChunk(ChunkIndex i);
    void RenderTransparentChunk(ChunkIndex i);
    bool GetChunk(ChunkIndex& result, GamePos blockP);
    bool GetBlock(BlockType& blockType, const GamePos& blockP);
    bool SaveChunk(ChunkIndex i);
    bool LoadChunk(ChunkIndex i);
    bool Init();
    void Update(float deltaTime);
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


extern std::vector<WorldPos> cubesToDraw;


//Vec3Int Convert_GameToChunk(Vec3 p);
GamePos Convert_ChunkIndexToGame(ChunkIndex i);
GamePos Convert_BlockToGame(const ChunkIndex blockParentIndex, const Vec3Int blockP);
Vec3Int Convert_GameToBlock(ChunkPos& result, const GamePos& inputP);

struct Camera;
void PreOpaqueChunkRender(const Mat4& perspective, Camera* camera);
void PreTransparentChunkRender(const Mat4& perspective, Camera* camera);

int64 PositionHash(ChunkPos p);
bool RayVsChunk(const Ray& ray, const ChunkIndex& chunkIndex, GamePos& block, float& distance, Vec3& normal);
void SetBlock(GamePos hitBlock, BlockType setBlockType);
