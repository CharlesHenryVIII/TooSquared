#include "Block.h"
#include "WinInterop.h"
#include "Noise.h"
#include "Computer.h"
#include "Misc.h"
#include "Entity.h"

#include <unordered_map>

ChunkArray* g_chunks;

BiomeType BiomeTable[+BiomeMoist::Count][+BiomeTemp::Count] = {
    //COLDEST        //COLDER            //COLD                   //HOT                           //HOTTER                        //HOTTEST
    { BiomeType::Ice, BiomeType::Tundra, BiomeType::Grassland,    BiomeType::Desert,              BiomeType::Desert,              BiomeType::Desert },              //DRYEST
    { BiomeType::Ice, BiomeType::Tundra, BiomeType::Grassland,    BiomeType::Desert,              BiomeType::Desert,              BiomeType::Desert },              //DRYER
    { BiomeType::Ice, BiomeType::Tundra, BiomeType::Woodland,     BiomeType::Woodland,            BiomeType::Savanna,             BiomeType::Savanna },             //DRY
    { BiomeType::Ice, BiomeType::Tundra, BiomeType::BorealForest, BiomeType::Woodland,            BiomeType::Savanna,             BiomeType::Savanna },             //WET
    { BiomeType::Ice, BiomeType::Tundra, BiomeType::BorealForest, BiomeType::SeasonalForest,      BiomeType::TropicalRainforest,  BiomeType::TropicalRainforest },  //WETTER
    { BiomeType::Ice, BiomeType::Tundra, BiomeType::BorealForest, BiomeType::TemperateRainforest, BiomeType::TropicalRainforest,  BiomeType::TropicalRainforest },  //WETTEST
};

BiomeType GetBiomeType(BiomeTemp temp, BiomeMoist moist)
{
    return BiomeTable[+moist][+temp];
}



int64 PositionHash(ChunkPos p)
{
    union {
        struct {
            int x, z;
        };
        int64 r;
    };
    x = p.p.x;
    z = p.p.z;
    return r;
}

bool ChunkArray::GetChunkFromPosition(ChunkIndex& result, ChunkPos p)
{
    assert(OnMainThread());
    int64 hash = PositionHash(p);
    auto it = chunkPosTable.find(hash);
    if (it != chunkPosTable.end())
    {
        result = it->second;
        return true;
    }
    return false;
}

void ChunkArray::ClearChunk(ChunkIndex index)
{
    SaveChunk(index);
    auto it = chunkPosTable.find(PositionHash(p[index]));
    assert(it != chunkPosTable.end());
    chunkPosTable.erase(it);

    active[index] = {};
    blocks[index] = {};
    p[index] = {};
    height[index] = {};

    faceVertices[index].clear();
    std::vector<Vertex_Chunk> swapping;
    faceVertices[index].swap(swapping);
    state[index] = {};
    uploadedIndexCount[index] = {};
    flags[index] = {};
    refs[index] = {};

    if (index == highestActiveChunk)
    {
        ChunkIndex newHighest = 0;
        for (ChunkIndex i = 0; i < highestActiveChunk; i++)
        {
            if (active[i])
                newHighest = i;
        }
        highestActiveChunk = newHighest;
    }

    g_chunks->chunkCount--;

    //delete vertexBuffer[index];
    //TODO: FIX
    //vertexBuffer[index] = {};

}

ChunkIndex ChunkArray::AddChunk(ChunkPos position)
{
    assert(OnMainThread());
    for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
    {
        if (!active[i])
        {
            active[i] = true;
            g_chunks->chunkCount++;
            g_chunks->chunkPosTable[PositionHash(position)] = i;
            g_chunks->p[i] = position;
            highestActiveChunk = Max(i, highestActiveChunk);
            return i;
        }
    }
    return uint32(-1);
}

struct VertexFace {
    Vertex_Chunk a,b,c,d;
};

