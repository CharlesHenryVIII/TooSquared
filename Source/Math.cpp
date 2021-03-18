#include "Math.h"
#include "Misc.h"
#include "Block.h"


//uint32 PCG32_Random_R(uint64& state, uint64& inc)
//{
//    uint64 oldstate = state;
//    state = oldstate * 6364136223846793005ULL + inc;
//    uint32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
//    uint32 rot = oldstate >> 59u;
//    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
//}
//
//void PCG32_SRandom_R(uint64 initstate, uint64 initseq)
//{
//    uint64 state = 0U;
//    uint64 inc = (initseq << 1u) | 1u;
//    PCG32_Random_R(state, inc);
//    state += initstate;
//    PCG32_Random_R(state, inc);
//}
//
//void PCG32_SRandom(uint64 seed, uint64 seq)
//{
//    PCG32_SRandom_R(seed, seq);
//}
//
//uint32 PCG32_BoundedRand_R(uint64 seed, uint64 seq, uint32 bound)
//{
//    // To avoid bias, we need to make the range of the RNG a multiple of
//    // bound, which we do by dropping output less than a threshold.
//    // A naive scheme to calculate the threshold would be to do
//    //
//    //     uint32_t threshold = 0x100000000ull % bound;
//    //
//    // but 64-bit div/mod is slower than 32-bit div/mod (especially on
//    // 32-bit platforms).  In essence, we do
//    //
//    //     uint32_t threshold = (0x100000000ull-bound) % bound;
//    //
//    // because this version will calculate the same modulus, but the LHS
//    // value is less than 2^32.
//
//    uint32_t threshold = -bound % bound;
//
//    // Uniformity guarantees that this loop will terminate.  In practice, it
//    // should usually terminate quickly; on average (assuming all bounds are
//    // equally likely), 82.25% of the time, we can expect it to require just
//    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
//    // (i.e., 2147483649), which invalidates almost 50% of the range.  In 
//    // practice, bounds are typically small and only a tiny amount of the range
//    // is eliminated.
//    for (;;) {
//        uint32 r = PCG32_Random_R(seed, seq);
//        if (r >= threshold)
//            return r % bound;
//    }
//}
//
//uint32 PCG32_BoundedRand(uint64 seed, uint64 seq, uint32 bound)
//{
//    return PCG32_BoundedRand_R(seed, seq, bound);
//}
//
//
////uint64 random(uint128 state)
////{
////    uint64 result = rotate64(uint64(state ^ (state >> 64)), state >> 122);
////    return result;
////}
//

uint32 PCG_Random(uint64 state)
{
    uint32 result = static_cast<uint32>((state ^ (state >> 22)) >> (22 + (state >> 61)));
    return result;
}


//[[nodiscard]] float Random(const float min, const float max)
//{
//    return min + (max - min) * (rand() / float(RAND_MAX));
//}

//uint32 RandomU32(uint32 min, uint32 max)
//{
//	return (_RandomU32() % (max - min)) + min;
//}

#if 1

float Bilinear(float p00, float p10, float p01, float p11, float x, float y)
{
   float p0 = Lerp(y, p00, p01);
   float p1 = Lerp(y, p10, p11);
   return Lerp(x, p0, p1);
}
#else
[[nodiscard]] float Bilinear(Vec2 p, Rect loc, float bl, float br, float tl, float tr)
{
    float denominator = ((loc.topRight.x - loc.botLeft.x) * (loc.topRight.y - loc.botLeft.y));

    float xLeftNum  = (loc.topRight.x - p.x);
    float xRightNum = (p.x            - loc.botLeft.x);
    float yBotNum   = (loc.topRight.y - p.y);
    float yTopNum   = (p.y            - loc.botLeft.y);

    float c1 = bl * ((xLeftNum  * yBotNum) / denominator);
    float c2 = br * ((xRightNum * yBotNum) / denominator);
    float c3 = tl * ((xLeftNum  * yTopNum) / denominator);
    float c4 = tr * ((xRightNum * yTopNum) / denominator);

    return c1 + c2 + c3 + c4;
}
#endif

[[nodiscard]] float Cubic( Vec4 v, float x )
{
    float a = 0.5f * (v.w - v.x) + 1.5f * (v.y - v.z);
    float b = 0.5f * (v.x + v.z) - v.y - a;
    float c = 0.5f * (v.z - v.x);
    float d = v.y;

    return d + x * (c + x * (b + x * a));
}

