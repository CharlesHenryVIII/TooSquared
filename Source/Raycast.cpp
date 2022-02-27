#include "Raycast.h"
#include "Block.h"
#include "Chunk.h"
#include "Collision.h"

//http://www.cs.yorku.ca/~amana/research/grid.pdf
RaycastResult LineCast(const Ray& ray, float length)
{
    RaycastResult result = {};

    Vec3 p = Vec3IntToVec3(ToGame((WorldPos(ray.origin))).p);
    Vec3 step = {};
    step.x = ray.direction.x >= 0 ? 1.0f : -1.0f;
    step.y = ray.direction.y >= 0 ? 1.0f : -1.0f;
    step.z = ray.direction.z >= 0 ? 1.0f : -1.0f;
    Vec3 pClose = Vec3IntToVec3(ToGame((WorldPos(Round(ray.origin + (step / 2))))).p);
    Vec3 tMax = Abs((pClose - ray.origin) / ray.direction);
    Vec3 tDelta = Abs(1.0f / ray.direction);
    
    BlockType blockType = BlockType::Empty;
    while (blockType == BlockType::Empty) {
        if (Distance(p, ray.origin) > length)
            break;

        result.normal = {};
        if (tMax.x < tMax.y && tMax.x < tMax.z)
        {
            p.x += step.x;
            tMax.x += tDelta.x;
            result.normal.x = float(-1.0) * step.x;
        }
        else if (tMax.y < tMax.x && tMax.y < tMax.z)
        {
            p.y += step.y;
            tMax.y += tDelta.y;
            result.normal.y = float(-1.0) * step.y;
        }
        else 
        {
            p.z += step.z;
            tMax.z += tDelta.z;
            result.normal.z = float(-1.0) * step.z;
        }

        result.p.p = Vec3ToVec3Int(p);
        g_chunks->GetBlock(blockType, result.p);
    }
    result.distance = Distance(ray.origin, p);
    result.success = blockType != BlockType::Empty;
    return result;
}

RaycastResult RayVsChunk(const Ray& ray, float length)
{
    RaycastResult result = LineCast(ray, length);
    return result;
}

bool RayVsAABB(const Ray& ray, const AABB& box, float& min, Vec3& intersect, Vec3& normal, Vec3& actualMove)
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

    const static Vec3 normals[] = {
        {  1,  0,  0 },
        { -1,  0,  0 },
        {  0,  1,  0 },
        {  0, -1,  0 },
        {  0,  0,  1 },
        {  0,  0, -1 },
    };

#if 1
    Vec3  v[6];
    v[0] = { box.max.x,   intersect.y, intersect.z };
    v[1] = { box.min.x,   intersect.y, intersect.z };
    v[2] = { intersect.x, box.max.y,   intersect.z };
    v[3] = { intersect.x, box.min.y,   intersect.z };
    v[4] = { intersect.x, intersect.y, box.max.z   };
    v[5] = { intersect.x, intersect.y, box.min.z   };

    float currentDistance = 0;
    float ClosestDistance = FLT_MAX;
    int32 closestFace = 0;
    for (int32 i = 0; i < arrsize(v); i++)
    {
        currentDistance = Distance(intersect, v[i]);
        if (currentDistance < ClosestDistance)
        {
            ClosestDistance = currentDistance;
            closestFace = i;
        }
    }
    normal = normals[closestFace];

    //one of these will be zero maybe I just add instead of branching?
    if (min)
        actualMove = NormalizeZero(intersect - ray.origin) * min;
    else
        actualMove = v[closestFace] - intersect;
#else
    Vec3 center = (box.max + box.min) / 2.0f;
    Vec3 toNormal = intersect - center;
    Vec3 normalized = Normalize(toNormal);
    float distance = -1;
    int32 index = 0;
    for (Vec3 n : normals)
    {
        float newDistance = ::fabs(DotProduct(normalized, n));
        if (newDistance > distance)
        {
            distance = newDistance;
            normal = n;
        }
        index++;
    }
#endif

    return true;
}

bool RayVsAABB(const Ray& ray, const AABB& box)
{
    float min;
    Vec3 intersect;
    Vec3 normal;
    Vec3 direction;
    return RayVsAABB(ray, box, min, intersect, normal, direction);
}
