#include "Math.h"
#define NOISETYPE 2


struct NoiseParams {
    int32 numOfOctaves = 8;
    float freq = 0.2f;
    float weight = 1.0f;
    float gainFactor; //"H" 0.5 to 1.0 Generally 0.5
};


void NoiseInit();
float PerlinNoise(Vec2 a, NoiseParams np);
float VoronoiNoise(Vec2 x, float u, float v);
int32 GenerateTerrainHeight(int32 min, int32 max, Vec2 input);
//float Noise(Vec3 a);