Vec3 faceNormals[+Face::Count] = {

{  1.0f,  0.0f,  0.0f },
{ -1.0f,  0.0f,  0.0f },
{  0.0f,  1.0f,  0.0f },
{  0.0f, -1.0f,  0.0f },
{  0.0f,  0.0f,  1.0f },
{  0.0f,  0.0f, -1.0f },
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

BlockSprites blockSprites[+BlockType::Count];

void SetMultipleBlockSprites(BlockType bt, uint32 v)
{
    for (uint32 i = 0; i < +Face::Count; i++)
    {
        blockSprites[+bt].faceSprites[i] = v;
    }
}
void SetBlockSprites()
{
    SetMultipleBlockSprites(BlockType::Dirt, 2);
    SetMultipleBlockSprites(BlockType::Grass, 3);
    blockSprites[+BlockType::Grass].faceSprites[+Face::Top] = 0;
    blockSprites[+BlockType::Grass].faceSprites[+Face::Bot] = 2;

    SetMultipleBlockSprites(BlockType::Stone, 1);
    SetMultipleBlockSprites(BlockType::IronBlock, 22);
    SetMultipleBlockSprites(BlockType::GoldBlock, 23);
    SetMultipleBlockSprites(BlockType::DiamondBlock, 24);
    SetMultipleBlockSprites(BlockType::Sand, 18);
    SetMultipleBlockSprites(BlockType::Snow, 66);
    SetMultipleBlockSprites(BlockType::Wood, 20);
    blockSprites[+BlockType::Wood].faceSprites[+Face::Top] = 21;
    blockSprites[+BlockType::Wood].faceSprites[+Face::Bot] = 21;
    SetMultipleBlockSprites(BlockType::Ice, 67);
    SetMultipleBlockSprites(BlockType::Obsidian, 37);
    SetMultipleBlockSprites(BlockType::Leaves, 53);
    //SetMultipleBlockSprites(BlockType::Leaves, 52);
    SetMultipleBlockSprites(BlockType::MossyCobblestone, 36);
    SetMultipleBlockSprites(BlockType::TNT, 8);
    blockSprites[+BlockType::TNT].faceSprites[+Face::Top] = 9;
    blockSprites[+BlockType::TNT].faceSprites[+Face::Bot] = 10;
    SetMultipleBlockSprites(BlockType::Water, 255);
    SetMultipleBlockSprites(BlockType::Bedrock, 17);
}

//TODO: Move to ChunkPos
GamePos Convert_ChunkIndexToGame(ChunkIndex i)
{
    if (g_chunks->active[i])
        return ToGame(g_chunks->p[i]);
    return {};
}

Vec3Int Convert_GameToChunk(Vec3 p)
{
    Vec3Int result = { static_cast<int32>(p.x) / static_cast<int32>(CHUNK_X),
                       static_cast<int32>(p.y) / static_cast<int32>(CHUNK_Y),
                       static_cast<int32>(p.z) / static_cast<int32>(CHUNK_Z) };
    return result;
}


BiomeType RandomBiome(ChunkPos v)
{
    uint32 first = *(uint32*)(&v.p.x);
    first ^= first << 13;
    first ^= first >> 17;
    first ^= first << 5;

    uint32 second = *(uint32*)(&v.p.y);
    second ^= second << 13;
    second ^= second >> 17;
    second ^= second << 5;
    BiomeType result = static_cast<BiomeType>(((first ^ second) * (+BiomeType::Count - 1)) / UINT_MAX);
    return static_cast<BiomeType>((+result) + 1);
}

uint32 chunkTypes[+BiomeType::Count] = {};
std::unordered_map<int64, BiomeType> s_biomePoints;
struct BiomePoints {
    ChunkPos pos;
    BiomeType biome;
};

#define stb_lerp(t,a,b)               ( (a) + (t) * (float) ((b)-(a)) )
#define stb_unlerp(t,a,b)             ( ((t) - (a)) / (float) ((b) - (a)) )
#define stb_linear_remap(t,a,b,c,d)   stb_lerp(stb_unlerp(t,a,b),c,d)

const uint32 averageBlockHeight = 72;

float octave_multiplier[8] =
{
   1.01f,
   1.03f,
   1.052f,
   1.021f,
   1.0057f,
   1.111f,
   1.089f,
   1.157f,
};

float stb_ComputeHeightFieldOctave(float ns, int o, float weight)
{
   float scale,heavier,sign;
   scale = (float) (1 << o) * octave_multiplier[o];
   sign = (ns < 0 ? -1.0f : 1.0f);
   ns = (float) fabs(ns);
   heavier = ns*ns*ns*ns*4*sign;
   return scale/2 * stb_lerp(weight, ns, heavier) / 2;
}

float stb_ComputeHeightField(int x, int y, float weight)
{
   int o;
   float ht = (float)averageBlockHeight;
   for (o=3; o < 8; ++o) {
      float scale = (float) (1 << o) * octave_multiplier[o];
      float ns = Perlin3D({ x / scale, o * 2.0f, y / scale }, { 256, 256, 256 });
      ht += stb_ComputeHeightFieldOctave(ns, o, weight);
   }
   return ht;
}

float stb_ComputeHeightFieldDelta(int x, int y, float weight)
{
   int o;
   float ht = 0;
   for (o=0; o < 3; ++o) {
       float ns = (BigNoise({ x, y }, o, 8348 + o * 23787) / 32678.0f - 1.0f) / 2.0f;
      ht += stb_ComputeHeightFieldOctave(ns, o, weight);
   }
   return ht;
}



#define HEIGHT_UNBREAKABLE  1U
#define HEIGHT_MIN_WATER    20U
#define HEIGHT_MAX_WATER    70U
uint64 s_worldSeed = 0;
const int32 generalNoiseOffset = HEIGHT_MIN_WATER;
const float generalPerlinScale = 0.01f;
const NoiseParams seaFloorParams = {
    .numOfOctaves = 8,
    .freq = 0.17f,
    //.freq = 0.3f,
    .weight = 1.0f,
    //.gainFactor = 0.8f,
    .gainFactor = 1.0f,
};


uint32 GetLandHeight(WorldPos chunkP, Vec3Int blockP, const NoiseParams& params, float scale, int32 noiseOffset, uint32 lowerClamp, uint32 upperClamp)
{
    Vec2 lookupLoc = { (chunkP.p.x + (blockP.x % CHUNK_X)) * scale, (chunkP.p.z + (blockP.z % CHUNK_Z)) * scale };
    return Clamp<uint32>(uint32(noiseOffset + CHUNK_Y * Perlin2D(lookupLoc, params)), lowerClamp, upperClamp);
}

std::vector<WorldPos> cubesToDraw;
thread_local Vec2 s_setBlocksCheckingPoint = {};
int32 SortVoronoiLocations(const void* a, const void* b)
{
    assert(a);
    assert(b);

    Vec2 _a = *((Vec2*)a);
    Vec2 _b = *((Vec2*)b);

    float aDist = Distance(_a, s_setBlocksCheckingPoint);
    float bDist = Distance(_b, s_setBlocksCheckingPoint);
    return aDist > bDist;
}

//  Vec2 lookupLoc = { (chunkGamePos.p.x + x) * perlinScale, (chunkGamePos.p.z + z) * perlinScale };
uint32 GetPlainsHeight(GamePos p, float perlinScale, int32 noiseOffset, uint32 lowDetailHeight)
{
    //float heightRatio = 1 / 10.0f;
    float frequency = 0.1f;
    //float perlinScale = 1.0f;
    NoiseParams np = {
        .numOfOctaves = 2,
        .freq = 0.1f,
        .weight = 1.0f,
        .gainFactor = 0.5f,
    };

    Vec2 lookupLoc = Vec2({ float(p.p.x), float(p.p.z) }) * perlinScale;
    uint32 deltaHeight = Clamp<uint32>(uint32(noiseOffset + CHUNK_Y * Perlin2D(lookupLoc * frequency, np)), 0, CHUNK_Y - lowDetailHeight);
    return deltaHeight;
}

uint32 GetHillsHeight(GamePos p, float perlinScale, int32 noiseOffset, uint32 lowDetailHeight)
{
    float frequency = 1.0f;
    NoiseParams np = {
        .numOfOctaves = 2,
        .freq = 0.2f,
        .weight = 1.0f,
        .gainFactor = 0.5f,
    };

    Vec2 lookupLoc = Vec2({ float(p.p.x), float(p.p.z) }) * perlinScale;
    uint32 deltaHeight = Clamp<uint32>(uint32(noiseOffset + CHUNK_Y * Perlin2D(lookupLoc * frequency, np)), 0, CHUNK_Y - lowDetailHeight);
    return deltaHeight;
}

uint32 GetMountainsHeight(GamePos p, float perlinScale, int32 noiseOffset, uint32 lowDetailHeight)
{
    float frequency = 2.0f;
    NoiseParams np = {
        .numOfOctaves = 4,
        .freq = 0.2f,
        .weight = 1.0f,
        .gainFactor = 0.5f,
    };

    Vec2 lookupLoc = Vec2({ float(p.p.x), float(p.p.z) }) * perlinScale;
    uint32 deltaHeight = Clamp<uint32>(uint32(noiseOffset + CHUNK_Y * Perlin2D(lookupLoc * frequency, np)), 0, CHUNK_Y - lowDetailHeight);
    return deltaHeight;
}

uint32 QueryLandHeight(ChunkPos chunkLocation, int32 xQueryLocation, int32 zQueryLocation)
{
    VoronoiRegion region;
    region.BuildRegion(chunkLocation, s_worldSeed);
    Vec3Int block_queryLocation = { xQueryLocation % CHUNK_X, 0, zQueryLocation % CHUNK_Z };

    uint32 waterVsLandHeight = GetLandHeight(ToWorld(chunkLocation), block_queryLocation, seaFloorParams, generalPerlinScale, generalNoiseOffset, HEIGHT_MIN_WATER, CHUNK_Y);
    if (!(waterVsLandHeight < HEIGHT_MAX_WATER - 2 && waterVsLandHeight < HEIGHT_MAX_WATER + 1))
    {
        GamePos blockP;
        blockP.p = ToGame(chunkLocation).p + block_queryLocation;//Vec3Int({xQueryLocation, 0, zQueryLocation });

        uint32 terrainFunctions[] = {
            GetPlainsHeight(blockP, generalPerlinScale, generalNoiseOffset, waterVsLandHeight),
            GetHillsHeight(blockP, generalPerlinScale, generalNoiseOffset, waterVsLandHeight),
            GetMountainsHeight(blockP, generalPerlinScale, generalNoiseOffset, waterVsLandHeight),
        };
        static_assert(arrsize(terrainFunctions) == +TerrainType::Count);

        float noiseHeightScale = 0.05f;

        float gBlurWeights[5][5] = {
          1,  4,  7,  4,  1,
          4, 16, 26, 16,  4,
          7, 26, 41, 26,  7,
          4, 16, 26, 16,  4,
          1,  4,  7,  4,  1,
        };
        float gWeightTotal = 0;
        for (int32 y = 0; y < sizeof(gBlurWeights) / sizeof(gBlurWeights[0]); y++)
        {
            for (int32 x = 0; x < sizeof(gBlurWeights[0]) / sizeof(gBlurWeights[0][0]); x++)
            {
                gWeightTotal += gBlurWeights[y][x];
            }
        }
        for (int32 y = 0; y < sizeof(gBlurWeights) / sizeof(gBlurWeights[0]); y++)
        {
            for (int32 x = 0; x < sizeof(gBlurWeights[0]) / sizeof(gBlurWeights[0][0]); x++)
            {
                gBlurWeights[y][x] /= gWeightTotal;
            }
        }

        float newHeight = 0;
        for (int32 y = 0; y < sizeof(gBlurWeights) / sizeof(gBlurWeights[0]); y++)
        {
            for (int32 x = 0; x < sizeof(gBlurWeights[0]) / sizeof(gBlurWeights[0][0]); x++)
            {
                Vec3Int blockOffset = {};
                blockOffset.x = x - (sizeof(gBlurWeights[0]) / sizeof(gBlurWeights[0][0])) / 2;
                blockOffset.z = y - (sizeof(gBlurWeights) / sizeof(gBlurWeights[0])) / 2;
                GamePos checkBlockP = {};
                checkBlockP.p = blockP.p + blockOffset;
                VoronoiCell* checkCell = region.GetCell(checkBlockP);
                assert(checkCell);
                if (checkCell)
                {
                    uint32 hashValue = checkCell->GetHash(s_worldSeed);
                    TerrainType neighborType = TerrainType(hashValue % +TerrainType::Count);
                    float cellHeight = float(terrainFunctions[+neighborType]) * noiseHeightScale;
                    newHeight += cellHeight * gBlurWeights[y][x];
                }
            }
        }

        waterVsLandHeight += uint32(newHeight);
        //________

        assert(waterVsLandHeight > HEIGHT_UNBREAKABLE);
        assert(waterVsLandHeight < CHUNK_Y);
        waterVsLandHeight = Clamp(waterVsLandHeight, HEIGHT_UNBREAKABLE + 1, CHUNK_Y - 1);
    }
    return waterVsLandHeight;
}
uint32 QueryLandHeight(GamePos queryLocation)
{
    return QueryLandHeight(ToChunk(queryLocation), queryLocation.p.x, queryLocation.p.z);
}

bool TreeIsAtLocation(GamePos p, float perlinScale, int32 noiseOffset)
{
    float frequency = 10.0f;
    //float frequency = 0.000001f;
    NoiseParams np = {
        .numOfOctaves = 2,
        .freq = 100.0f,
        .weight = 1.0f,
        .gainFactor = 0.5f,
    };

    Vec2 lookupLoc = Vec2({ float(p.p.x), float(p.p.z) }) * perlinScale;
    bool treeIsThere = Perlin2D(lookupLoc * frequency, np) > 0.6f;
    return treeIsThere;
}

void TreeGenerationProcess(uint32& height, ChunkIndex index, int32 chunkx, int32 chunky, int32 chunkz, const GamePos& p)
{
    const Vec3Int maxSize = { 5, 6, 5 };
    const int32 logHeight = 5;
    const Range<Vec3Int>  branchRange = { { -2, 3, -2}, { 2, 5, 2 } };
    float perlinScale = 0.01f;

    if (p.p == Vec3Int({ -114, 0, 23 }))
        int32 DEBUG_TEST = 10;

    if (height >= HEIGHT_MAX_WATER + 1 && TreeIsAtLocation(p, perlinScale, HEIGHT_MIN_WATER) && height < CHUNK_Y)
    {//determin what will be in the 
        int32 treeY = 0;
        for (treeY; treeY + height < CHUNK_Y && treeY < logHeight; treeY++)
        {
            g_chunks->blocks[index].e[chunkx][chunky + treeY][chunkz] = BlockType::Wood;
        }
        int32 leafy = 0;
        for (int32 leafx = branchRange.min.x; leafx <= branchRange.max.x; leafx++)
        {
            for (leafy = branchRange.min.y; leafy <= branchRange.max.y && leafy + chunky < CHUNK_Y; leafy++)
            {
                for (int32 leafz = branchRange.min.z; leafz <= branchRange.max.z; leafz++)
                {
                    if (leafx + chunkx < CHUNK_X && leafy + treeY + height < CHUNK_Y && leafz + chunkz < CHUNK_Z)
                        if (leafx + chunkx >= 0 && leafy + chunky >= 0 && leafz + chunkz >= 0)
                        {
                            BlockType& block = g_chunks->blocks[index].e[chunkx + leafx][chunky + leafy][chunkz + leafz];
                            if (block == BlockType::Empty)
                                block = BlockType::Leaves;
                        }
                }
            }
        }
        height += treeY + leafy;
        assert(height < CHUNK_Y);
    }
    else
    {
        uint32 maxHeight = 0;
        for (int32 x = branchRange.min.x; x <= branchRange.max.x; x++)
        {
            for (int32 z = branchRange.min.z; z <= branchRange.max.z; z++)
            {
                GamePos checkLocation = {};
                checkLocation.p = p.p + Vec3Int({ x, 0, z });
                if (TreeIsAtLocation(checkLocation, perlinScale, HEIGHT_MIN_WATER))
                {
                    if (GetLandHeight(ToWorld(ToChunk(checkLocation)), checkLocation.p, seaFloorParams, generalPerlinScale, generalNoiseOffset, HEIGHT_MIN_WATER, CHUNK_Y) < HEIGHT_MAX_WATER + 1)
                        continue;

                    uint32 landHeight = QueryLandHeight(checkLocation);
                    for (int32 y = branchRange.min.y; y <= branchRange.max.y && landHeight + y < CHUNK_Y; y++)
                    {
                        uint32 newY = landHeight + y;
                        if (newY < 0)
                            continue;
                        BlockType& block = g_chunks->blocks[index].e[chunkx][newY][chunkz];

                        if (block == BlockType::Empty)
                        {
                            block = BlockType::Leaves;
                            maxHeight = Max(maxHeight, newY + 1);
                        }
                    }
                }
            }
        }
        height = Max(height, maxHeight);
        assert(height < CHUNK_Y);
    }
}

void ChunkArray::SetBlocks(ChunkIndex chunkIndex)
{
#define STB_METHOD 0
#define VORONOI 13

#if VORONOI == 13

    ChunkPos chunkPos = g_chunks->p[chunkIndex];
    WorldPos chunkGamePos = ToWorld(chunkPos);
    uint16 heightMap[CHUNK_X][CHUNK_Z] = {};

    //set the base unbreakable layer
    {
        for (uint32 x = 0; x < CHUNK_X; x++)
        {
            for (uint32 y = 0; y < HEIGHT_UNBREAKABLE; y++)
            {
                for (uint32 z = 0; z < CHUNK_Z; z++)
                {
                    blocks[chunkIndex].e[x][y][z] = BlockType::Bedrock;
                    heightMap[x][z] = HEIGHT_UNBREAKABLE;
                }
            }
        }
    }

    //set the stone in between water and bedrock
    {
        for (uint32 x = 0; x < CHUNK_X; x++)
        {
            for (uint32 y = HEIGHT_UNBREAKABLE; y < HEIGHT_MIN_WATER; y++)
            {
                for (uint32 z = 0; z < CHUNK_Z; z++)
                {
                    blocks[chunkIndex].e[x][y][z] = BlockType::Stone;
                    heightMap[x][z] = HEIGHT_MIN_WATER;
                }
            }
        }
    }

    bool TEST_firstTimeThrough = true;
    int32 cellSize = 10; //size of the area for sampling positions
    double terrainWeights[CHUNK_X][CHUNK_Z][+TerrainType::Count] = {};

    VoronoiRegion region;
    region.BuildRegion(chunkPos, s_worldSeed);

    //set the majority of the blocks
    {
        uint32 newHeight = 0;
        uint32 waterVsLandHeight = 0;
        {
            for (int32 x = 0; x < CHUNK_X; x++)
            {
                for (int32 z = 0; z < CHUNK_Z; z++)
                {
                    GamePos blockP = Convert_BlockToGame(chunkIndex, { x, 0, z });

                    //Fill with stone from HEIGHT_MIN_WATER to the determined height - 3
                    waterVsLandHeight = GetLandHeight(chunkGamePos, { int32(x), 0, int32(z) }, seaFloorParams, generalPerlinScale, generalNoiseOffset, HEIGHT_MIN_WATER, CHUNK_Y);//HEIGHT_MIN_WATER - 2);
                    //waterVsLandHeight = Clamp<uint32>(uint32(noiseOffset + CHUNK_Y * Perlin2D({ (chunkGamePos.p.x + x) * perlinScale, (chunkGamePos.p.z + z) * perlinScale }, seaFloorParams)), HEIGHT_MIN_WATER, HEIGHT_MAX_WATER - 2);
                    uint32 y = HEIGHT_UNBREAKABLE;
                    for (y; y < Min(waterVsLandHeight, HEIGHT_MIN_WATER) - 3; y++)
                    {
                        blocks[chunkIndex].e[x][y][z] = BlockType::Stone;
                    }

                    //Fill the last 3 high blocks with either stone or sand depending on if the location is max height or not

                    if (waterVsLandHeight < HEIGHT_MAX_WATER - 2)
                    {//ocean

                        for (y; y < waterVsLandHeight - 3; y++)
                        {
                            blocks[chunkIndex].e[x][y][z] = BlockType::Stone;
                        }

                        for (y; y < waterVsLandHeight; y++)
                        {
                            blocks[chunkIndex].e[x][y][z] = BlockType::Sand;
                        }
                    }
                    else if (waterVsLandHeight < HEIGHT_MAX_WATER + 1)
                    {//coastal

                        for (y; y < waterVsLandHeight - 2; y++)
                        {
                            blocks[chunkIndex].e[x][y][z] = BlockType::Stone;
                        }

                        for (y; y < waterVsLandHeight; y++)
                        {
                            blocks[chunkIndex].e[x][y][z] = BlockType::Sand;
                        }
                    }
                    else
                    {//land

                        waterVsLandHeight = QueryLandHeight(chunkPos, x, z);

                        for (y; y < waterVsLandHeight - 3; y++)
                        {
                            blocks[chunkIndex].e[x][y][z] = BlockType::Stone;
                        }

                        for (y; y < waterVsLandHeight - 1; y++)
                        {
                            blocks[chunkIndex].e[x][y][z] = BlockType::Dirt;
                        }

                        for (y; y < waterVsLandHeight; y++)
                        {
#if 1
                            blocks[chunkIndex].e[x][y][z] = BlockType::Grass;
#else
                            if (counts == 0)
                                blocks[chunkIndex].e[x][y][z] = BlockType::Grass;
                            else if (counts == 1)
                                blocks[chunkIndex].e[x][y][z] = BlockType::Stone;
                            else if (counts == 2)
                                blocks[chunkIndex].e[x][y][z] = BlockType::TNT;
                            else if (counts == 3)
                                blocks[chunkIndex].e[x][y][z] = BlockType::Snow;
                            else if (counts == 4)
                                blocks[chunkIndex].e[x][y][z] = BlockType::Wood;
                            else if (counts == 5)
                                blocks[chunkIndex].e[x][y][z] = BlockType::Obsidian;
                            else if (counts > 5)
                                blocks[chunkIndex].e[x][y][z] = BlockType::DiamondBlock;
#endif
                        }
                    }
                    TreeGenerationProcess(waterVsLandHeight, chunkIndex, x, y, z, blockP);
                    assert(waterVsLandHeight < CHUNK_Y);
                    assert(waterVsLandHeight > 0);
                    heightMap[x][z] = waterVsLandHeight;
                    newHeight = Max(waterVsLandHeight, newHeight);
                }
            }
        }
        g_chunks->height[chunkIndex] = Max(uint16(newHeight), g_chunks->height[chunkIndex]);
    }

    

    //Add water
    for (int32 z = 0; z < CHUNK_Z; z++)
    {
        for (int32 y = 0; y < HEIGHT_MAX_WATER; y++)
        {
            for (int32 x = 0; x < CHUNK_X; x++)
            {
                if (g_chunks->blocks[chunkIndex].e[x][y][z] == BlockType::Empty)
                {
                    g_chunks->blocks[chunkIndex].e[x][y][z] = BlockType::Water;
                    g_chunks->height[chunkIndex] = Max(uint16(y + 1), g_chunks->height[chunkIndex]);
                }
            }
        }
    }

    //Update chunk height
    for (int32 z = 0; z < CHUNK_Z; z++)
    {
        for (int32 x = 0; x < CHUNK_X; x++)
        {
            g_chunks->height[chunkIndex] = Max(heightMap[x][z], g_chunks->height[chunkIndex]);
        }
    }


#elif VORONOI == 12

    WorldPos chunkGamePos = ToWorld(g_chunks->p[i]);
    NoiseParams seaFloorParams = {
        .numOfOctaves = 8,
        .freq = 0.17f,
        //.freq = 0.3f,
        .weight = 1.0f,
        //.gainFactor = 0.8f,
        .gainFactor = 1.0f,
    };
    int32 noiseOffset = 40;
    const float perlinScale = 0.01f;

    //Step 1: 
    //Generate base layer
    {
        for (uint32 x = 0; x < CHUNK_X; x++)
        {
            for (uint32 y = 0; y < HEIGHT_UNBREAKABLE; y++)
            {
                for (uint32 z = 0; z < CHUNK_Z; z++)
                {
                    blocks[i].e[x][y][z] = BlockType::Bedrock;
                }
            }
        }
        g_chunks->height[i] = 1;
    }

    //Step 2:
    //Generate heightmap of basic terrain
    uint32 heightMap[CHUNK_X][CHUNK_Z] = {};
    {
        uint32 newHeight = 0;
        uint32 waterVsLandHeight = 0;
        {
            for (uint32 x = 0; x < CHUNK_X; x++)
            {
                for (uint32 z = 0; z < CHUNK_Z; z++)
                {
                    //Fill with stone from HEIGHT_MIN_WATER to the determined height - 3
                    waterVsLandHeight = Clamp<uint32>(uint32(noiseOffset + CHUNK_Y * Perlin2D({ (chunkGamePos.p.x + x) * perlinScale, (chunkGamePos.p.z + z) * perlinScale }, seaFloorParams)), HEIGHT_MIN_WATER, HEIGHT_MAX_WATER - 2);
                    uint32 y = HEIGHT_UNBREAKABLE;
                    for (y; y < waterVsLandHeight - 3; y++)
                    {
                        blocks[i].e[x][y][z] = BlockType::Stone;
                    }
                    //Fill the last 3 high blocks with either stone or sand depending on if the location is max height or not
                    for (y; y < waterVsLandHeight; y++)
                    {
                        if (waterVsLandHeight < HEIGHT_MAX_WATER - 2)
                            blocks[i].e[x][y][z] = BlockType::Sand;
                        else
                            blocks[i].e[x][y][z] = BlockType::Stone;
                    }
                    assert(waterVsLandHeight < CHUNK_Y);
                    assert(waterVsLandHeight > 0);
                    heightMap[x][z] = waterVsLandHeight;
                    newHeight = Max(waterVsLandHeight, newHeight);
                }
            }
        }
        g_chunks->height[i] = newHeight;
    }

    //Step 3:
    //Update chunk type based on terrain height
    {
        GamePos checkLocations[] = {
            {-2,            0,          -2 },
            { CHUNK_X + 2,  0,          -2 },
            {-2,            0, CHUNK_Z + 2 },
            { CHUNK_X + 2,  0, CHUNK_Z + 2 },
        };
        uint32 landTileCount = 0;
        for (int32 c = 0; c < arrsize(checkLocations); c++)
        {
            Vec2 lookupLoc = { (chunkGamePos.p.x + checkLocations[c].p.x) * perlinScale, (chunkGamePos.p.z + checkLocations[c].p.z) * perlinScale };
            uint32 height = Clamp<uint32>(uint32(noiseOffset + CHUNK_Y * Perlin2D(lookupLoc, seaFloorParams)), HEIGHT_MIN_WATER, HEIGHT_MAX_WATER - 2);
            if (height == HEIGHT_MAX_WATER - 2)
                landTileCount++;
        }
        if (landTileCount == 4)
            g_chunks->chunkType[i] = ChunkType::Inland;
        else if (landTileCount == 0)
            g_chunks->chunkType[i] = ChunkType::Ocean;
        else
            g_chunks->chunkType[i] = ChunkType::Coastal;
    }

    //Step 4:
    //Determine Terrain Type
    {
        const float threshold = 0.1f;
        Vec2 chunkPForNoise = { chunkGamePos.p.x, chunkGamePos.p.z };
        float vorResult = VoronoiNoise(chunkPForNoise / (16.0f * 16.0f), 1.0f, 1.0f);
        switch (g_chunks->chunkType[i])
        {
        case ChunkType::Inland:
            if (vorResult > 0.5 + threshold)
                g_chunks->terrainType[i] = TerrainType::Mountains;
            if (vorResult < 0.5 - threshold)
                g_chunks->terrainType[i] = TerrainType::Plains;
            else
                g_chunks->terrainType[i] = TerrainType::Hills;
            break;
        case ChunkType::Coastal:
            g_chunks->terrainType[i] = TerrainType::Plains;
            break;
        }
    }

    //Step 5:
    //Generate terrain on Inland types
    {
        NoiseParams np;
        switch (g_chunks->terrainType[i])
        {
        case TerrainType::Plains:
        {
            np = {
                .numOfOctaves = 1,
                .freq = 1.0f,
                .weight = 1.0f,
                .gainFactor = 0.5f,
            };
            break;
        }
        case TerrainType::Hills:
        {
            np = {
                .numOfOctaves = 4,
                .freq = 0.75f,
                .weight = 1.0f,
                .gainFactor = 1.0f,
            };
            break;
        }
        case TerrainType::Mountains:
        {
            np = {
                .numOfOctaves = 8,
                .freq = 0.4f,
                .weight = 1.5f,
                .gainFactor = 0.9f,
            };
            break;
        }
        default:
        {
            assert(false);
            break;
        }
        }

        uint32 newHeight = 0;
        switch (g_chunks->chunkType[i])
        {
        case ChunkType::Inland:
            for (uint32 x = 0; x < CHUNK_X; x++)
            {
                for (uint32 z = 0; z < CHUNK_Z; z++)
                {
                    Vec2 blockP = { chunkGamePos.p.x + x, chunkGamePos.p.z + z };
                    assert(heightMap[x][z] < CHUNK_Y);
                        assert(heightMap[x][z] > 0);
                        int32 yTotal = Clamp<int32>(static_cast<int32>(Perlin2D(blockP * perlinScale, np) * (CHUNK_Y)), 2, CHUNK_Y - heightMap[x][z] - 1);
                    assert(yTotal < CHUNK_Y);
                    int32 compareValue = yTotal + (int32)heightMap[x][z];
                    for (int32 y = (int32)heightMap[x][z]; y < compareValue - 3; y++)
                    {
                        g_chunks->blocks[i].e[x][y][z] = BlockType::Stone;
                        heightMap[x][z]++;
                    }
                    assert(heightMap[x][z] < CHUNK_Y);
                    compareValue += 2;
                    for (int32 y = (int32)heightMap[x][z]; y < compareValue - 1; y++)
                    {
                        g_chunks->blocks[i].e[x][y][z] = BlockType::Dirt;
                        heightMap[x][z]++;
                    }
                    assert(heightMap[x][z] < CHUNK_Y);
                    compareValue += 1;
                    for (int32 y = (int32)heightMap[x][z]; y < compareValue; y++)
                    {
                        g_chunks->blocks[i].e[x][y][z] = BlockType::Grass;
                        heightMap[x][z]++;
                    }
                    assert(heightMap[x][z] < CHUNK_Y);
                    newHeight = Max(newHeight, heightMap[x][z]);
                }
            }
            g_chunks->height[i] = newHeight;
            break;
        case ChunkType::Coastal:
            for (uint32 x = 0; x < CHUNK_X; x++)
            {
                for (uint32 z = 0; z < CHUNK_Z; z++)
                {
                }
            }
            break;
        //default:
        }
    }

    //Step 6: Set Biomes
    {
        ChunkPos chunkP = g_chunks->p[i];
        uint32 areaSize = 10; //size of the area for sampling positions

        Vec3Int localPoint = chunkP.p / areaSize; //floor
        Vec2 allPoints[9];
        //Vec2 neighborPoints[8] = {};
        //Vec2 centerPoint = {};
        {
            float offsetSize = float(Max<uint32>((areaSize / 2 - 1), 1));
            int32 incrimentor = 0;
            for (int32 z = localPoint.z - 1; z <= localPoint.z + 1; z++)
            {
                for (int32 x = localPoint.x - 1; x <= localPoint.x + 1; x++)
                {
                    Vec2 p = {};
                    float xOffsetRatio = ((XXSeedHash(s_worldSeed, x) & 0xFFFF) / float(0xFFFF));
                    float yOffsetRatio = ((XXSeedHash(s_worldSeed, z) & 0xFFFF) / float(0xFFFF));
                    p.x = float(x) + xOffsetRatio * offsetSize;
                    p.y = float(z) + yOffsetRatio * offsetSize;
                    allPoints[incrimentor++] = p;
                }
            }
        }
        //{
        //    Vec2 p = {};
        //    float xOffsetRatio = ((XXSeedHash(s_worldSeed, localPoint.x) & 0xFFFF) / float(0xFFFF));
        //    float yOffsetRatio = ((XXSeedHash(s_worldSeed, localPoint.z) & 0xFFFF) / float(0xFFFF));
        //    p.x = float(localPoint.x) + xOffsetRatio;
        //    p.y = float(localPoint.z) + yOffsetRatio;
        //    centerPoint = p;
        //}
    }

#endif

#if 0

    for (int32 x = 0; x < CHUNK_X; x++)
    {
        for (int32 z = 0; z < CHUNK_Z; z++)
        {
            GamePos blockP = { x, 0, z };
            GamePos chunkBlockP = Convert_ChunkIndexToGame(chunkIndex);

            Vec2 blockRatio = { static_cast<float>(chunkBlockP.p.x + blockP.p.x), static_cast<float>(chunkBlockP.p.z + blockP.p.z) };

            const int32 blockRatioProduct = 100;noiseOffset
            blockRatio /= blockRatioProduct;
            int32 yTotal = 0;
            //BlockType topBlockType;// = BlockType::Grass;
#endif



#if 0

            float tempVal = {};
            float moistVal = {};
            BiomeTemp temp;
            BiomeMoist moist;
            Vec2 chunkPForNoise = { float(g_chunks->p[i].p.x), float(g_chunks->p[i].p.z) };
            BiomeType biome;
            float boundaryThreshold = 0.05f;
            Vec2 chunkAndBlock = chunkPForNoise + Vec2({ float(x) / CHUNK_X, float(z) / CHUNK_Z });
            tempVal  = VoronoiNoise(chunkAndBlock / 18, 1.0f, 0.0f);
            moistVal = VoronoiNoise(chunkAndBlock / 6,  1.0f, 0.0f);
            float tempIndex  = (Clamp<float>(tempVal  * +BiomeTemp::Count, 0.0f, 5.9f));
            float moistIndex = (Clamp<float>(moistVal * +BiomeMoist::Count, 0.0f, 5.9f));
            float secondaryBiomeTemp = tempIndex;
            float secondaryBiomeMoist = moistIndex;
            //BiomeType secondaryBiome;
            //lerp the difference of the threshold value and the index value?  something like that
            if (floorf(tempIndex + boundaryThreshold) != floorf(tempIndex))
                secondaryBiomeTemp = tempIndex + 1.0f;
            else if (floorf(tempIndex - boundaryThreshold) != floorf(tempIndex))
                secondaryBiomeTemp = tempIndex - 1.0f;
            if (floorf(moistIndex + boundaryThreshold) != floorf(moistIndex))
                secondaryBiomeMoist = moistIndex + 1.0f;
            else if (floorf(moistIndex - boundaryThreshold) != floorf(moistIndex))
                secondaryBiomeMoist = moistIndex - 1.0f;
            temp  = BiomeTemp(tempIndex);
            moist = BiomeMoist(moistIndex);
            biome = GetBiomeType(temp, moist);
            NoiseParams np;
            switch (biome)
            {
            case BiomeType::Woodland:
            {
                np = {
                    .numOfOctaves = 4,
                    .freq = 0.75f,
                    .weight = 1.0f,
                    .gainFactor = 1.0f,
                };
                topBlockType = BlockType::Wood;
                break;
            }
            case BiomeType::TropicalRainforest:
            {
                np = {
                    .numOfOctaves = 4,
                    .freq = 0.75f,
                    .weight = 1.0f,
                    .gainFactor = 1.0f,
                };
                topBlockType = BlockType::DiamondBlock;
                break;
            }
            case BiomeType::Grassland:
            {
                np = {
                    .numOfOctaves = 4,
                    .freq = 0.75f,
                    .weight = 1.0f,
                    .gainFactor = 1.0f,
                };noiseOffset
                topBlockType = BlockType::Grass;
                break;
            }
            case BiomeType::SeasonalForest:
            {
                np = {
                    .numOfOctaves = 8,
                    .freq = 0.4f,
                    .weight = 1.5f,
                    .gainFactor = 0.9f,
                };
                topBlockType = BlockType::TNT;
                break;
            }
            case BiomeType::TemperateRainforest:
            {
                np = {
                    .numOfOctaves = 8,
                    .freq = 0.4f,
                    .weight = 1.5f,
                    .gainFactor = 0.9f,
                };
                topBlockType = BlockType::MossyCobblestone;
                break;
            }
            case BiomeType::BorealForest:
            //case BiomeType::Mountain:
            {
                np = {
                    .numOfOctaves = 8,
                    .freq = 0.4f,
                    .weight = 1.5f,
                    .gainFactor = 0.9f,
                };
                topBlockType = BlockType::Leaves;
                break;
            }
            case BiomeType::Savanna:
            {
                np = {
                    .numOfOctaves = 1,
                    .freq = 1.0f,
                    .weight = 1.0f,
                    .gainFactor = 0.5f,
                };
                topBlockType = BlockType::GoldBlock;
                break;
            }
            case BiomeType::Desert:
            {
                np = {
                    .numOfOctaves = 1,
                    .freq = 1.0f,
                    .weight = 1.0f,
                    .gainFactor = 0.5f,
                };
                topBlockType = BlockType::Sand;
                break;
            }
            case BiomeType::Ice:
            {
                np = {
                    .numOfOctaves = 1,
                    .freq = 2.0f,
                    .weight = 1.0f,
                    .gainFactor = 0.5f,
                };
                topBlockType = BlockType::Ice;
                break;
            }
            case BiomeType::Tundra:
            {
                np = {
                    .numOfOctaves = 1,
                    .freq = 2.0f,
                    .weight = 1.0f,
                    .gainFactor = 0.5f,
                };
                topBlockType = BlockType::Snow;
                break;
            }
            default:
            {
                assert(false);//(+chunkType) > +ChunkType::None && (+chunkType) < +ChunkType::Count);
                break;
            }
            }
            yTotal = Clamp<uint32>(static_cast<int32>(Perlin2D(blockRatio, np) * CHUNK_Y), 10, CHUNK_Y - 1);
#endif
#if VORONOI == 9

#if 1
            float tempVal = {};
            float moistVal = {};
            BiomeTemp temp;
            BiomeMoist moist;
            Vec2 chunkPForNoise = { float(g_chunks->p[i].p.x), float(g_chunks->p[i].p.z) };
            BiomeType biome;
            float boundaryThreshold = 0.05f;
            Vec2 chunkAndBlock = chunkPForNoise + Vec2({ float(x) / CHUNK_X, float(z) / CHUNK_Z });
            tempVal  = VoronoiNoise(chunkAndBlock / 18, 1.0f, 0.0f);
            moistVal = VoronoiNoise(chunkAndBlock / 6,  1.0f, 0.0f);
            float tempIndex  = (Clamp<float>(tempVal  * +BiomeTemp::Count, 0.0f, 5.9f));
            float moistIndex = (Clamp<float>(moistVal * +BiomeMoist::Count, 0.0f, 5.9f));
            float secondaryBiomeTemp = tempIndex;
            float secondaryBiomeMoist = moistIndex;
            //BiomeType secondaryBiome;
            //lerp the difference of the threshold value and the index value?  something like that
            if (floorf(tempIndex + boundaryThreshold) != floorf(tempIndex))
                secondaryBiomeTemp = tempIndex + 1.0f;
            else if (floorf(tempIndex - boundaryThreshold) != floorf(tempIndex))
                secondaryBiomeTemp = tempIndex - 1.0f;

            if (floorf(moistIndex + boundaryThreshold) != floorf(moistIndex))
                secondaryBiomeMoist = moistIndex + 1.0f;
            else if (floorf(moistIndex - boundaryThreshold) != floorf(moistIndex))
                secondaryBiomeMoist = moistIndex - 1.0f;
            temp  = BiomeTemp(tempIndex);
            moist = BiomeMoist(moistIndex);
            biome = GetBiomeType(temp, moist);

#endif

            NoiseParams np;
            switch (biome)
            {
            case BiomeType::Woodland:
            {
                np = {
                    .numOfOctaves = 4,
                    .freq = 0.75f,
                    .weight = 1.0f,
                    .gainFactor = 1.0f,
                };
                topBlockType = BlockType::Wood;
                break;
            }
            case BiomeType::TropicalRainforest:
            {
                np = {
                    .numOfOctaves = 4,
                    .freq = 0.75f,
                    .weight = 1.0f,
                    .gainFactor = 1.0f,
                };
                topBlockType = BlockType::DiamondBlock;
                break;
            }
            case BiomeType::Grassland:
            {
                np = {
                    .numOfOctaves = 4,
                    .freq = 0.75f,
                    .weight = 1.0f,
                    .gainFactor = 1.0f,
                };
                topBlockType = BlockType::Grass;
                break;
            }
            case BiomeType::SeasonalForest:
            {
                np = {
                    .numOfOctaves = 8,
                    .freq = 0.4f,
                    .weight = 1.5f,
                    .gainFactor = 0.9f,
                };
                topBlockType = BlockType::TNT;
                break;
            }
            case BiomeType::TemperateRainforest:
            {
                np = {
                    .numOfOctaves = 8,
                    .freq = 0.4f,
                    .weight = 1.5f,
                    .gainFactor = 0.9f,
                };
                topBlockType = BlockType::MossyCobblestone;
                break;
            }
            case BiomeType::BorealForest:
            //case BiomeType::Mountain:
            {
                np = {
                    .numOfOctaves = 8,
                    .freq = 0.4f,
                    .weight = 1.5f,
                    .gainFactor = 0.9f,
                };
                topBlockType = BlockType::Leaves;
                break;
            }
            case BiomeType::Savanna:
            {
                np = {
                    .numOfOctaves = 1,
                    .freq = 1.0f,
                    .weight = 1.0f,
                    .gainFactor = 0.5f,
                };
                topBlockType = BlockType::GoldBlock;
                break;
            }
            case BiomeType::Desert:
            {
                np = {
                    .numOfOctaves = 1,
                    .freq = 1.0f,
                    .weight = 1.0f,
                    .gainFactor = 0.5f,
                };
                topBlockType = BlockType::Sand;
                break;
            }
            case BiomeType::Ice:
            {
                np = {
                    .numOfOctaves = 1,
                    .freq = 2.0f,
                    .weight = 1.0f,
                    .gainFactor = 0.5f,
                };
                topBlockType = BlockType::Ice;
                break;
            }
            case BiomeType::Tundra:
            {
                np = {
                    .numOfOctaves = 1,
                    .freq = 2.0f,
                    .weight = 1.0f,
                    .gainFactor = 0.5f,
                };
                topBlockType = BlockType::Snow;
                break;
            }
#endif
#if 0
    //Layer 1 + 2
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 7][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;
    //
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 4] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 7][CHUNK_Z - 4] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 5] = BlockType::Grass;
    //
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 7][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 6][CHUNK_Z - 7] = BlockType::Grass;
    //
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 6][CHUNK_Z - 7] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 7][CHUNK_Z - 6] = BlockType::Grass;
    //
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 7][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 6][CHUNK_Z - 7] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 6][CHUNK_Z - 6] = BlockType::Grass;
    //
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 6][CHUNK_Z - 4] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 7][CHUNK_Z - 4] = BlockType::Grass;
    //
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 6][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 7][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;
    //
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 7][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;



    //Layer 3
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 4][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 4][CHUNK_Z - 1] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 4][CHUNK_Z - 4] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 4][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 4][CHUNK_Z - 7] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 4][CHUNK_Z - 7] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 4][CHUNK_Z - 7] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 4][CHUNK_Z - 6] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 4][CHUNK_Z - 4] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 4][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 4][CHUNK_Z - 1] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 4][CHUNK_Z - 1] = BlockType::Grass;



    //Layer 4 + 5
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 2][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 1][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 2][CHUNK_Z - 1] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 2][CHUNK_Z - 4] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 1][CHUNK_Z - 4] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 2][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 1][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 2][CHUNK_Z - 7] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 2][CHUNK_Z - 7] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 1][CHUNK_Z - 6] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 1][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 2][CHUNK_Z - 7] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 2][CHUNK_Z - 6] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 2][CHUNK_Z - 4] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 1][CHUNK_Z - 4] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 2][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 1][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 2][CHUNK_Z - 1] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 1][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 2][CHUNK_Z - 1] = BlockType::Grass;
#endif


}

