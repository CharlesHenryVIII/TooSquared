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
//uint64 PCG_Random(uint128 state)
//{
//    uint64 result = rotate64(uint64(state ^ (state >> 64)), state >> 122);
//    return result;
//}
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
[[nodiscard]] WorldPos ToWorld(ChunkPos a)
{
    return ToWorld(ToGame(a));
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

bool RayVsAABB(const Ray& ray, const AABB& box, float& min, Vec3& intersect, Vec3& normal)
{
    float tmin = 0;
    float tmax = FLT_MAX;

    for (int32 slab = 0; slab < 3; ++slab)
    {
        if (::fabs(ray.direction.e[slab]) < FLT_EPSILON)
        {
            // Ray is parallel to the slab
            if (ray.origin.e[slab] < box.min.e[slab] || ray.origin.e[slab] > box.max.e[slab])
                return false;
        }
        else
        {

            float ood = 1.0f / ray.direction.e[slab];
            float t1 = (box.min.e[slab] - ray.origin.e[slab]) * ood;
            float t2 = (box.max.e[slab] - ray.origin.e[slab]) * ood;
            if (t1 > t2)
            {
                std::swap(t1, t2);
            }

            tmin = Max(tmin, t1);
            tmax = Min(tmax, t2);

            if (tmin > tmax)
                return false;

        }
    }


    intersect = ray.origin + ray.direction * tmin;
    min = tmin;

    Vec3 center = (box.max + box.min) / 2.0f;
    Vec3 toNormal = intersect - center;
    Vec3 normalized = Normalize(toNormal);
    Vec3 normals[] = {
        {1, 0, 0},
        {-1, 0, 0},
        {0, 1, 0},
        {0, -1, 0},
        {0, 0, 1},
        {0, 0, -1},
    };

    float distance = -1;
    for (Vec3 n : normals)
    {
        float newDistance = DotProduct(normalized, n);
        if (newDistance > distance)
        {
            distance = newDistance;
            normal = n;
        }
    }

    return true;
}

bool SphereVsTriangle(const Vec3& center, const float& radius, const Triangle& triangle, Vec3& directionToTriangle, float& distance, bool checkDistanceToTriangle)
{
    // plane normal
    Vec3 N = Normalize(CrossProduct(triangle.p1.p - triangle.p0.p, triangle.p2.p - triangle.p0.p));

    // signed distance between sphere and plane
    float dist = DotProduct(center - triangle.p0.p, N);

    // can pass through back side of triangle (optional)
    //bool isDoubleSided = false;
    //if (!isDoubleSided && dist > 0)
    //{
    //    return false;
    //}
    // no intersection if 
    if (checkDistanceToTriangle && (dist < -radius || dist > radius))
    {
        return false;
    }

    Vec3 point0 = center - N * dist; // projected sphere center on triangle plane

    // Now determine whether point0 is inside all triangle edges: 
    Vec3 c0 = CrossProduct(point0 - triangle.p0.p, triangle.p1.p - triangle.p0.p);
    Vec3 c1 = CrossProduct(point0 - triangle.p1.p, triangle.p2.p - triangle.p1.p);
    Vec3 c2 = CrossProduct(point0 - triangle.p2.p, triangle.p0.p - triangle.p2.p);

    bool inside = DotProduct(c0, N) <= 0 && DotProduct(c1, N) <= 0 && DotProduct(c2, N) <= 0;
    bool intersects = false;
    Vec3 point1 = {};
    Vec3 point2 = {};
    Vec3 point3 = {};
    if (!inside)
    { 
        float radiussq = radius * radius; // sphere radius squared

        // Edge 1:
        Vec3 point1 = ClosestPointOnLineSegment(triangle.p0.p, triangle.p1.p, center);
        Vec3 v1 = center - point1;
        float distsq1 = DotProduct(v1, v1);
        intersects = distsq1 < radiussq;

        // Edge 2:
        Vec3 point2 = ClosestPointOnLineSegment(triangle.p1.p, triangle.p2.p, center);
        Vec3 v2 = center - point2;
        float distsq2 = DotProduct(v2, v2);
        intersects |= distsq2 < radiussq;

        // Edge 3:
        Vec3 point3 = ClosestPointOnLineSegment(triangle.p2.p, triangle.p0.p, center);
        Vec3 v3 = center - point3;
        float distsq3 = DotProduct(v3, v3);
        intersects |= distsq3 < radiussq;
    }

    if (inside || intersects)
    {
        Vec3 best_point = point0;
        Vec3 intersection_vec;

        if (inside)
        {
            intersection_vec = center - point0;
        }
        else
        {
            Vec3 d = center - point1;
            float best_distsq = DotProduct(d, d);
            best_point = point1;
            intersection_vec = d;

            d = center - point2;
            float distsq = DotProduct(d, d);
            if (distsq < best_distsq)
            {
                distsq = best_distsq;
                best_point = point2;
                intersection_vec = d;
            }

            d = center - point3;
            distsq = DotProduct(d, d);
            if (distsq < best_distsq)
            {
                distsq = best_distsq;
                best_point = point3;
                intersection_vec = d;
            }
        }

        float len = Length(intersection_vec);  // vector3 length calculation: 
        if (len)
        { 
            //Vec3 penetration_vec = sqrtf(DotProduct(intersection_vec, intersection_vec));
            directionToTriangle = intersection_vec / len;  // normalize
            if (isnan(directionToTriangle.x))
                int32 i = 10;//issues;
            directionToTriangle = -directionToTriangle;
            distance = radius - len; // radius = sphere radius
        }
        else
        {
            directionToTriangle = {};
            distance = {};
        }
        return true; // intersection success
    }
    else
        return false;
}


bool SphereVsBlock(const Vec3& center, const float& radius, const GamePos& blockP)
{
    // plane normal
    Vec3 N = Normalize(CrossProduct(triangle.p1.p - triangle.p0.p, triangle.p2.p - triangle.p0.p));

    // signed distance between sphere and plane
    float dist = DotProduct(center - triangle.p0.p, N);

    // can pass through back side of triangle (optional)
    //bool isDoubleSided = false;
    //if (!isDoubleSided && dist > 0)
    //{
    //    return false;
    //}
    // no intersection if 
    if (checkDistanceToTriangle && (dist < -radius || dist > radius))
    {
        return false;
    }

    Vec3 point0 = center - N * dist; // projected sphere center on triangle plane

    // Now determine whether point0 is inside all triangle edges: 
    Vec3 c0 = CrossProduct(point0 - triangle.p0.p, triangle.p1.p - triangle.p0.p);
    Vec3 c1 = CrossProduct(point0 - triangle.p1.p, triangle.p2.p - triangle.p1.p);
    Vec3 c2 = CrossProduct(point0 - triangle.p2.p, triangle.p0.p - triangle.p2.p);

    bool inside = DotProduct(c0, N) <= 0 && DotProduct(c1, N) <= 0 && DotProduct(c2, N) <= 0;
    bool intersects = false;
    Vec3 point1 = {};
    Vec3 point2 = {};
    Vec3 point3 = {};
    if (!inside)
    { 
        float radiussq = radius * radius; // sphere radius squared

        // Edge 1:
        Vec3 point1 = ClosestPointOnLineSegment(triangle.p0.p, triangle.p1.p, center);
        Vec3 v1 = center - point1;
        float distsq1 = DotProduct(v1, v1);
        intersects = distsq1 < radiussq;

        // Edge 2:
        Vec3 point2 = ClosestPointOnLineSegment(triangle.p1.p, triangle.p2.p, center);
        Vec3 v2 = center - point2;
        float distsq2 = DotProduct(v2, v2);
        intersects |= distsq2 < radiussq;

        // Edge 3:
        Vec3 point3 = ClosestPointOnLineSegment(triangle.p2.p, triangle.p0.p, center);
        Vec3 v3 = center - point3;
        float distsq3 = DotProduct(v3, v3);
        intersects |= distsq3 < radiussq;
    }

    if (inside || intersects)
    {
        Vec3 best_point = point0;
        Vec3 intersection_vec;

        if (inside)
        {
            intersection_vec = center - point0;
        }
        else
        {
            Vec3 d = center - point1;
            float best_distsq = DotProduct(d, d);
            best_point = point1;
            intersection_vec = d;

            d = center - point2;
            float distsq = DotProduct(d, d);
            if (distsq < best_distsq)
            {
                distsq = best_distsq;
                best_point = point2;
                intersection_vec = d;
            }

            d = center - point3;
            distsq = DotProduct(d, d);
            if (distsq < best_distsq)
            {
                distsq = best_distsq;
                best_point = point3;
                intersection_vec = d;
            }
        }

        float len = Length(intersection_vec);  // vector3 length calculation: 
        if (len)
        { 
            //Vec3 penetration_vec = sqrtf(DotProduct(intersection_vec, intersection_vec));
            directionToTriangle = intersection_vec / len;  // normalize
            if (isnan(directionToTriangle.x))
                int32 i = 10;//issues;
            directionToTriangle = -directionToTriangle;
            distance = radius - len; // radius = sphere radius
        }
        else
        {
            directionToTriangle = {};
            distance = {};
        }
        return true; // intersection success
    }
    else
        return false;
}



Vec3 ClosestPointOnLineSegment(const Vec3& A, const Vec3& B, const Vec3& Point)
{
    Vec3 AB = B - A;
    float t = DotProduct(Point - A, AB) / DotProduct(AB, AB);
    //Saturate func: Min(Max(t, 0.0f), 1.0f)
    return A + Min(Max(t, 0.0f), 1.0f) * AB;
}

struct VertexFace {
    
    Vec3 e[4];
};

const VertexFace cubeVertices[6] = {
    // +x
    VertexFace( { 
    Vec3(1.0,  1.0,  1.0),
    Vec3(1.0,    0,  1.0),
    Vec3(1.0,  1.0,    0),
    Vec3(1.0,    0,    0) 
    }),

    // -x
    VertexFace({
    Vec3(0,  1.0,    0),
    Vec3(0,    0,    0),
    Vec3(0,  1.0,  1.0),
    Vec3(0,    0,  1.0)
    }),

    // +y
    VertexFace({
    Vec3( 1.0,  1.0,  1.0 ),
    Vec3( 1.0,  1.0,    0 ),
    Vec3(   0,  1.0,  1.0 ), 
    Vec3(   0,  1.0,    0 )
    }),

    // -y
    VertexFace({
    Vec3(   0,    0,  1.0 ), 
    Vec3(   0,    0,    0 ),
    Vec3( 1.0,    0,  1.0 ),
    Vec3( 1.0,    0,    0 )
    }),

    // z
    VertexFace({
    Vec3(   0,  1.0,  1.0 ), 
    Vec3(   0,    0,  1.0 ),
    Vec3( 1.0,  1.0,  1.0 ),
    Vec3( 1.0,    0,  1.0 )
    }),

    // -z
    VertexFace({
    Vec3( 1.0,  1.0,    0 ),
    Vec3( 1.0,    0,    0 ),
    Vec3(   0,  1.0,    0 ), 
    Vec3(   0,    0,    0 )
    })
};

bool CapsuleVsBlock(Capsule collider, GamePos blockGP, Vec3& toOutside, std::vector<Triangle>& debug_triangles)
{
    Triangle triangles[+Face::Count][2] = {};
    bool behindFaces[+Face::Count] = {};

    BlockType blockType;
    ChunkPos blockChunkP = {};
    Vec3Int block_blockP = Convert_GameToBlock(blockChunkP, blockGP);
    ChunkIndex blockChunkIndex;
    if (g_chunks->GetChunkFromPosition(blockChunkIndex, blockChunkP))
    {
        //checking if there is a block at the target location
        blockType = g_chunks->blocks[blockChunkIndex].e[block_blockP.x][block_blockP.y][block_blockP.z];
        if (blockType == BlockType::Empty)
            return false;

        //variables used
        WorldPos blockP = ToWorld(blockGP);
        Vec3 topBound = { collider.m_tip.p.x,  collider.m_tip.p.y  - collider.m_radius, collider.m_tip.p.z };
        Vec3 botBound = { collider.m_tail.p.x, collider.m_tail.p.y + collider.m_radius, collider.m_tail.p.z };
        Vec3 sphereCenter = {};
        sphereCenter.x = Clamp(float(blockGP.p.x) + 0.5f, botBound.x, topBound.x);
        sphereCenter.y = Clamp(float(blockGP.p.y) + 0.5f, botBound.y, topBound.y);
        sphereCenter.z = Clamp(float(blockGP.p.z) + 0.5f, botBound.z, topBound.z);

        //Creating vertices for each triangle
        for (int32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
        {
            triangles[faceIndex][0] = {};
            triangles[faceIndex][0].p0 = blockP.p + cubeVertices[faceIndex].e[0];
            triangles[faceIndex][0].p1 = blockP.p + cubeVertices[faceIndex].e[1];
            triangles[faceIndex][0].p2 = blockP.p + cubeVertices[faceIndex].e[2];
            debug_triangles.push_back(triangles[faceIndex][0]);

            triangles[faceIndex][1] = {};
            triangles[faceIndex][1].p0 = blockP.p + cubeVertices[faceIndex].e[1];
            triangles[faceIndex][1].p1 = blockP.p + cubeVertices[faceIndex].e[3];
            triangles[faceIndex][1].p2 = blockP.p + cubeVertices[faceIndex].e[2];
            debug_triangles.push_back(triangles[faceIndex][1]);
        }


        //Check sphere against every triangle
        for (int32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
        {
            for (int32 i = 0; i < 2; i++)
            {
                const Triangle& triangle = triangles[faceIndex][0];
                //Vec3 triangleCenter = triangle.Center();

                Vec3 directionToTriangle = {};
                float distanceToTriangle = {};
                behindFaces[faceIndex] = (behindFaces[faceIndex] || (SphereVsTriangle(sphereCenter, collider.m_radius, triangle, directionToTriangle, distanceToTriangle, false)/* && distanceToTriangle > 0.0f*/));
            }
        }

        //Check if we are inside the block
        for (int32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
            if (!behindFaces[faceIndex])
                return false;
    
        //Find vector to outside the block
#if 1
        float distances[+Face::Count] = {};
        WorldPos blockCenter = blockP.p + 0.5f;
        toOutside = sphereCenter - blockCenter.p;
        toOutside = Normalize(toOutside);
        
#else
        float distances[+Face::Count] = {};
        WorldPos blockCenter = blockP.p + 0.5f;
        
        for (int32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
        {
            Vec3 face = blockCenter + (faceNormals[faceIndex] / 2);
            distances[faceIndex] = Distance(sphereCenter, face);
        }
#endif

        return true;
    }
    return false;
}


//#include <iostream>
//#include <stdio.h>
//#include <string_view>
//#include <string>
//#include <cassert>


int ComparisonFunction(const void* a, const void* b)
{
    return  *(int32*)b - *(int32*)a;
}

void Swap(void* a, void* b, const int32 size)
{
    
    uint8* c = (uint8*)a;
    uint8* d = (uint8*)b;
    for (int32 i = 0; i < size; i++)
    {
        
        uint8 temp = c[i];
        c[i] = d[i];
        d[i] = temp;
    }
}

int Partition(uint8* array, const int32 itemSize, int32 iBegin, int32 iEnd, int32 (*compare)(const void*, const void*))
{
    assert(array != nullptr);
    uint8* pivot = &array[iEnd * itemSize];
    assert(pivot != nullptr);
    int32 lowOffset = iBegin;

    for (int32 i = iBegin; i < iEnd; i++)
    {
        if (compare(&array[i * itemSize], pivot) > 0)
        {
            Swap(&array[lowOffset * itemSize], &array[i * itemSize], itemSize);
            lowOffset++;
        }
    }

    Swap(&array[lowOffset * itemSize], &array[iEnd * itemSize], itemSize);
    return lowOffset;
}


void QuickSortInternal(uint8* array, const int32 itemSize, int32 iBegin, int32 iEnd, int32 (*compare)(const void*, const void*))
{
    if (iBegin < iEnd)
    {
        int32 pivotIndex = Partition(array, itemSize, iBegin, iEnd, compare);
        QuickSortInternal(array, itemSize, iBegin, pivotIndex - 1, compare); //Low Sort
        QuickSortInternal(array, itemSize, pivotIndex + 1, iEnd, compare); //High Sort
    }
}

void QuickSort(uint8* data, const int32 length, const int32 itemSize, int32 (*compare)(const void* a, const void* b))
{
    QuickSortInternal(data, itemSize, 0, length - 1, compare);
}
