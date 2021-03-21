#include "Block.h"
#include "WinInterop.h"
#include "Noise.h"
#include "Computer.h"

#include <unordered_map>

ChunkArray* g_chunks;

//
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
//


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
    //int64 result = {};
    //result = static_cast<int64>(p.p.z) & 0x00000000FFFFFFFFLL;
    //result |= static_cast<int64>(p.p.x) << (8 * sizeof(int32));
    //return result;
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
    auto it = chunkPosTable.find(PositionHash(p[index]));
    assert(it != chunkPosTable.end());
    chunkPosTable.erase(it);

    active[index] = {};
    blocks[index] = {};
    p[index] = {};

    faceVertices[index].clear();
    std::vector<Vertex_Chunk> swapping;
    faceVertices[index].swap(swapping);
    state[index] = {};
    uploadedIndexCount[index] = {};
    flags[index] = {};
    refs[index] = {};

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
            return i;
        }
    }
    return uint32(-1);
}

struct VertexFace {
    Vertex_Chunk a,b,c,d;
};

Vec3 faceNormals[] = {

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

struct BlockSprites
{
    uint8 faceSprites[+Face::Count] = {
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
};

static BlockSprites faceSprites[+BlockType::Count];

void SetMultipleBlockSprites(BlockType bt, uint32 v)
{
    for (uint32 i = 0; i < +Face::Count; i++)
    {
        faceSprites[+bt].faceSprites[i] = v;
    }
}
void SetBlockSprites()
{
    SetMultipleBlockSprites(BlockType::Grass, 3);
    faceSprites[+BlockType::Grass].faceSprites[+Face::Top] = 0;
    faceSprites[+BlockType::Grass].faceSprites[+Face::Bot] = 2;

    SetMultipleBlockSprites(BlockType::Stone, 1);
    SetMultipleBlockSprites(BlockType::IronBlock, 22);
    SetMultipleBlockSprites(BlockType::GoldBlock, 23);
    SetMultipleBlockSprites(BlockType::DiamondBlock, 24);
    SetMultipleBlockSprites(BlockType::Dirt, 2);
    SetMultipleBlockSprites(BlockType::Sand, 18);
    SetMultipleBlockSprites(BlockType::Snow, 66);
    SetMultipleBlockSprites(BlockType::Wood, 20);
    faceSprites[+BlockType::Wood].faceSprites[+Face::Top] = 21;
    faceSprites[+BlockType::Wood].faceSprites[+Face::Bot] = 21;
    SetMultipleBlockSprites(BlockType::Ice, 67);
    SetMultipleBlockSprites(BlockType::Obsidian, 37);
    SetMultipleBlockSprites(BlockType::Leaves, 53);
    SetMultipleBlockSprites(BlockType::MossyCobblestone, 36);
    SetMultipleBlockSprites(BlockType::TNT, 8);
    faceSprites[+BlockType::TNT].faceSprites[+Face::Top] = 9;
    faceSprites[+BlockType::TNT].faceSprites[+Face::Bot] = 10;
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
//std::vector<BiomePoints> s_biomePoints;

//TODO:!!!
//SRAND IS NOT THREAD SAFE
void UpdateBiomePoints()
{
    ChunkPos cameraP = ToChunk(g_camera.p);
    float biLinearThreshold = 0.05f;

    for (int32 z = -g_camera.drawDistance + cameraP.p.z; z <= g_camera.drawDistance + cameraP.p.z; z++)
    {
        for (int32 x = -g_camera.drawDistance + cameraP.p.x; z <= g_camera.drawDistance + cameraP.p.x; x++)
        {
            //float vor = VoronoiNoise({ x, z }, 1.0f, -2.0f);
            //if ((vor > 0.5f - biLinearThreshold) && (vor < 0.5f + biLinearThreshold))

            float vor = VoronoiNoise({ float(x), float(z) }, 1.0f, -3.0f);
            if (vor)
            {
                int64 hash = PositionHash({ x, 0, z });
                if (s_biomePoints.find(hash) == s_biomePoints.end())
                {
                    srand(uint32(hash));
                    s_biomePoints[hash] = BiomeType(rand() % +BiomeType::Count);
                }
            }
        }
    }
}

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

void ChunkArray::SetBlocks(ChunkIndex i)
{
#define STB_METHOD 0
#define VORONOI 200

////________________________________________
////___________NEW IMPLIMENTATION___________
    WorldPos chunkGamePos = ToWorld(g_chunks->p[i]);
    NoiseParams seaFloorParams = {
        .numOfOctaves = 8,
        .freq = 0.17f,
        .weight = 1.0f,
        .gainFactor = 1.0f,
    };
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
    }

    //Step 2:
    //Generate heightmap of basic terrain
    uint32 heightMap[CHUNK_X][CHUNK_Z] = {};
    {
        uint32 waterVsLandHeight = 0;
        {
            for (uint32 x = 0; x < CHUNK_X; x++)
            {
                for (uint32 z = 0; z < CHUNK_Z; z++)
                {
                    //Fill with stone from HEIGHT_MIN_WATER to the determined height - 3
                    waterVsLandHeight = Clamp<uint32>(uint32(CHUNK_Y * Perlin2D({ (chunkGamePos.p.x + x) * perlinScale, (chunkGamePos.p.z + z) * perlinScale }, seaFloorParams)), HEIGHT_MIN_WATER, HEIGHT_MAX_WATER - 2);
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
                    heightMap[x][z] = waterVsLandHeight;
                }
            }
        }
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
            uint32 height = Clamp<uint32>(uint32(CHUNK_Y * Perlin2D(lookupLoc, seaFloorParams)), HEIGHT_MIN_WATER, HEIGHT_MAX_WATER - 2);
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

    //Step 5:
    //Generate terrain on Inland types
    {
        NoiseParams np;
        switch (g_chunks->terrainType[i])
        {
        case TerrainType::Plains:
        {
            np = {
                .numOfOctaves = 4,
                .freq = 0.75f,
                .weight = 1.0f,
                .gainFactor = 1.0f,
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
                .numOfOctaves = 4,
                .freq = 0.75f,
                .weight = 1.0f,
                .gainFactor = 1.0f,
            };
            break;
        }
        default:
        {
            assert(false);//(+chunkType) > +ChunkType::None && (+chunkType) < +ChunkType::Count);
            break;
        }
        }

        if (g_chunks->chunkType[i] == ChunkType::Inland)
        {
            for (uint32 x = 0; x < CHUNK_X; x++)
            {
                for (uint32 z = 0; z < CHUNK_Z; z++)
                {
                    Vec2 blockP = { chunkGamePos.p.x + x, chunkGamePos.p.z + z } ;

                    int32 yTotal = Clamp<int32>(static_cast<int32>(Perlin2D(blockP * perlinScale, np) * CHUNK_Y), 2, CHUNK_Y);

                    for (int32 y = heightMap[x][z]; y < yTotal - 3; y++)
                    {
                        g_chunks->blocks[i].e[x][y][z] = BlockType::Stone;
                        heightMap[x][z]++;
                    }

                    for (int32 y = heightMap[x][z]; y < yTotal - 1; y++)
                    {
                        g_chunks->blocks[i].e[x][y][z] = BlockType::Dirt;
                        heightMap[x][z]++;
                    }

                    for (int32 y = heightMap[x][z]; y < yTotal; y++)
                    {
                        g_chunks->blocks[i].e[x][y][z] = BlockType::Grass;
                        heightMap[x][z]++;
                    }
                }
            }
        }
    }


    ////float cellTypeVal = VoronoiNoise(chunkAndBlock / 64, 1.0f, 0.0f);


    //yTotal = Clamp<uint32>(waterVsLandHeight, 3, CHUNK_Y - 1); //add all together?

#if VORONOI == 9

    //Reference article:
    //https://www.gamasutra.com/blogs/JonGallant/20160211/264591/Procedurally_Generating_Wrapping_World_Maps_in_Unity_C__Part_4.php
    //

    float tempVal = {};
    float moistVal = {};
    BiomeTemp temp;
    BiomeMoist moist;
    BiomeType biome;
#if 1
    Vec2 chunkPForNoise = { float(g_chunks->p[i].p.x), float(g_chunks->p[i].p.z) };
    tempVal  = VoronoiNoise(chunkPForNoise / 180, 1.0f, 0.5f);
    moistVal = VoronoiNoise(chunkPForNoise / 60, 1.0f, 0.5f);
    temp  = BiomeTemp(Clamp<float>(tempVal   * +BiomeTemp::Count,  0.0f, 5.9f));
    moist = BiomeMoist(Clamp<float>(moistVal * +BiomeMoist::Count, 0.0f, 5.9f));
    biome = GetBiomeType(temp, moist);
#else
    NoiseParams tempParams = {
        .numOfOctaves = 4,
        .freq = 0.2f,
        .weight = 1.0f,
        .gainFactor = 0.5f,
    };
    NoiseParams moistParams = {
        .numOfOctaves = 4,
        .freq = 0.2f,
        .weight = 1.0f,
        .gainFactor = 0.5f,
    };
    tempVal = PerlinNoise(chunkPForNoise / 18, tempParams);
    moistVal = PerlinNoise(chunkPForNoise / 6, moistParams);
    temp = BiomeTemp(Clamp<float>(tempVal * +BiomeTemp::Count, 0.0f, 5.9f));
    moist = BiomeMoist(Clamp<float>(moistVal * +BiomeMoist::Count, 0.0f, 5.9f));
    biome = GetBiomeType(temp, moist);
#endif
#endif
    //PROFILE_SCOPE("SetBlocks() ");
    BlockType options[] = {
        BlockType::Empty,
        BlockType::Grass,
        BlockType::Dirt,
        BlockType::Stone,
        BlockType::IronBlock,
    };
    BlockType topBlockType = BlockType::Empty;


////________________________________________
////___________NEW IMPLIMENTATION___________
//
//    GamePos chunkInGame = ToGame(g_chunks->p[i]);
//
//    NoiseParams np = {
//        .numOfOctaves = 8,
//        .freq = 0.17f,
//        .weight = 1.0f,
//        .gainFactor = 1.0f,
//    };
//
//    WorldPos chunkMeaningfullFloatLocation = ToWorld(chunkInGame);
//    //Vec3 cell = Voronoi_DAndP(Vec2({ chunkMeaningfullFloatLocation.p.x, chunkMeaningfullFloatLocation.p.z }) * 100 / float(INT_MAX));
//    Vec3 cell = Voronoi_DAndP(Vec2({ chunkMeaningfullFloatLocation.p.x, chunkMeaningfullFloatLocation.p.z}) / 100.0);
//    cell = cell * 16.0f;// * 10 * 16 * 4 + 0.5;
//    Vec2Int cellCenterInt = { int32(cell.x), int32(cell.y) };
//
//    //Vec2Int cellCenterInt = { int32(cell.x * float(INT_MAX)), int32(cell.y * float(INT_MAX)) };
//    //Vec2Int cellCenterInt = { int32(cell.x * float(INT_MAX)), int32(cell.z * float(INT_MAX)) };
//    Vec2 cellCenterFloat = /*Vec2({ chunkMeaningfullFloatLocation.p.x, chunkMeaningfullFloatLocation.p.x }) + */Vec2({ cellCenterInt.x / 1.0f, cellCenterInt.y / 1.0f });
//
//    uint32 centerCellHeight = NewHightFunc(cellCenterFloat);
//    //uint32 centerCellHeight = Clamp<uint32>(uint32(CHUNK_Y * Perlin2D(cellCenterFloat, np)), MIN_WATER_HEIGHT, MAX_WATER_HEIGHT - 2);
//
//    CellType cellType = CellType::Count;
//    if (centerCellHeight > MAX_WATER_HEIGHT - 2 - 2)
//    {
//        cellType = CellType::Inland;
//    }
//    else
//    {
//        cellType = CellType::Ocean;
//    }
//
////_________END NEW IMPLIMENTATION_________
////________________________________________

    for (int32 x = 0; x < CHUNK_X; x++)
    {
        for (int32 z = 0; z < CHUNK_Z; z++)
        {
            GamePos blockP = { x, 0, z };
            GamePos chunkBlockP = Convert_ChunkIndexToGame(i);

            Vec2 blockRatio = { static_cast<float>(chunkBlockP.p.x + blockP.p.x), static_cast<float>(chunkBlockP.p.z + blockP.p.z) };

            const int32 blockRatioProduct = 100;
            blockRatio /= blockRatioProduct;
            int32 yTotal = 0;
            //BlockType topBlockType;// = BlockType::Grass;



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
            default:
            {
                assert(false);//(+chunkType) > +ChunkType::None && (+chunkType) < +ChunkType::Count);
                break;
            }
            }

            yTotal = Clamp<uint32>(static_cast<int32>(Perlin2D(blockRatio, np) * CHUNK_Y), 10, CHUNK_Y - 1);
#endif

////___________OLD IMPLIMENTATION___________
////________________________________________


////________________________________________
////___________NEW IMPLIMENTATION___________
//
//
//            //Step 1:
//            //Generate heightmap of basic terrain
//            //uint32 waterVsLandHeight = Clamp<uint32>(uint32(CHUNK_Y * Perlin2D(blockRatio, np)), MIN_WATER_HEIGHT, MAX_WATER_HEIGHT - 2);
//            uint32 waterVsLandHeight = NewHightFunc(blockRatio * 100);
//
//            BlockType topBlockType;// = BlockType::Grass;
//
//            //Step 2:
//            //Generate Cells using Voronoi and determine what type of region that cell is
//            // - Ocean, Coastal, or Inland
//            //float cellRegion = VoronoiNoise(blockRatio, 1.0f, 0.5f);
//            
//            uint32 cellTypeHeight = 0;
//            switch (cellType)
//            {
//            case CellType::Inland:
//
//                np = {
//                    .numOfOctaves = 8,
//                    .freq = 0.4f,
//                    .weight = 1.5f,
//                    .gainFactor = 0.9f,
//                };
//                cellTypeHeight = Clamp<uint32>(uint32(CHUNK_Y * Perlin2D(blockRatio, np)), 0, CHUNK_Y - (MAX_WATER_HEIGHT - 2));
//                topBlockType = BlockType::Grass;
//                break;
//            case CellType::Ocean:
//                topBlockType = BlockType::Water;
//                break;
//            case CellType::Coastal:
//                break;
//            default:
//                assert(false);
//            }
//
//
//            
//            //union CellGarbage {
//            //    uint64 a;
//            //    Vec2 v;
//            //};
//            //CellGarbage cellGarbage;
//            //cellGarbage.v = { cell.x, cell.y };
//            //uint32 cellHash = PCG_Random(cellGarbage.a);
//            //CellType cellType = static_cast<CellType>(cellHash % +CellType::Count);
//
//
//
//            yTotal = Clamp<uint32>(waterVsLandHeight + cellTypeHeight, 3, CHUNK_Y - 1); //add all together?
#if VORONOI == 11
#elif VORONOI == 10
#elif VORONOI == 9

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
            default:
            {
                assert(false);//(+chunkType) > +ChunkType::None && (+chunkType) < +ChunkType::Count);
                break;
            }
            }

            yTotal = Clamp<uint32>(static_cast<int32>(Perlin2D(blockRatio, np) * CHUNK_Y), 10, CHUNK_Y - 1);


#elif VORONOI == 7

            blockRatio /= 2;

            float vor = VoronoiNoise(blockRatio, 1.0f, 1.0f);
            NoiseParams np;
            BiomeType biome = BiomeType(int32(vor * 10) % +BiomeType::Count);
            switch (biome)
            {
            case BiomeType::Plains:
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
            case BiomeType::Mountain:
            {
                np = {
                    .numOfOctaves = 8,
                    .freq = 0.4f,
                    .weight = 1.5f,
                    .gainFactor = 0.9f,
                };
				topBlockType = BlockType::Stone;

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

            yTotal = Clamp<uint32>(static_cast<int32>(PerlinNoise(blockRatio, np) * CHUNK_Y), 10, CHUNK_Y - 1);

            //else
            //{
            //    vor = ((vor - plainsSetpoint) / (mountainSetpoint - plainsSetpoint));
            //    //DebugPrint("B Vor: %f\n", vor);
            //    //vor = (vor - plainsSetpoint) / (mountainSetpoint - plainsSetpoint);
            //    if (RandomFloat(-1.0f, 1.0f) > 0)
            //        topBlockType = BlockType::Grass;
            //    else
            //        topBlockType = BlockType::Stone;

            //    float mountainHeight = static_cast<float>(Clamp<uint32>(static_cast<int32>(PerlinNoise(blockRatio, mountainParams) * CHUNK_Y), 10, CHUNK_Y - 1));
            //    float plainsHeight = static_cast<float>(Clamp<uint32>(static_cast<int32>(PerlinNoise(blockRatio, plainParams) * CHUNK_Y), 10, CHUNK_Y - 1));
            //    yTotal = static_cast<uint32>(Lerp<float>(plainsHeight, mountainHeight, vor));
            //}


#elif VORONOI == 6

            //blockRatio /= 2;

            //float vor = (VoronoiNoise(blockRatio, 1.0f, 0.0f) + 1.0f) / 2;
            float vor = VoronoiNoise(blockRatio, 1.0f, 0);
            NoiseParams mountainParams = {
                .numOfOctaves = 8,
                .freq = 0.4f,
                .weight = 1.5f,
                .gainFactor = 0.9f,
            };
            NoiseParams plainParams = {
                .numOfOctaves = 4,
                .freq = 0.75f,
                .weight = 1.0f,
                .gainFactor = 1.0f,
            };

            float dupe = 0.5f;
            float mountainSetpoint = dupe;
            float plainsSetpoint = dupe;

            if (vor > mountainSetpoint)
            {
                yTotal = Clamp<uint32>(static_cast<int32>(PerlinNoise(blockRatio, mountainParams) * CHUNK_Y), 10, CHUNK_Y - 1);
                topBlockType = BlockType::Stone;
            }
            else// if (vor < plainsSetpoint)
            {
                yTotal = Clamp<uint32>(static_cast<int32>(PerlinNoise(blockRatio, plainParams) * CHUNK_Y), 10, CHUNK_Y - 1);
                topBlockType = BlockType::Grass;
            }
            //else
            //{
            //    vor = ((vor - plainsSetpoint) / (mountainSetpoint - plainsSetpoint));
            //    //DebugPrint("B Vor: %f\n", vor);
            //    //vor = (vor - plainsSetpoint) / (mountainSetpoint - plainsSetpoint);
            //    if (RandomFloat(-1.0f, 1.0f) > 0)
            //        topBlockType = BlockType::Grass;
            //    else
            //        topBlockType = BlockType::Stone;

            //    float mountainHeight = static_cast<float>(Clamp<uint32>(static_cast<int32>(PerlinNoise(blockRatio, mountainParams) * CHUNK_Y), 10, CHUNK_Y - 1));
            //    float plainsHeight = static_cast<float>(Clamp<uint32>(static_cast<int32>(PerlinNoise(blockRatio, plainParams) * CHUNK_Y), 10, CHUNK_Y - 1));
            //    yTotal = static_cast<uint32>(Lerp<float>(plainsHeight, mountainHeight, vor));
            //}

#elif VORONOI == 5

            Vec2 blockPosScaled = blockRatio;
            ChunkPos chunkP = ToChunk(chunkBlockP);

            ChunkPos biomeIndices = chunkP / 10;
            //ChunkType chunkType = RandomBiome(biomeIndices);
            ChunkType chunkType;// = ChunkType::None;
#if 0
            //chunkType = static_cast<ChunkType>((static_cast<uint32>(fabsf(static_cast<float>(biomeIndices.x + biomeIndices.y + biomeIndices.z))) % (+ChunkType::Count - 1)) + 1);
            uint8 a = (biomeIndices.x % CHAR_MAX) + CHAR_MAX;
            uint8 b = (biomeIndices.z % CHAR_MAX) + CHAR_MAX;
            chunkType = static_cast<ChunkType>(static_cast<uint8>(GetRandomUint8(a) +
                                                                  GetRandomUint8(b)) % +ChunkType::Count);
#endif
            int64 _hash = PositionHash(biomeIndices);
            uint64 hash = *(uint64*)(&_hash);
            hash ^= hash << 13;
            hash ^= hash >> 17;
            hash ^= hash << 5;

            chunkType = static_cast<ChunkType>(hash % +ChunkType::Count);

            chunkTypes[+chunkType]++;
            NoiseParams noiseParams = {};

            switch (chunkType)
            {
            case ChunkType::Plains:
			{
				noiseParams = {
					.numOfOctaves = 4,
					.freq = 0.75f,
					.weight = 1.0f,
					.gainFactor = 1.0f,
				};
				topBlockType = BlockType::Grass;

                break;
            }
            case ChunkType::Mountain:
            {
				noiseParams = {
					.numOfOctaves = 8,
					.freq = 0.4f,
					.weight = 1.5f,
					.gainFactor = 0.9f,
				};
				topBlockType = BlockType::Stone;

                break;
            }
            case ChunkType::Desert:
            {

				noiseParams = {
					.numOfOctaves = 1,
					.freq = 1.0f,
					.weight = 1.0f,
					.gainFactor = 0.5f,
				};
				topBlockType = BlockType::Sand;

                break;
            }
            default:
            {
                assert(false);//(+chunkType) > +ChunkType::None && (+chunkType) < +ChunkType::Count);
                break;
            }
            }
			yTotal = Clamp<uint32>(static_cast<int32>(PerlinNoise(blockRatio / 2, noiseParams) * CHUNK_Y), 10, CHUNK_Y - 1);
#endif
#if 0

            //#if NOISETYPE == 2
            //            //int32 yTotal = Max(static_cast<int32>(Noise(blockRatio) * CHUNK_Y), 10);
            //            blockRatio /= 100;
            //            int32 yTotal = GenerateTerrainHeight(10, CHUNK_Y, blockRatio);
            //#elif NOISETYPE == 4
            //            blockRatio /= 100;
            //            int32 yTotal = (static_cast<int32>(Noise(blockRatio) * CHUNK_Y), 80);
            //#else
            //            static_assert(false, "Need to set noise implimentation variabls in SetBlocks()");
            //#endif
            assert(yTotal >= 0 && yTotal < CHUNK_Y);


            for (int32 y = 0; y < yTotal; y++)
            {

                BlockType bt = BlockType::Empty;
                //if (y > CHUNK_Y / 2)
                //{
                //  bt = BlockType::Empty;
                //}
                //else
                {
                    if (y == yTotal - 1)
                    {
                        //bt = BlockType::Grass;
                        bt = topBlockType;
                    }
                    else if (y > yTotal - 4)
                    {
                        bt = BlockType::Dirt;
                        //uint32 random = RandomU32(+BlockType::Grass, static_cast<uint32>(arrsize(options)));
                    }
                    else
                    {
                        //uint32 random = RandomU32(+BlockType::Stone, static_cast<uint32>(arrsize(options)));
                        //uint32 random = RandomU32(0, static_cast<uint32>(arrsize(options)));
                        //bt = options[random];
                        bt = BlockType::Stone;
                    }
                }
                blocks[i].e[x][y][z] = bt;
            }
#endif
        }
    }

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
    return Abs({ inputP.p.x - chunkP.p.x, inputP.p.y - chunkP.p.y, inputP.p.z - chunkP.p.z });
}

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
        if (g_chunks->p[center].p == newChunkPos.p)
        {
            assert(newRelBlockP == blockRelP);
            result = g_chunks->blocks[center].e[newRelBlockP.x][newRelBlockP.y][newRelBlockP.z];
            return true;
        }
        for (int32 i = 0; i < arrsize(neighbors); i++)
        {

            if (g_chunks->p[neighbors[i]].p == newChunkPos.p)
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
    ChunkPos centerChunk = g_chunks->p[i];
    for (int32 z = -1; z <= 1; z++)
    {
        for (int32 x = -1; x <= 1; x++)
        {
            if (x == 0 && z == 0)
                continue;
            ChunkIndex chunkIndex = 0;
            ChunkPos newChunkP = { centerChunk.p.x + x, 0, centerChunk.p.z + z };
            int64 tesHash = PositionHash(newChunkP);
            if (g_chunks->GetChunkFromPosition(chunkIndex, newChunkP))
            {
                if (g_chunks->state[chunkIndex] >= ChunkArray::BlocksLoaded)
                {
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

void ChunkArray::BuildChunkVertices(RegionSampler region)
{
    ChunkIndex i = region.center;
    faceVertices[i].clear();
    faceVertices[i].reserve(10000);
    uploadedIndexCount[i] = 0;
    GamePos realP = Convert_ChunkIndexToGame(i);
    for (int32 x = 0; x < CHUNK_X; x++)
    {
        for (int32 y = 0; y < CHUNK_Y; y++)
        {
            for (int32 z = 0; z < CHUNK_Z; z++)
            {
                for (uint32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
                {
                    BlockType currentBlockType = blocks[i].e[x][y][z];
                    if (currentBlockType == BlockType::Empty)
                        continue;

                    Vec3Int vf = Vec3ToVec3Int(faceNormals[faceIndex]);
                    int32 xReal = x + vf.x;
                    int32 yReal = y + vf.y;
                    int32 zReal = z + vf.z;

                    bool outOfBounds = (xReal >= CHUNK_X || yReal >= CHUNK_Y || zReal >= CHUNK_Z ||
                        xReal < 0 || yReal < 0 || zReal < 0);

                    BlockType type;
                    if (region.GetBlock(type, {xReal, yReal, zReal}) && type == BlockType::Empty)
                    {
                        VertexFace f = {};// = cubeFaces[faceIndex];
                        Vec3 offset = { static_cast<float>(x + realP.p.x), static_cast<float>(y + realP.p.y), static_cast<float>(z + realP.p.z) };

                        f.a.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;
                        f.b.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;
                        f.c.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;
                        f.d.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;

                        f.a.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];
                        f.b.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];
                        f.c.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];
                        f.d.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];

                        f.a.nAndConnectedVertices = 0xF0 & (faceIndex << 4);
                        f.b.nAndConnectedVertices = 0xF0 & (faceIndex << 4);
                        f.c.nAndConnectedVertices = 0xF0 & (faceIndex << 4);
                        f.d.nAndConnectedVertices = 0xF0 & (faceIndex << 4);

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

    uint32 vertIndex = 0;
    //-X and -Z
    for (Vertex_Chunk& vert : faceVertices[i])
    {
        uint8 normal = (vert.nAndConnectedVertices & 0xF0) >> 4;
        Vec3Int blockN = Vec3ToVec3Int(faceNormals[normal]);
        Vec3Int blockP = GetBlockPosFromIndex(vert.blockIndex);

        uint8 faceIndex = normal;
        Vec3Int a = *(&vertexBlocksToCheck[faceIndex].e0 + (vertIndex + 0));
        Vec3Int b = *(&vertexBlocksToCheck[faceIndex].e0 + (vertIndex + 1));
        Vec3Int c = a + b;

        BlockType at = BlockType::Grass;
        BlockType bt = BlockType::Grass;
        BlockType ct = BlockType::Grass;


        //ChunkIndex neighbors[8];  This is what neighbors is
        if (!region.GetBlock(at, blockP + blockN + a) || !region.GetBlock(bt, blockP + blockN + b) ||
            !region.GetBlock(ct, blockP + blockN + c))
        {
            //failed to get block needs to be redone;
            g_chunks->flags[i] |= CHUNK_RESCAN_BLOCKS;
        }
        else
            g_chunks->flags[i] &= ~(CHUNK_RESCAN_BLOCKS);
        if (at != BlockType::Empty)
            vert.nAndConnectedVertices += 1;
        if (bt != BlockType::Empty)
            vert.nAndConnectedVertices += 1;
        if (ct != BlockType::Empty)
            vert.nAndConnectedVertices += 1;

        vertIndex += 2;
        vertIndex = vertIndex % 8;
    }
}

void ChunkArray::UploadChunk(ChunkIndex i)
{
    vertexBuffer[i].Upload(faceVertices[i].data(), faceVertices[i].size());
    std::vector<Vertex_Chunk> faces;
    faceVertices[i].swap(faces);
    g_chunks->state[i] = ChunkArray::Uploaded;
}

void PreChunkRender(const Mat4& perspective)
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
    sp->UpdateUniformMat4("u_view",        1, false, g_camera.view.e);

#if DIRECTIONALLIGHT == 1
    sp->UpdateUniformVec3("u_directionalLight_d",  1,  g_light.d.e);
#else
    sp->UpdateUniformVec3("u_lightColor",  1,  g_light.c.e);
    sp->UpdateUniformVec3("u_lightP",      1,  g_light.p.e);
#endif
    sp->UpdateUniformVec3("u_cameraP",     1,  g_camera.p.p.e);

    sp->UpdateUniformUint8("u_CHUNK_X", CHUNK_X);
    sp->UpdateUniformUint8("u_CHUNK_Y", CHUNK_Y);
    sp->UpdateUniformUint8("u_CHUNK_Z", CHUNK_Z);

    Material material;
    material.ambient = { 0.02f, 0.02f, 0.02f };
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
    g_chunks->state[region.center] = ChunkArray::VertexLoaded;
    region.DecrimentRefCount();
}

void DrawBlock(WorldPos p, Color color, float scale, const Mat4& perspective)
{
    DrawBlock(p, color, { scale, scale, scale }, perspective);
}

void DrawBlock(WorldPos p, Color color, Vec3 scale, const Mat4& perspective)
{

    if (g_renderer.cubeVertexBuffer == nullptr)
    {
        const Vec3 cubeVertices[] = {
            // +x
            gb_vec3(1.0, 1.0, 1.0),
            gb_vec3(1.0, 0, 1.0),
            gb_vec3(1.0, 1.0, 0),
            gb_vec3(1.0, 0, 0),

            // -x
            gb_vec3(0, 1.0, 0),
            gb_vec3(0, 0, 0),
            gb_vec3(0, 1.0, 1.0),
            gb_vec3(0, 0, 1.0),

            // +y
            gb_vec3(1.0, 1.0, 1.0),
            gb_vec3(1.0, 1.0, 0),
            gb_vec3(0, 1.0, 1.0),
            gb_vec3(0, 1.0, 0),

            // -y
            gb_vec3(0, 0, 1.0),
            gb_vec3(0, 0, 0),
            gb_vec3(1.0, 0, 1.0),
            gb_vec3(1.0, 0, 0),

            // z
            gb_vec3(0, 1.0, 1.0),
            gb_vec3(0, 0, 1.0),
            gb_vec3(1.0, 1.0, 1.0),
            gb_vec3(1.0, 0, 1.0),

            // -z
            gb_vec3(1.0, 1.0, 0),
            gb_vec3(1.0, 0, 0),
            gb_vec3(0, 1.0, 0),
            gb_vec3(0, 0, 0),
        };

        g_renderer.cubeVertexBuffer = new VertexBuffer();

        Vertex vertices[arrsize(cubeVertices)] = {};

        for (int32 i = 0; i < arrsize(cubeVertices); i++)
        {
            vertices[i].p = cubeVertices[i] - 0.5f;
        }

        g_renderer.cubeVertexBuffer->Upload(vertices, arrsize(vertices));
    }

    g_renderer.cubeVertexBuffer->Bind();
    g_renderer.chunkIB->Bind();

    Mat4 transform;
    gb_mat4_translate(&transform, { p.p.x, p.p.y, p.p.z });

    ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
    sp->UseShader();
    sp->UpdateUniformMat4("u_perspective", 1, false, perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, g_camera.view.e);
    sp->UpdateUniformMat4("u_model",       1, false, transform.e);
    sp->UpdateUniformVec3("u_scale",       1,        scale.e);

    sp->UpdateUniformVec4("u_color",       1,        color.e);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glDisableVertexArrayAttrib(g_renderer.vao, 1);
    glDisableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += 36 / 3;
}