Vec3Int GetBlockPosFromIndex(uint16 index)
{
    int32 blockY =  index / CHUNK_Y;
    int32 duplicateMath = (index - blockY * CHUNK_Y);
    int32 blockZ = (duplicateMath) / CHUNK_Z;
    int32 blockX =  duplicateMath - blockZ * CHUNK_Z;

    return { blockX, blockY, blockZ };
}

//returns false on failure and true on success/found
bool ChunkArray::GetChunk(ChunkIndex& result, GamePos blockP)
{
    assert(OnMainThread());
    ChunkIndex index;
    if (GetChunkFromPosition(index, ToChunk(blockP)))
    {
        result = index;
        return true;
    }
    else
        return false;
}

GamePos Convert_BlockToGame(ChunkIndex blockParentIndex, Vec3Int blockP)
{
    GamePos chunkLocation = ToGame(g_chunks->p[blockParentIndex]);
    return { chunkLocation.p.x + blockP.x, chunkLocation.p.y + blockP.y, chunkLocation.p.z + blockP.z };
}

Vec3Int Convert_GameToBlock(ChunkPos& result, GamePos inputP)
{
    result = ToChunk(inputP);
    GamePos chunkP = ToGame(result);
    return Abs(Vec3Int({ inputP.p.x - chunkP.p.x, inputP.p.y - chunkP.p.y, inputP.p.z - chunkP.p.z }));
}

