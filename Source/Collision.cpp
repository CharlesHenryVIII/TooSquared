#include "Collision.h"
#include "Chunk.h"
#include "Raycast.h"

bool PointVsAABB(const Vec3& point, const AABB& box)
{
  return ((point.x >= box.min.x && point.x <= box.max.x) &&
          (point.y >= box.min.y && point.y <= box.max.y) &&
          (point.z >= box.min.z && point.z <= box.max.z));
}


bool AABBVsAABB(const AABB& box1, const AABB& box2)
{
    bool result = ((box1.min.x <= box2.max.x && box1.max.x >= box2.min.x) &&
                   (box1.min.y <= box2.max.y && box1.max.y >= box2.min.y) &&
                   (box1.min.z <= box2.max.z && box1.max.z >= box2.min.z));
    return result;
}

//https://noonat.github.io/intersect/
bool AABBVsAABB(Vec3& out_intersection, const AABB& box1, const AABB& box2)
{
    out_intersection = {};

    const Vec3 box1Center  = box1.Center();
    const Vec3 box2Center  = box2.Center();
    const Vec3 box1Lengths = box1.GetLengths();
    const Vec3 box2Lengths = box2.GetLengths();

    const float dx = box1Center.x - box2Center.x;
    const float px = (box1Lengths.x / 2 + box2Lengths.x / 2) - abs(dx);

    const float dy = box1Center.y - box2Center.y;
    const float py = (box1Lengths.y / 2 + box2Lengths.y / 2) - abs(dy);

    const float dz = box1Center.z - box2Center.z;
    const float pz = (box1Lengths.z / 2 + box2Lengths.z / 2) - abs(dz);
#if 0
    result = AABBVsAABB(box1, box2);

#else
    if (px <= 0)
        return false;
    if (py <= 0)
        return false;
    if (pz <= 0)
        return false;
#endif


    if (py < px && py < pz)
    {
        const float sy = Sign(dy);
        out_intersection.y = py * sy;
    }
    else if (px < py && px < pz)
    {
        const float sx = Sign(dx);
        out_intersection.x = px * sx;
    }
    else if (pz < px && pz < py)
    {
        const float sz = Sign(dz);
        out_intersection.z = pz * sz;
    }
    return true;
}

