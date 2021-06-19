#include "Misc.h"
#include "Math.h"
#define NOISETYPE 2


struct NoiseParams {
    int32 numOfOctaves = 8;
    float freq = 0.2f;
    float weight = 1.0f;
    float gainFactor = 0.5f; //"H" 0.5 to 1.0 Generally 0.5
};

enum class VoronoiEdges {
    West,
    North,
    East,
    South,
    Count,
};
ENUMOPS(VoronoiEdges);

extern Vec2Int vornoiEdges[+VoronoiEdges::Count];

struct VoronoiCell {
    GamePos m_center = {};
    LineSegment m_lines[+VoronoiEdges::Count] = {};

    void BuildCell(GamePos* corners, GamePos cellCenter);
    [[nodiscard]] bool Contains(GamePos blockP);
    [[nodiscard]] std::vector<float> DistanceToEachLineSegment(const GamePos& blockP);
    [[nodiscard]] uint32 GetHash(uint64 worldSeed);
};

struct VoronoiResult {
    VoronoiCell cell;
    float distance;
};

struct VoronoiRegion {
    const int32 m_cellSize = 10; //size of the area for sampling positions
    const int32 m_apronDistance = 4;
    ChunkPos Debug_referenceChunkP;
    Vec2Int Debug_referenceCellP;
    std::vector<VoronoiCell> cells;

    [[nodiscard]] VoronoiCell* GetCell(GamePos p);
    [[nodiscard]] ChunkPos CellToChunk(Vec2Int cellP);
    [[nodiscard]] GamePos CellToGame(Vec2Int cellP);
    [[nodiscard]] Vec2Int ToCell(ChunkPos p);
    [[nodiscard]] Vec2Int ToCell(GamePos p);
    
    //VoronoiCell* GetNeighbors(GamePos blockP);
    VoronoiCell* HuntForBlock(VoronoiCell* baseCell, const Vec2& basePos, const Vec2& _direction);
    //TODO: Improve
    std::vector<VoronoiResult> GetVoronoiDistancesAndNeighbors(VoronoiCell* blockCell, const GamePos& blockP);
    void BuildRegion(ChunkPos chunkP, uint64 worldSeed);
};



Vec3 Voronoi_DAndP(Vec2 v);

void NoiseInit();
float Perlin2D(Vec2 a, NoiseParams np);
float VoronoiNoise(Vec2 x, float u, float v);
uint8 GetRandomUint8(uint8 a);
int32 BigNoise(Vec2Int p, int32 s, uint32 seed);
uint32 XXSeedHash(uint64 seed, int32 buf);
uint32 XXSeedHash(uint64 seed, int64 buf);

#if NOISETYPE == 2
float Perlin3D(Vec3 in, Vec3Int wrap);
#endif
//float Noise(Vec3 a);
