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

bool RayVsAABB(const Ray& ray, const AABB& box, float& min, Vec3& intersect, Vec3& normal, uint8& face)
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
    int32 index = 0;
    for (Vec3 n : normals)
    {
        float newDistance = DotProduct(normalized, n);
        if (newDistance > distance)
        {
            distance = newDistance;
            normal = n;
            face = index;
        }
        index++;
    }

    return true;
}

bool RayVsAABB(const Ray& ray, const AABB& box)
{
    float min;
    Vec3 intersect;
    Vec3 normal;
    uint8 face;
    return RayVsAABB(ray, box, min, intersect, normal, face);
}