[[nodiscard]] float Bicubic(Mat4 p, Vec2 pos)
{
    Vec4 a;
    a.e[0] = Cubic(p.col[0], pos.y);
    a.e[1] = Cubic(p.col[1], pos.y);
    a.e[2] = Cubic(p.col[2], pos.y);
    a.e[3] = Cubic(p.col[3], pos.y);
    return Cubic(a, pos.x);
}


[[nodiscard]] GamePos ToGame(ChunkPos a)
{
    return { a.p.x * static_cast<int32>(CHUNK_X), a.p.y * static_cast<int32>(CHUNK_Y), a.p.z * static_cast<int32>(CHUNK_Z) };
}

[[nodiscard]] ChunkPos ToChunk(GamePos a)
{
    ChunkPos result = { static_cast<int32>(floorf(static_cast<float>(a.p.x) / static_cast<float>(CHUNK_X))),
                        static_cast<int32>(floorf(static_cast<float>(a.p.y) / static_cast<float>(CHUNK_Y))),
                        static_cast<int32>(floorf(static_cast<float>(a.p.z) / static_cast<float>(CHUNK_Z))) };
    return result;
}
[[nodiscard]] WorldPos ToWorld(GamePos a)
{
    return { static_cast<float>(a.p.x), static_cast<float>(a.p.y), static_cast<float>(a.p.z) };
}

[[nodiscard]] GamePos ToGame(WorldPos a)
{
    GamePos result = { static_cast<int32>(a.p.x), static_cast<int32>(a.p.y), static_cast<int32>(a.p.z) };
    return result;
}
[[nodiscard]] ChunkPos ToChunk(WorldPos a)
{
    return ToChunk(ToGame(a));
}

static void matd_mul(float out[4][4], float src1[4][4], float src2[4][4])
{
   int i,j,k;
   for (j=0; j < 4; ++j) 
      for (i=0; i < 4; ++i) {
         float t=0;
         for (k=0; k < 4; ++k)
            t += src1[k][i] * src2[j][k];
         out[i][j] = t;
      }
}

Frustum ComputeFrustum(const Mat4& in)
{
    Frustum result = {};
    Mat4 test1 = {};
    test1.col[3] = {1, 20, 0, 1};
    test1.col[0] = {3, 4, 0, 0};
    test1.col[1] = {300, 0, 1, 30};
    Mat4 test2 = {};
    test2.col[3] = {20, 0.11f, 13, 0};
    test2.col[0] = {3, 12, 0.12f, 0.3f};
    test2.col[1] = {0, 1, 100, 18};

    float out[4][4];
    float in1[4][4];
    float in2[4][4];

    memcpy(in1, test1.e, sizeof(in1));
    memcpy(in2, test2.e, sizeof(in2));

    matd_mul(out, in1, in2);
    Mat4 testOut = test1 * test2;
    Mat4 mvProj = in;
    gb_mat4_transpose(&mvProj);

    for (int32 i = 0; i < 4; ++i)
    {
        (&result.e[0].x)[i] = mvProj.col[3].e[i] + mvProj.col[0].e[i];
        (&result.e[1].x)[i] = mvProj.col[3].e[i] - mvProj.col[0].e[i];
        (&result.e[2].x)[i] = mvProj.col[3].e[i] + mvProj.col[1].e[i];
        (&result.e[3].x)[i] = mvProj.col[3].e[i] - mvProj.col[1].e[i];
        (&result.e[4].x)[i] = mvProj.col[3].e[i] + mvProj.col[2].e[i];
        (&result.e[5].x)[i] = mvProj.col[3].e[i] - mvProj.col[2].e[i];
    }
    return result;
}

int32 TestPlane(const Plane *p, float x0, float y0, float z0, float x1, float y1, float z1)
{
   // return false if the box is entirely behind the plane
   float d = 0;
   assert(x0 <= x1 && y0 <= y1 && z0 <= z1);
   if (p->x > 0) d += x1 * p->x; else d += x0 * p->x;
   if (p->y > 0) d += y1 * p->y; else d += y0 * p->y;
   if (p->z > 0) d += z1 * p->z; else d += z0 * p->z;
   return d + p->w >= 0;
}

bool IsBoxInFrustum(const Frustum& f, float *bmin, float *bmax)
{
   int32 i;
   for (i=0; i < 5; ++i)
      if (!TestPlane(&f.e[i], bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]))
         return 0;
   return 1;
}

int32 ManhattanDistance(Vec3Int a, Vec3Int b)
{
    return abs(a.x - b.x) + abs(a.y - b.y) + abs(a.z - b.z);
}