//returns true if the block is within the array
bool RegionSampler::GetBlock(BlockType& result, Vec3Int blockRelP)
{
    if (blockRelP.y >= CHUNK_Y || blockRelP.y < 0)
    {
        result = BlockType::Empty;
        return false;
    }
    else
    {
        GamePos gameSpace_block = Convert_BlockToGame(center, blockRelP);
        ChunkPos newChunkPos = {};
        Vec3Int newRelBlockP = Convert_GameToBlock(newChunkPos, gameSpace_block);
        //if (g_chunks->p[center].p == newChunkPos.p)
        if (centerP.p == newChunkPos.p)
        {
            assert(newRelBlockP == blockRelP);
            result = g_chunks->blocks[center].e[newRelBlockP.x][newRelBlockP.y][newRelBlockP.z];
            return true;
        }
        for (int32 i = 0; i < arrsize(neighbors); i++)
        {
            //if (g_chunks->p[neighbors[i]].p == newChunkPos.p)

            if (neighborsP[i].p == newChunkPos.p)
            {
                result = g_chunks->blocks[neighbors[i]].e[newRelBlockP.x][newRelBlockP.y][newRelBlockP.z];
                return true;
            }
        }
        return false;
    }
}

bool RegionSampler::RegionGather(ChunkIndex i)
{
    center = i;

    int32 numIndices = 0;
    centerP = g_chunks->p[i];
    for (int32 z = -1; z <= 1; z++)
    {
        for (int32 x = -1; x <= 1; x++)
        {
            if (x == 0 && z == 0)
                continue;
            ChunkIndex chunkIndex = 0;
            ChunkPos newChunkP = { centerP.p.x + x, 0, centerP.p.z + z };
            int64 tesHash = PositionHash(newChunkP);
            if (g_chunks->GetChunkFromPosition(chunkIndex, newChunkP))
            {
                if (g_chunks->state[chunkIndex] >= ChunkArray::BlocksLoaded)
                {
                    neighborsP[numIndices] = g_chunks->p[chunkIndex];
                    neighbors[numIndices++] = chunkIndex;
                }
            }
        }
    }

    bool valid = numIndices == arrsize(neighbors);

    return valid;
}
void RegionSampler::DecrimentRefCount()
{
    g_chunks->refs[center]--;
    for (int32 i = 0; i < arrsize(neighbors); i++)
        g_chunks->refs[i]--;
}
void RegionSampler::IncrimentRefCount()
{
    g_chunks->refs[center]++;
    for (int32 i = 0; i < arrsize(neighbors); i++)
        g_chunks->refs[i]++;
}

