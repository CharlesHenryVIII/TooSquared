#include "Math.h"
#define NOISETYPE 2


struct NoiseParams {
    int32 numOfOctaves = 8;
    float freq = 0.2f;
    float weight = 1.0f;
    float gainFactor; //"H" 0.5 to 1.0 Generally 0.5
};


Vec3 Voronoi_DAndP(Vec2 v);

void NoiseInit();
float Perlin2D(Vec2 a, NoiseParams np);
float VoronoiNoise(Vec2 x, float u, float v);
uint8 GetRandomUint8(uint8 a);
int32 BigNoise(Vec2Int p, int32 s, uint32 seed);

#if NOISETYPE == 2
float Perlin3D(Vec3 in, Vec3Int wrap);
#endif
//float Noise(Vec3 a);
