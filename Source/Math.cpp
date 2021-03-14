
#include "Math.h"
#include "Misc.h"
#include "Block.h"

[[nodiscard]] float Random(const float min, const float max)
{
    return min + (max - min) * (rand() / float(RAND_MAX));
}

//static uint32 _RandomU32()
//{
//    uint32 result = rand();
//    result ^= result << 13;
//    result ^= result >> 17;
//    result ^= result << 5;
//    return result;
//}

//uint32 RandomU32(uint32 min, uint32 max)
//{
//	return (_RandomU32() % (max - min)) + min;
//}


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
    return { a.x * static_cast<int32>(CHUNK_X), a.y * static_cast<int32>(CHUNK_Y), a.z * static_cast<int32>(CHUNK_Z) };
}

[[nodiscard]] ChunkPos ToChunk(GamePos a)
{
    ChunkPos result = { static_cast<int32>(floorf(static_cast<float>(a.x) / static_cast<float>(CHUNK_X))),
                        static_cast<int32>(floorf(static_cast<float>(a.y) / static_cast<float>(CHUNK_Y))),
                        static_cast<int32>(floorf(static_cast<float>(a.z) / static_cast<float>(CHUNK_Z))) };
    return result;
}
[[nodiscard]] WorldPos ToWorld(GamePos a)
{
    return { static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z) };
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