//TODO: Add error reporting when this fails
bool BlockSampler::RegionGather(GamePos base)
{
    m_baseBlockP = base;
    bool result = true;

    ChunkPos blockChunkP = {};
    Vec3Int block_blockP = Convert_GameToBlock(blockChunkP, base);
    ChunkIndex blockChunkIndex;
    if (g_chunks->GetChunkFromPosition(blockChunkIndex, blockChunkP))
    {
        m_baseBlockType = g_chunks->blocks[blockChunkIndex].e[block_blockP.x][block_blockP.y][block_blockP.z];
        result = !(m_baseBlockType == BlockType::Empty);
    }
    else
        result = false;

    if (result)
    {
        for (uint8 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
        {
            blocks[faceIndex] = BlockType::Empty;
            Vec3Int faceNormal = { int32(faceNormals[faceIndex].x), int32(faceNormals[faceIndex].y), int32(faceNormals[faceIndex].z) };
            GamePos checkingBlock = GamePos(base.p + faceNormal);

            ChunkPos blockChunkP = {};
            Vec3Int block_blockP = Convert_GameToBlock(blockChunkP, checkingBlock);
            ChunkIndex blockChunkIndex;
            if (g_chunks->GetChunkFromPosition(blockChunkIndex, blockChunkP))
            {
                blocks[faceIndex] = g_chunks->blocks[blockChunkIndex].e[block_blockP.x][block_blockP.y][block_blockP.z];
            }
            else if (checkingBlock.p.y < 0 || checkingBlock.p.y >= CHUNK_Y)
            {
                blocks[faceIndex] = BlockType::Empty;
            }
            else
            {
                result = false;
                break;
            }
        }
    }
    return result;
}

//{
///*
//    1. Place temporary blocks in the chunk when it is edited and flag the chunk to rebuild vertices
//        - editing the chunk when the chunk is being rebuilt.  Need to know which temp blocks to remove
//    2. Remove the vertices at the target location and update the vertices at that area.
//        - issues: inserting vertices out of order would cause problems for future updates
//        - place new vertices at the back of the 
//    3. Job priority queue with chunk generation and create these as jobs?
//        - possibly even have other jobs early out?
//
//*/
//    
//}

enum class T_Vertices : uint32 {
    GetBlock,
    Faces,
    BlockCheck,
    Overall,
    Pushback,
    Count,
};
ENUMOPS(T_Vertices);
const char * T_verticesStrings[] = {
    "GetBlock",
    "Faces",
    "BlockCheck",
    "Overall",
    "Pushback",
};
static_assert(arrsize(T_verticesStrings) == +T_Vertices::Count);

struct Accumulator {
    uint64 begin;
    uint64 totalTime;
};

//float accTimers[+T_Vertices::Count] = {};
thread_local Accumulator accumulators[+T_Vertices::Count];
void AccBeginFunc(T_Vertices enumType)
{
    accumulators[+enumType].begin = GetCurrentTime();
}

void AccEndTimer(T_Vertices enumType)
{
    Accumulator& acc = accumulators[+enumType];
    acc.totalTime += GetCurrentTime() - acc.begin;
}

#define ACCTIMER(enumType) \
AccBeginFunc(enumType);\
Defer\
{\
    AccEndTimer(enumType);\
}


void PrintTimers()
{
    for (uint32 j = 0; j < +T_Vertices::Count; j++)
    {
        double totalValue = (accumulators[j].totalTime / (1000.0 * 1000.0));
        DebugPrint("T_Vertices %s: %f ms\n", T_verticesStrings[j], totalValue);
        accumulators[j].totalTime = 0;
    }
}

#define PRINTTIMER() \
PrintTimers()

uint16 CreateBlockIndex(Vec3Int pos)
{
    return pos.z * CHUNK_Z + pos.y * CHUNK_Y + pos.x;
}


void ChunkArray::BuildChunkVertices(RegionSampler region)
{
    assert(g_chunks->state[region.center] == ChunkArray::VertexLoading);
    ChunkIndex i = region.center;
    faceVertices[i].clear();
    faceVertices[i].reserve(10000);
    uploadedIndexCount[i] = 0;
    GamePos realP = Convert_ChunkIndexToGame(i);
    float timerTotal[+T_Vertices::Count] = {};
    uint16 heightOfChunk = height[i];
    {
        //PROFILE_SCOPE("THREAD: Vertex Creation");
        for (int32 x = 0; x < CHUNK_X; x++)
        {
            for (int32 y = 0; y < heightOfChunk; y++)
            {
                for (int32 z = 0; z < CHUNK_Z; z++)
                {
                    BlockType currentBlockType = blocks[i].e[x][y][z];
                    if (currentBlockType == BlockType::Empty)
                        continue;
                    //ACCTIMER(T_Vertices::Faces);
                    for (uint32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
                    {
                        Vec3Int vf = Vec3ToVec3Int(faceNormals[faceIndex]);
                        int32 xReal = x + vf.x;
                        int32 yReal = y + vf.y;
                        int32 zReal = z + vf.z;

                        BlockType type;
                        bool getBlockResult = false;
                        {
                            //ACCTIMER(T_Vertices::GetBlock);
                            getBlockResult = (region.GetBlock(type, { xReal, yReal, zReal })) || (yReal == CHUNK_Y);
                        }
                        if (getBlockResult && type == BlockType::Empty || (currentBlockType != BlockType::Water && type == BlockType::Water))
                        {
                            VertexFace f = {};
                            Vec3 offset = { static_cast<float>(x + realP.p.x), static_cast<float>(y + realP.p.y), static_cast<float>(z + realP.p.z) };

                            f.a.blockIndex =
                            f.b.blockIndex =
                            f.c.blockIndex =
                            f.d.blockIndex = CreateBlockIndex({ x, y, z });

                            f.a.spriteIndex =
                            f.b.spriteIndex =
                            f.c.spriteIndex =
                            f.d.spriteIndex = blockSprites[+currentBlockType].faceSprites[faceIndex];

                            f.a.nAndConnectedVertices = 
                            f.b.nAndConnectedVertices = 
                            f.c.nAndConnectedVertices =
                            f.d.nAndConnectedVertices = 0xF0 & (faceIndex << 4);

                            //ACCTIMER(T_Vertices::Pushback);
                            faceVertices[i].push_back(f.a);
                            faceVertices[i].push_back(f.b);
                            faceVertices[i].push_back(f.c);
                            faceVertices[i].push_back(f.d);
                            uploadedIndexCount[i] += 6;
                        }

                    }
                }
            }
        }
    }
    //PRINTTIMER();

    uint32 vertIndex = 0;
    //-X and -Z
    {
        //PROFILE_SCOPE("THREAD: AO Creation");
        for (Vertex_Chunk& vert : faceVertices[i])
        {
            uint8 normal = (vert.nAndConnectedVertices & 0xF0) >> 4;
            Vec3Int blockN = Vec3ToVec3Int(faceNormals[normal]);
                Vec3Int blockP = GetBlockPosFromIndex(vert.blockIndex);

            uint8 faceIndex = normal;
            Vec3Int a = *(&vertexBlocksToCheck[faceIndex].e0 + (vertIndex + 0));
            Vec3Int b = *(&vertexBlocksToCheck[faceIndex].e0 + (vertIndex + 1));
            Vec3Int c = a + b;

            BlockType aType = BlockType::Empty;
            BlockType bType = BlockType::Empty;
            BlockType cType = BlockType::Empty;
            region.GetBlock(aType, blockP + blockN + a);
            region.GetBlock(bType, blockP + blockN + b);
            region.GetBlock(cType, blockP + blockN + c);

            if (aType != BlockType::Empty)
                vert.nAndConnectedVertices += 1;
            if (bType != BlockType::Empty)
                vert.nAndConnectedVertices += 1;
            if (cType != BlockType::Empty)
                vert.nAndConnectedVertices += 1;

            vertIndex += 2;
            vertIndex = vertIndex % 8;
        }
        g_chunks->state[region.center] = ChunkArray::VertexLoaded;
    }
}

void ChunkArray::UploadChunk(ChunkIndex i)
{
    vertexBuffer[i].Upload(faceVertices[i].data(), faceVertices[i].size());
    std::vector<Vertex_Chunk> faces;
    faceVertices[i].swap(faces);
    g_chunks->state[i] = ChunkArray::Uploaded;
}

void PreChunkRender(const Mat4& perspective, Camera* camera)
{
    assert(g_renderer.chunkIB);
    if (g_renderer.chunkIB)
        g_renderer.chunkIB->Bind();
    else
        return;

    g_renderer.programs[+Shader::Chunk]->UseShader();
    g_renderer.spriteTextArray->Bind();

    ShaderProgram* sp = g_renderer.programs[+Shader::Chunk];
    sp->UpdateUniformMat4("u_perspective", 1, false, perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, camera->m_view.e);

#if DIRECTIONALLIGHT == 1
    sp->UpdateUniformVec3("u_directionalLight_d",  1,  g_renderer.sunLight.d.e);
    sp->UpdateUniformVec3("u_lightColor",  1,  g_renderer.sunLight.c.e);
    sp->UpdateUniformVec3("u_directionalLightMoon_d", 1, g_renderer.moonLight.d.e);
    sp->UpdateUniformVec3("u_moonColor", 1, g_renderer.moonLight.c.e);
#else
    sp->UpdateUniformVec3("u_lightColor",  1,  g_light.c.e);
    sp->UpdateUniformVec3("u_lightP",      1,  g_light.p.e);
#endif
    //sp->UpdateUniformVec3("u_cameraP",     1,  camera->RealWorldPos().p.e);
    sp->UpdateUniformVec3("u_cameraP",     1,  camera->GetWorldPosition().p.e);

    sp->UpdateUniformUint8("u_CHUNK_X", CHUNK_X);
    sp->UpdateUniformUint8("u_CHUNK_Y", CHUNK_Y);
    sp->UpdateUniformUint8("u_CHUNK_Z", CHUNK_Z);

    Material material;
    material.ambient = { 0.2f, 0.2f, 0.2f };
    material.diffuse = { 1.0f, 1.0f, 1.0f };
    material.specular = { 0.4f, 0.4f,  0.4f };
    material.shininess = 32;//0.78125f;
    sp->UpdateUniformVec3( "material.ambient",  1,  material.ambient.e);
    sp->UpdateUniformVec3( "material.diffuse",  1,  material.diffuse.e);
    sp->UpdateUniformVec3( "material.specular", 1,  material.specular.e);
    sp->UpdateUniformFloat("material.shininess",    material.shininess);

}

void ChunkArray::RenderChunk(ChunkIndex i)
{
    vertexBuffer[i].Bind();

    glVertexAttribIPointer(0, 1, GL_UNSIGNED_SHORT, sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, blockIndex));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, spriteIndex));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, nAndConnectedVertices));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    //glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, connectedVertices));
    //glEnableVertexArrayAttrib(g_renderer.vao, 3);

    ShaderProgram* sp = g_renderer.programs[+Shader::Chunk];
    sp->UpdateUniformVec3("u_chunkP",      1,  ToWorld(Convert_ChunkIndexToGame(i)).p.e);

    glDrawElements(GL_TRIANGLES, (GLsizei)uploadedIndexCount[i], GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += uploadedIndexCount[i] / 3;
}