//https://wickedengine.net/2020/04/26/capsule-collision-detection/
bool SphereVsTriangle(const Vec3& center, const float radius, const Triangle& triangle, Vec3& directionToTriangle, float& distance)
{
    // plane normal
    Vec3 N = Normalize(CrossProduct(triangle.p1.p - triangle.p0.p, triangle.p2.p - triangle.p0.p));

    // signed distance between sphere and plane
    float dist = DotProduct(center - triangle.p0.p, N);

    // can pass through back side of triangle (optional)
    bool isDoubleSided = false;
    if (!isDoubleSided && dist < 0)
    {
        return false;
    }
    // no intersection if
    if ((dist < -radius || dist > radius))
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
        point1 = ClosestPointOnLineSegment(triangle.p0.p, triangle.p1.p, center);
        Vec3 v1 = center - point1;
        float distsq1 = DotProduct(v1, v1);
        intersects = distsq1 < radiussq;

        // Edge 2:
        point2 = ClosestPointOnLineSegment(triangle.p1.p, triangle.p2.p, center);
        Vec3 v2 = center - point2;
        float distsq2 = DotProduct(v2, v2);
        intersects |= distsq2 < radiussq;

        // Edge 3:
        point3 = ClosestPointOnLineSegment(triangle.p2.p, triangle.p0.p, center);
        Vec3 v3 = center - point3;
        float distsq3 = DotProduct(v3, v3);
        intersects |= distsq3 < radiussq;
    }

    if (inside || intersects)
    {
        Vec3 bestPoint = point0;
        Vec3 intersectionVector;

        if (inside)
        {
            intersectionVector = center - point0;
        }
        else
        {
#include "Raycast.h"
            Vec3 d = center - point1;
            float bestDistanceSQ = DotProduct(d, d);
            bestPoint = point1;
            intersectionVector = d;

            d = center - point2;
            float distsq = DotProduct(d, d);
            if (distsq < bestDistanceSQ)
            {
                distsq = bestDistanceSQ;
                bestPoint = point2;
                intersectionVector = d;
            }

            d = center - point3;
            distsq = DotProduct(d, d);
            if (distsq < bestDistanceSQ)
            {
                distsq = bestDistanceSQ;
                bestPoint = point3;
                intersectionVector = d;
            }
        }

        float len = Length(intersectionVector);  // vector3 length calculation:
        if (radius <= len)
        {
            return false;
        }
        else if (len)
        {
            directionToTriangle = intersectionVector / len;  // normalize
            assert(!isnan(directionToTriangle.x));
            assert(!isnan(directionToTriangle.y));
            assert(!isnan(directionToTriangle.z));
            distance = radius - len;
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

bool SphereVsBlock(const Vec3& center, const float radius, const GamePos& _blockP, Vec3& toOutside, std::vector<Triangle>& debug_triangles)
{
    WorldPos blockP = ToWorld(_blockP);

    //Creating vertices for each triangle
    for (int32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
    {
        Triangle triangle = {};
        triangle.p0 = blockP.p + cubeVertices[faceIndex].e[0];
        triangle.p1 = blockP.p + cubeVertices[faceIndex].e[1];
        triangle.p2 = blockP.p + cubeVertices[faceIndex].e[2];
        debug_triangles.push_back(triangle);


        // plane normal
        Vec3 N = triangle.Normal();

        // signed distance between sphere and plane
        float dist = DotProduct(center - triangle.p0.p, N);
        if (dist > radius)
            return false;

    }

    WorldPos blockCenter = blockP.p + 0.5f;
    toOutside = center - blockCenter.p;
    toOutside = (Normalize(toOutside) / 2.0f) + radius;

    return true;
}

Vec3 ClosestPointOnLineSegment(const Vec3& A, const Vec3& B, const Vec3& Point)
{
    Vec3 AB = B - A;
    float t = DotProduct(Point - A, AB) / DotProduct(AB, AB);
    //Saturate func: Min(Max(t, 0.0f), 1.0f)
    return A + Min(Max(t, 0.0f), 1.0f) * AB;
}

Vec3 LinePlaneIntersectionVsTriangle(const Vec3& linePlaneIntersection, const Triangle& triangle)
{
    // Determine whether line_plane_intersection is inside all triangle edges:
    Vec3 triangleNormal = triangle.Normal();
    bool inside = false;

#if 1

    Vec3 max = {};
    max.x = Max(triangle.p0.p.x, Max(triangle.p1.p.x, triangle.p2.p.x));
    max.y = Max(triangle.p0.p.y, Max(triangle.p1.p.y, triangle.p2.p.y));
    max.z = Max(triangle.p0.p.z, Max(triangle.p1.p.z, triangle.p2.p.z));
    Vec3 min = {inf, inf, inf};
    min.x = Min(triangle.p0.p.x, Min(triangle.p1.p.x, triangle.p2.p.x));
    min.y = Min(triangle.p0.p.y, Min(triangle.p1.p.y, triangle.p2.p.y));
    min.z = Min(triangle.p0.p.z, Min(triangle.p1.p.z, triangle.p2.p.z));
    inside = (linePlaneIntersection.x > min.x && linePlaneIntersection.x < max.x) &&
            (linePlaneIntersection.y > min.y && linePlaneIntersection.y < max.y) &&
            (linePlaneIntersection.z > min.z && linePlaneIntersection.z < max.z);
#else

    Vec3 c0 = CrossProduct(linePlaneIntersection - triangle.p0.p, triangle.p1.p - triangle.p0.p);
    Vec3 c1 = CrossProduct(linePlaneIntersection - triangle.p1.p, triangle.p2.p - triangle.p1.p);
    Vec3 c2 = CrossProduct(linePlaneIntersection - triangle.p2.p, triangle.p0.p - triangle.p2.p);
    inside = DotProduct(c0, triangleNormal) <= 0 && DotProduct(c1, triangleNormal) <= 0 && DotProduct(c2, triangleNormal) <= 0;
#endif
    Vec3 resultReferencePoint = {};

    if (inside)
    {
        resultReferencePoint = linePlaneIntersection;
    }
    else
    {
        // Edge 1:
        Vec3 point1 = ClosestPointOnLineSegment(triangle.p0.p, triangle.p1.p, linePlaneIntersection);
        Vec3 v1 = linePlaneIntersection - point1;
        float distsq = DotProduct(v1, v1);
        float bestDistance = distsq;
        resultReferencePoint = point1;

        // Edge 2:
        Vec3 point2 = ClosestPointOnLineSegment(triangle.p1.p, triangle.p2.p, linePlaneIntersection);
        Vec3 v2 = linePlaneIntersection - point2;
        distsq = DotProduct(v2, v2);
        if (distsq < bestDistance)
        {
            resultReferencePoint = point2;
            bestDistance = distsq;
        }

        // Edge 3:
        Vec3 point3 = ClosestPointOnLineSegment(triangle.p2.p, triangle.p0.p, linePlaneIntersection);
        Vec3 v3 = linePlaneIntersection - point3;
        distsq = DotProduct(v3, v3);
        if (distsq < bestDistance)
        {
            resultReferencePoint = point3;
            bestDistance = distsq;
        }
    }
    return resultReferencePoint;
}

bool CapsuleVsTriangle(const Capsule& collider, const Triangle& triangle, Vec3& directionToTriangle, float& distanceToTriangle, bool checkDistanceToTriangle)
{
    // Compute capsule line endpoints A, B like before in capsule-capsule case:
    Vec3 capsuleNormal = Normalize(collider.m_tip.p - collider.m_tail.p);
    Vec3 lineEndOffset = capsuleNormal * collider.m_radius;
    Vec3 A = collider.m_tail.p + lineEndOffset;
    Vec3 B = collider.m_tip.p - lineEndOffset;

    // Then for each triangle, ray-plane intersection:
    Vec3 triangleNormal = triangle.Normal();
    float t = DotProduct(triangleNormal, (triangle.p0.p - collider.m_tail.p) / abs(DotProduct(triangleNormal, capsuleNormal)));
    Vec3 linePlaneIntersection = collider.m_tail.p + capsuleNormal * t;

    if (DotProduct(triangleNormal, capsuleNormal) == 0)
        linePlaneIntersection = triangle.p1.p;

    Vec3 referencePoint = LinePlaneIntersectionVsTriangle(linePlaneIntersection, triangle);

    // The center of the best sphere candidate:
    Vec3 sphereCenter = ClosestPointOnLineSegment(A, B, referencePoint);

    if (SphereVsTriangle(sphereCenter, collider.m_radius, triangle, directionToTriangle, distanceToTriangle))// && distanceToTriangle > 0.0f)
    {
        //Need to capture if the distance is zero then we are inside the triangle;

        assert(DotProduct(triangleNormal, { directionToTriangle.x, 0.0f, 0.0f }) >= 0.0f);
        assert(DotProduct(triangleNormal, { 0.0f, directionToTriangle.y, 0.0f }) >= 0.0f);
        assert(DotProduct(triangleNormal, { 0.0f, 0.0f, directionToTriangle.z }) >= 0.0f);

        if ((Distance(sphereCenter, collider.m_tail.p) > collider.m_radius) && (Distance(sphereCenter, collider.m_tip.p) > collider.m_radius) &&
            distanceToTriangle == 0.0f && (triangleNormal.x == 0 && triangleNormal.z == 0) && checkDistanceToTriangle)
        {
            Vec3 capsulePoint = {};
            float dotResult = DotProduct(triangleNormal, capsuleNormal);
            if (dotResult)
            {
                if (dotResult > 0.0f)
                    capsulePoint = collider.m_tail.p;
                else// if (dotResult < 0.0f)
                    capsulePoint = collider.m_tip.p;

                directionToTriangle = Normalize(sphereCenter - capsulePoint);
                distanceToTriangle  = Distance(sphereCenter, capsulePoint) + collider.m_radius;
            }
        }
        return true;
    }
    return false;
}


bool CapsuleVsBlock(Capsule collider, const BlockSampler& blockSampler, Vec3& toOutside, std::vector<Triangle>& debug_triangles)
{
    bool result = false;

    WorldPos blockP = ToWorld(blockSampler.m_baseBlockP);
    Vec3 directionToTriangle = {};
    float distanceToTriangle = {};
    int32 cubeIndices[] = { 0, 1, 2, 1, 3, 2 };
    int32 faceIndices[] = { +Face::Right, +Face::Left, +Face::Back, +Face::Front, +Face::Top, +Face::Bot };
    Triangle triangle = {};


    for (int32 faceIndex : faceIndices)
    {
        Block& faceBlock = g_blocks[+blockSampler.blocks[faceIndex]];
        Block& baseBlock = g_blocks[+blockSampler.m_baseBlockType];
        if (faceBlock.m_flags & BLOCK_COLLIDABLE)
            continue;
        if (baseBlock.m_flags & BLOCK_COMPLEX)
            int32 asdf = 213124;
        for (int32 j = 0; j <= 1; j++)
        {
            triangle = {};
            for (int32 k = 0; k < 3; k++)
            {
#if 1
#if 1
                triangle.e[k].p.x = blockP.p.x + ((1 - baseBlock.m_size.x) / 2) + (cubeVertices[faceIndex].e[cubeIndices[k + (j * 3)]].x * baseBlock.m_size.x);
                triangle.e[k].p.y = blockP.p.y +                                  (cubeVertices[faceIndex].e[cubeIndices[k + (j * 3)]].y * baseBlock.m_size.y);
                triangle.e[k].p.z = blockP.p.z + ((1 - baseBlock.m_size.z) / 2) + (cubeVertices[faceIndex].e[cubeIndices[k + (j * 3)]].z * baseBlock.m_size.z);
#else
                triangle.e[k].p.x = blockP.p.x + ((1 - faceBlock.m_size.x) / 2) + (cubeVertices[faceIndex].e[cubeIndices[k + (j * 3)]].x * faceBlock.m_size.x);
                triangle.e[k].p.y = blockP.p.y +                                  (cubeVertices[faceIndex].e[cubeIndices[k + (j * 3)]].y * faceBlock.m_size.y);
                triangle.e[k].p.z = blockP.p.z + ((1 - faceBlock.m_size.z) / 2) + (cubeVertices[faceIndex].e[cubeIndices[k + (j * 3)]].z * faceBlock.m_size.z);
#endif
#else
                triangle.e[k] = blockP.p + (cubeVertices[faceIndex].e[cubeIndices[k + (j * 3)]];
#endif
            }

            directionToTriangle = {};
            distanceToTriangle = {};
            if (CapsuleVsTriangle(collider, triangle, directionToTriangle, distanceToTriangle, true))// && distanceToTriangle > 0.0f)
            {//Sphere inside triangle
                Vec3 offset = {};
                int32 dimension = faceIndex / 2;

#if 1
                if (fabs(directionToTriangle.e[dimension] * distanceToTriangle) > fabsf(toOutside.e[dimension]))
                    offset.e[dimension] = directionToTriangle.e[dimension] * distanceToTriangle;
#else
                offset.e[dimension] = directionToTriangle.e[dimension] * distanceToTriangle;
#endif
                toOutside += offset;
                //collider.UpdateTipLocation(collider.m_tip.p + offset);
                debug_triangles.push_back(triangle);
                result = true;
            }
        }
    }

    collider.UpdateMidTipLocation(collider.m_tip.p + Vec3({ 0.0f, directionToTriangle.y * distanceToTriangle, 0.0f }));

    return result;
}

bool CapsuleVsWorldBlocks(Capsule capsuleCollider, Vec3 in_positionDelta, Vec3& out_positionDelta, std::vector<Triangle>& debug_trianglesToDraw)
{
    assert(OnMainThread());
    bool result = false;

    while (in_positionDelta.x != 0.0f || in_positionDelta.y != 0.0f || in_positionDelta.z != 0.0f)
    {

        {
            Vec3 pDelta = {};
            float clampVal = 0.3f;
            pDelta.x = Clamp(in_positionDelta.x, -clampVal, clampVal);
            pDelta.y = Clamp(in_positionDelta.y, -clampVal, clampVal);
            pDelta.z = Clamp(in_positionDelta.z, -clampVal, clampVal);

            //g_camera.transform.m_p.p += pDelta;
            in_positionDelta -= pDelta;
            capsuleCollider.UpdateLocation(pDelta);
        }


        //ZoneScopedN("Collision Update");
        BlockSampler blockSampler = {};

        GamePos referenceGamePosition = ToGame(capsuleCollider.m_tip);
        int32 horizontalOffset = Max(int32(capsuleCollider.m_radius + 0.5f), 1);
        int32 verticalOffset = Max(int32(capsuleCollider.m_tip.p.y - capsuleCollider.m_tail.p.y + 0.5f), 1);
        //g_camera.transform.m_isGrounded = false;

        for (int32 y = -verticalOffset; y <= verticalOffset; y++)
        {
            for (int32 x = -horizontalOffset; x <= horizontalOffset; x++)
            {
                for (int32 z = -horizontalOffset; z <= horizontalOffset; z++)
                {
                    if (y + referenceGamePosition.p.y >= CHUNK_Y || y + referenceGamePosition.p.y < 0)
                        continue;
                    if (!(blockSampler.RegionGather(GamePos(referenceGamePosition.p + Vec3Int({ x, y, z })))))
                        continue;
                    if (!(g_blocks[+blockSampler.m_baseBlockType].m_flags & BLOCK_COLLIDABLE))
                        continue;
                    Vec3 outsideOfBlock = {};
                    if (CapsuleVsBlock(capsuleCollider, blockSampler, outsideOfBlock, debug_trianglesToDraw))
                    {
                        out_positionDelta += outsideOfBlock;
                        capsuleCollider.UpdateLocation(outsideOfBlock);
                        result = true;
                    }
                }
            }
        }
    }
    return result;
}

bool CubeVsBlock(Cube collider, const BlockSampler& blockSampler, Vec3& toOutside, std::vector<Triangle>& debug_triangles)
{
    AABB block;
    block.min = ToWorld(blockSampler.m_baseBlockP).p;
    block.max = block.min + Vec3({ 1.0f, 1.0f, 1.0f });

    AABB cube;
    cube.min = collider.m_center.p - Vec3({ collider.m_length / 2.0f, collider.m_length / 2.0f, collider.m_length / 2.0f });
    cube.max = cube.min + Vec3({ collider.m_length, collider.m_length, collider.m_length });

    return AABBVsAABB(toOutside, cube, block);
}

bool CubeVsWorldBlocks(Cube collider, Vec3 in_positionDelta, Vec3& out_positionDelta, std::vector<Triangle>& debug_trianglesToDraw)
{
    //ZoneScopedN("CubeVsWorldBlocks()");
    assert(OnMainThread());
    bool result = false;
    WorldPos boxOrigin;
    boxOrigin.p = collider.m_center.p;
    Vec3 deltaPOrigin = in_positionDelta;
    Vec3 normalMap = {};

    while (in_positionDelta.x != 0.0f || in_positionDelta.y != 0.0f || in_positionDelta.z != 0.0f)
    {
        //at 100 items this took 8.27ms
        Vec3 pDelta = {};
        float clampVal = 0.3f;
        pDelta.x = Clamp(in_positionDelta.x, -clampVal, clampVal);
        pDelta.y = Clamp(in_positionDelta.y, -clampVal, clampVal);
        pDelta.z = Clamp(in_positionDelta.z, -clampVal, clampVal);

        in_positionDelta -= pDelta;

        Vec3 halfLengths = { collider.m_length / 2.0f, collider.m_length / 2.0f, collider.m_length / 2.0f };
        GamePos blockp = {};

        WorldPos minBoundsCheckWorld = collider.m_center.p - halfLengths;
        WorldPos maxBoundsCheckWorld = collider.m_center.p + halfLengths;
        GamePos minBoundsCheck = ToGame(minBoundsCheckWorld);
        GamePos maxBoundsCheck = ToGame(maxBoundsCheckWorld);
        for (int32 y = minBoundsCheck.p.y; y <= maxBoundsCheck.p.y; y++)
        {
            for (int32 x = minBoundsCheck.p.x; x <= maxBoundsCheck.p.x; x++)
            {
                for (int32 z = minBoundsCheck.p.z; z <= maxBoundsCheck.p.z; z++)
                {
                    blockp.p = { x, y, z };



                    BlockType blockCheck;
                    g_chunks->GetBlock(blockCheck, blockp);
                    if (!(g_blocks[+blockCheck].m_flags & BLOCK_COLLIDABLE))
                        continue;
                    if (g_blocks[+blockCheck].m_flags & BLOCK_COMPLEX)
                        int32 testsetset = 123123;

                    WorldPos worldBlockP = ToWorld(blockp);
#if 1
                    //Vec3 min = Vec3({ Sign(worldBlockP.p.x) * halfLengths.x, Sign(worldBlockP.p.y) * halfLengths.y, Sign(worldBlockP.p.z) * halfLengths.z });
                    Vec3 min = Vec3({ halfLengths.x, halfLengths.y, halfLengths.z });
                    //Vec3 min = Vec3({ collider.m_length, halfLengths.y, collider.m_length });
                    Vec3 minkowskiMax = {};
#if 0
                    minkowskiMax.x = (g_blocks[+blockCheck].m_size.x + collider.m_length);
                    minkowskiMax.y = (g_blocks[+blockCheck].m_size.y + collider.m_length);
                    minkowskiMax.z = (g_blocks[+blockCheck].m_size.z + collider.m_length);
#else
                    minkowskiMax.x = (g_blocks[+blockCheck].m_size.x + halfLengths.x);
                    minkowskiMax.y = (g_blocks[+blockCheck].m_size.y + halfLengths.y);
                    minkowskiMax.z = (g_blocks[+blockCheck].m_size.z + halfLengths.z);

                    //minkowskiMax.x = (g_blocks[+blockCheck].m_size.x + collider.m_length);
                    //minkowskiMax.y = (g_blocks[+blockCheck].m_size.y + halfLengths.y);
                    //minkowskiMax.z = (g_blocks[+blockCheck].m_size.z + collider.m_length);
#endif
                    AABB minkowskiSum = {
                        .min = worldBlockP.p - min,
                        .max = worldBlockP.p + minkowskiMax,
                    };
                    AddCubeToRender(minkowskiSum.Center(), transRed, minkowskiSum.GetLengths());
#else
                    AABB minkowskiSum = {
                        .min = worldBlockP.p - halfLengths,
                        .max = worldBlockP.p + Vec3( { 1.0f, 1.0f, 1.0f} ) + halfLengths,
                    };
#endif
                    Ray ray = {
                        .origin = collider.m_center.p,
                        .direction = Normalize(pDelta),
                    };

                    float distanceToMove;
                    Vec3 intersect;
                    Vec3 normal;
                    Vec3 direction;
                    uint8 face;
                    if (RayVsAABB(ray, minkowskiSum, distanceToMove, intersect, normal, face, direction))
                    {
                        if ((normal.x != 0.0f && Sign(normal.x) == Sign(pDelta.x)) ||
                            (normal.y != 0.0f && Sign(normal.y) == Sign(pDelta.y)) ||
                            (normal.z != 0.0f && Sign(normal.z) == Sign(pDelta.z)))
                            continue;

                        if (distanceToMove < Length(pDelta))
                        {
                            //BlockType normalFaceBlockType;
                            //GamePos normalFaceBlockP = ToGame(WorldPos(worldBlockP.p + normal));
                            //if (!(g_chunks->GetBlock(normalFaceBlockType, normalFaceBlockP)) || (!(g_blocks[+normalFaceBlockType].m_flags & BLOCK_COLLIDABLE)))
                            {
                                if (normal.x != 0)
                                    normalMap.x = 1;
                                else if (normal.y != 0)
                                    normalMap.y = 1;
                                else if (normal.z != 0)
                                    normalMap.z = 1;

                                Vec3 delta = NormalizeZero(intersect - collider.m_center.p) * distanceToMove;
                                if (distanceToMove == 0)
                                    delta = direction;
                                collider.m_center.p += delta;
                                result = true;
                            }
                        }
                    }
                }
            }
        }

    }

//#if RAY_ATTEMPT != 0
    if (result)
    {
        out_positionDelta = HadamardProduct((collider.m_center.p - boxOrigin.p) - deltaPOrigin, normalMap);
    }
//#endif
    return result;
}