void SetBlocks::DoThing()
{
    //PROFILE_SCOPE("THREAD: SetBlocks()");
    g_chunks->refs[chunk]++;
    g_chunks->SetBlocks(chunk);
    g_chunks->state[chunk] = ChunkArray::BlocksLoaded;
    g_chunks->refs[chunk]--;
}

void CreateVertices::DoThing()
{
    //PROFILE_SCOPE("THREAD: CreateVertices()");
    region.IncrimentRefCount();
    g_chunks->BuildChunkVertices(region);
    region.DecrimentRefCount();
}

Rect GetUVsFromIndex(uint8 index)
{
    int32 spritesPerSide = 16;
    const Vec2Int& size = g_renderer.textures[Texture::Minecraft]->m_size;

    //auto blockIndex = blockSprites[+block].faceSprites[+Face::Top];
    //if (block == BlockType::Empty)
    //    blockIndex = 31;

    int32 x = index % spritesPerSide;
    int32 y = index / spritesPerSide;
    Vec2Int pixelsPerSprite = size / spritesPerSide;
    RectInt UVs = {
        .botLeft  = { x * pixelsPerSprite.x, (spritesPerSide - y) * pixelsPerSprite.y },
        .topRight = { UVs.botLeft.x + pixelsPerSprite.x, UVs.botLeft.y - pixelsPerSprite.y },
    };
    Rect result = {
        .botLeft  = { UVs.botLeft.x  / float(size.x), UVs.botLeft.y  / float(size.y) },
        .topRight = { UVs.topRight.x / float(size.x), UVs.topRight.y / float(size.y) },
    };
    return result;
}

static const Vec3 cubeVertices[] = {
    // +x
    gb_vec3(1.0f, 1.0f, 1.0f),
    gb_vec3(1.0f, 0.0f, 1.0f),
    gb_vec3(1.0f, 1.0f, 0.0f),
    gb_vec3(1.0f, 0.0f, 0.0f),
    // -x
    gb_vec3(0.0f, 1.0f, 0.0f),
    gb_vec3(0.0f, 0.0f, 0.0f),
    gb_vec3(0.0f, 1.0f, 1.0f),
    gb_vec3(0.0f, 0.0f, 1.0f),
    // +y
    gb_vec3(1.0f, 1.0f, 1.0f),
    gb_vec3(1.0f, 1.0f, 0.0f),
    gb_vec3(0.0f, 1.0f, 1.0f),
    gb_vec3(0.0f, 1.0f, 0.0f),
    // -y
    gb_vec3(0.0f, 0.0f, 1.0f),
    gb_vec3(0.0f, 0.0f, 0.0f),
    gb_vec3(1.0f, 0.0f, 1.0f),
    gb_vec3(1.0f, 0.0f, 0.0f),
    // z
    gb_vec3(0.0f, 1.0f, 1.0f),
    gb_vec3(0.0f, 0.0f, 1.0f),
    gb_vec3(1.0f, 1.0f, 1.0f),
    gb_vec3(1.0f, 0.0f, 1.0f),
    // -z
    gb_vec3(1.0f, 1.0f, 0.0f),
    gb_vec3(1.0f, 0.0f, 0.0f),
    gb_vec3(0.0f, 1.0f, 0.0f),
    gb_vec3(0.0f, 0.0f, 0.0f),
};

static const Vec2 faceUV[4] = {
    Vec2{ 0, 1 },
    Vec2{ 0, 0 },
    Vec2{ 1, 1 },
    Vec2{ 1, 0 }
};

void DrawBlock(const Mat4& mat, Color color, float scale, Camera* camera, Texture::T textureType, BlockType blockType)
{
    DrawBlock(mat, color, { scale, scale, scale }, camera, textureType, blockType);
}

void DrawBlock(const Mat4& mat, Color color, Vec3 scale, Camera* camera, Texture::T textureType, BlockType blockType)
{
    std::unique_ptr<VertexBuffer> vb = std::make_unique<VertexBuffer>();

    Vertex vertices[arrsize(cubeVertices)] = {};

    for (int32 i = 0; i < arrsize(cubeVertices); i++)
    {
        vertices[i].p = cubeVertices[i] - 0.5f;
        auto spriteIndex = blockSprites[+blockType].faceSprites[i / 4];
        //TODO: Refactor this garbago:
        Rect UVSquare = GetUVsFromIndex(spriteIndex);
        vertices[i].uv.x = Lerp(UVSquare.botLeft.x, UVSquare.topRight.x, faceUV[i % 4].x);
        vertices[i].uv.y = Lerp(UVSquare.topRight.y, UVSquare.botLeft.y, faceUV[i % 4].y);
    }

    vb->Upload(vertices, arrsize(vertices));
    g_renderer.chunkIB->Bind();

    ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
    sp->UseShader();
    g_renderer.textures[textureType]->Bind();
    sp->UpdateUniformMat4("u_perspective", 1, false, camera->m_perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, camera->m_view.e);
    sp->UpdateUniformMat4("u_model",       1, false, mat.e);
    sp->UpdateUniformVec3("u_scale",       1,        scale.e);
    sp->UpdateUniformVec4("u_color",       1,        color.e);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glDisableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += 36 / 3;
}

void DrawCube(WorldPos p, Color color, float scale, Camera* camera, Texture::T textureType, BlockType blockType)
{
    DrawCube(p, color, { scale, scale, scale }, camera, textureType, blockType);
}
void DrawCube(WorldPos p, Color color, Vec3  scale, Camera* camera, Texture::T textureType, BlockType blockType)
{
    if (g_renderer.cubeVertexBuffer == nullptr)
    {
        g_renderer.cubeVertexBuffer = new VertexBuffer();

        Vertex vertices[arrsize(cubeVertices)] = {};
        Vec2 uvOffset = { 1.0f, 1.0f };

        for (int32 i = 0; i < arrsize(cubeVertices); i++)
        {
            vertices[i].p  = cubeVertices[i] - 0.5f;
            vertices[i].uv = faceUV[i % 4];
        }

        g_renderer.cubeVertexBuffer->Upload(vertices, arrsize(vertices));
    }

    g_renderer.cubeVertexBuffer->Bind();
    g_renderer.chunkIB->Bind();

    Mat4 transform;
    gb_mat4_translate(&transform, { p.p.x, p.p.y, p.p.z });
    g_renderer.textures[textureType]->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
    sp->UseShader();
    sp->UpdateUniformMat4("u_perspective", 1, false, camera->m_perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, camera->m_view.e);
    sp->UpdateUniformMat4("u_model",       1, false, transform.e);
    sp->UpdateUniformVec3("u_scale",       1,        scale.e);
    sp->UpdateUniformVec4("u_color",       1,        color.e);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glDisableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += 36 / 3;
}

//UNTESTED AND DOES NOT WORK
void Draw2DSquare(Rect rect, Color color)
{
    std::unique_ptr<VertexBuffer> vertexBuffer = std::make_unique<VertexBuffer>();

    //WRONG COORDINATE SPACE -1 TO 1 NOT 0 TO SIZE OF SCREEN IN PIXELS
    Vertex vertices[4] = {};
    vertices[0].p = { rect.botLeft.x,  rect.topRight.y, 1.5f };
    vertices[1].p = { rect.botLeft.x,  rect.botLeft.y,  1.5f };
    vertices[2].p = { rect.topRight.x, rect.topRight.y, 1.5f };
    vertices[3].p = { rect.topRight.x, rect.botLeft.y,  1.5f };
    vertices[0].uv = { 0, 1 };
    vertices[1].uv = { 0, 0 };
    vertices[2].uv = { 1, 1 };
    vertices[3].uv = { 1, 0 };

    vertexBuffer->Upload(vertices, arrsize(vertices));

    vertexBuffer->Bind();
    uint32 indices[] = { 0, 1, 2, 1, 3, 2 };
    uint32 spriteIndices[+Face::Count] = { 0, 0, 0, 0, 0, 0 };

    std::unique_ptr<IndexBuffer> indexBuffer = std::make_unique<IndexBuffer>();
    indexBuffer->Upload(indices, arrsize(indices));
    indexBuffer->Bind();

    Mat4 perspective, view, transform, scale;
    gb_mat4_identity(&perspective);
    view, transform, scale = perspective;

    g_renderer.textures[Texture::T::Plain]->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
    sp->UseShader();
    sp->UpdateUniformMat4("u_perspective", 1, false, perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, view.e);
    sp->UpdateUniformMat4("u_model",       1, false, transform.e);
    sp->UpdateUniformVec3("u_scale",       1,        scale.e);
    sp->UpdateUniformVec4("u_color",       1,        color.e);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glDisableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);

    glDrawElements(GL_TRIANGLES, arrsize(indices), GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += arrsize(indices);
}
            //Draw2DSquare(horz);

bool RayVsChunk(const Ray& ray, ChunkIndex chunkIndex, GamePos& block, float& distance, Vec3& normal)
{
    distance = inf;
    int32 rayCheckCount = 0;

    AABB chunkBox;
    chunkBox.min = ToWorld(g_chunks->p[chunkIndex]).p;
    chunkBox.max = { chunkBox.min.x + CHUNK_X, chunkBox.min.y + CHUNK_Y, chunkBox.min.z + CHUNK_Z };
    {
        PROFILE_SCOPE_TAB("RayVsChunk()/RayVsAABB()");
        rayCheckCount++;
        if (!RayVsAABB(ray, chunkBox))
            return false;
    }

    PROFILE_SCOPE_TAB("RayVsChunk/BlockLoop");
    for (int32 z = 0; z < CHUNK_Z; z++)
    {
        GamePos blockPZ = Convert_BlockToGame(chunkIndex, { 0, 0, z });
        AABB boxZ;
        boxZ.min = ToWorld(blockPZ).p;
        boxZ.max = boxZ.min;
        boxZ.max.x += float(CHUNK_X);
        boxZ.max.y += float(CHUNK_Y);
        boxZ.max.z += 1.0f;

        rayCheckCount++;
        if (RayVsAABB(ray, boxZ))
        {
            for (int32 y = 0; y < CHUNK_Y; y++)
            {
                GamePos blockPY = Convert_BlockToGame(chunkIndex, { 0, y, z });
                AABB boxY;
                boxY.min = ToWorld(blockPY).p;
                boxY.max = boxY.min;
                boxY.max.x += float(CHUNK_X);
                boxY.max.y += 1.0f;
                boxY.max.z += 1.0f;

                rayCheckCount++;
                if (RayVsAABB(ray, boxY))
                {
                    for (int32 x = 0; x < CHUNK_X; x++)
                    {
                        if (g_chunks->blocks[chunkIndex].e[x][y][z] != BlockType::Empty)
                        {
                            GamePos blockP = Convert_BlockToGame(chunkIndex, { x, y, z });

                            AABB boxX;
                            boxX.min = ToWorld(blockP).p;
                            boxX.max = boxX.min + 1.0f;

                            float minDistanceToHit;
                            Vec3 intersectionPoint = {};
                            Vec3 normalFace;
                            rayCheckCount++;
                            if (RayVsAABB(ray, boxX, minDistanceToHit, intersectionPoint, normalFace))
                            {
                                if (minDistanceToHit < distance)
                                {
                                    block = blockP;
                                    distance = minDistanceToHit;
                                    normal = normalFace;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return distance != inf;
}

void ChunkUpdateBlocks(ChunkPos p, Vec3Int offset = {})
{
    ChunkIndex chunkIndex;
    p.p += offset;
    if (g_chunks->GetChunkFromPosition(chunkIndex, p))
    {
        g_chunks->state[chunkIndex] = ChunkArray::VertexLoading;
        RegionSampler regionUpdate;
        regionUpdate.RegionGather(chunkIndex);
        g_chunks->BuildChunkVertices(regionUpdate);
    }
}
void SetBlock(GamePos hitBlock, BlockType setBlockType)
{
    PROFILE_SCOPE_TAB("SetBlock");

    ChunkPos hitChunkPos;
    Vec3Int hitBlockRelP = Convert_GameToBlock(hitChunkPos, hitBlock);
    ChunkIndex hitChunkIndex;
    if (g_chunks->GetChunkFromPosition(hitChunkIndex, hitChunkPos) && (hitBlock.p.y >= 0) && (hitBlock.p.y < CHUNK_Y))
    {
        PROFILE_SCOPE_TAB("SetBlock Success");

        g_chunks->blocks[hitChunkIndex].e[hitBlockRelP.x][hitBlockRelP.y][hitBlockRelP.z] = setBlockType;
        g_chunks->height[hitChunkIndex] = Max((uint16)(hitBlockRelP.y + 1), g_chunks->height[hitChunkIndex]);
        g_chunks->flags[hitChunkIndex]  |= CHUNK_FLAG_MODIFIED;

        RegionSampler regionUpdate;
        regionUpdate.RegionGather(hitChunkIndex);
        g_chunks->state[hitChunkIndex] = ChunkArray::VertexLoading;
        g_chunks->BuildChunkVertices(regionUpdate);

        if (hitBlockRelP.x == CHUNK_X - 1)
            ChunkUpdateBlocks(hitChunkPos, { 1,  0,  0 });
        if (hitBlockRelP.x == 0)
            ChunkUpdateBlocks(hitChunkPos, { -1,  0,  0 });
        if (hitBlockRelP.z == CHUNK_Z - 1)
            ChunkUpdateBlocks(hitChunkPos, { 0,  0,  1 });
        if (hitBlockRelP.z == 0)
            ChunkUpdateBlocks(hitChunkPos, { 0,  0, -1 });
    }

}

bool ChunkArray::GetBlock(BlockType& blockType, const GamePos& blockP)
{
    ChunkIndex chunkIndex = 0;
    blockType = BlockType::Empty;
    ChunkPos chunkP = ToChunk(blockP);
    if (g_chunks->GetChunkFromPosition(chunkIndex, chunkP))
    {
        Vec3Int block_blockP = Convert_GameToBlock(chunkP, blockP);
        blockType = g_chunks->blocks[chunkIndex].e[block_blockP.x][block_blockP.y][block_blockP.z];
        return true;
    }
    return false;
}


struct ChunkSaveData {
    ChunkData blocks = {};
    ChunkPos  p = {};
};

struct SaveChunkJob : public Job {
    ChunkSaveData m_data;

    void DoThing() override;
};

bool ChunkArray::Init()
{
    bool success = true;
    std::string filename = g_gameData.m_folderPath;
    success &= CreateFolder(filename);
    filename = g_gameData.m_saveFolderPath;
    success &= CreateFolder(filename);
    filename += g_gameData.m_saveFilename;
    success &= CreateFolder(filename);
    filename += "\\Chunk_Data";
    success &= CreateFolder(filename);
    return success;
}

bool ChunkArray::SaveChunk(ChunkIndex i)
{
    assert(OnMainThread());
    if (g_chunks->flags[i] & CHUNK_FLAG_MODIFIED)
    {
        SaveChunkJob* job = new SaveChunkJob();
        job->m_data.p = g_chunks->p[i];
        memcpy(job->m_data.blocks.e, g_chunks->blocks[i].e, CHUNK_X * CHUNK_Y * CHUNK_Z * sizeof(BlockType));

        MultiThreading::GetInstance().SubmitJob(job);
        return true;
    }
    return false;
}

#pragma pack(push, 1)
struct ChunkDiskFileHeader {
    uint32 m_magic_header;
    uint32 m_magic_type;
    uint32 version;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ChunkDiskHeader {
    uint32 m_magic_header;
    uint32 m_magic_type;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ChunkDiskData {
    uint16 m_count;
    std::underlying_type<BlockType>::type m_type;
};
#pragma pack(pop)

void SaveChunkJob::DoThing()
{
    ChunkDiskFileHeader mainHeader = {};
    mainHeader.m_magic_header = SDL_FOURCC('C', 'H', 'N', 'K');
    mainHeader.m_magic_type   = SDL_FOURCC('H', 'E', 'A', 'D');
    mainHeader.version = 1;

    ChunkDiskHeader chunkHeader = {};
    chunkHeader.m_magic_header = SDL_FOURCC('C', 'H', 'N', 'K');
    chunkHeader.m_magic_type   = SDL_FOURCC('D', 'A', 'T', 'A');

    std::vector<ChunkDiskData> dataArray;
    dataArray.reserve(100);
    ChunkDiskData data;
    data.m_type  = +m_data.blocks.e[0][0][0];
    data.m_count = 0;
    for (int32 y = 0; y < CHUNK_Y; y++)
    {
        for (int32 x = 0; x < CHUNK_X; x++)
        {
            for (int32 z = 0; z < CHUNK_Z; z++)
            {
                if (data.m_type == +m_data.blocks.e[x][y][z])
                {
                    data.m_count++;
                }
                else
                {
                    dataArray.push_back(data);
                    data.m_type  = +m_data.blocks.e[x][y][z];
                    data.m_count = 1;
                }
            }
        }
    }
    dataArray.push_back(data);

    std::string filename = g_gameData.m_saveFolderPath + g_gameData.m_saveFilename + "\\Chunk_Data\\" + ToString("%i_%i.wad", m_data.p.p.x, m_data.p.p.z);

    File file(filename.c_str(), File::FileMode::Write, true);

    if (file.m_handleIsValid)
    {
        bool success = true;
        success &= file.Write(&mainHeader, sizeof(ChunkDiskFileHeader));
        success &= file.Write(&chunkHeader, sizeof(ChunkDiskHeader));
        success &= file.Write(dataArray.data(), dataArray.size() * sizeof(ChunkDiskData));
    }

}
