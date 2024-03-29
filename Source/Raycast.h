#pragma once
#include "Math.h"

struct RaycastResult {
    bool success = false;
    GamePos p = {};
    float distance = {};
    Vec3 normal = {};
};

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

RaycastResult LineCast(const Ray& ray, float length);
RaycastResult RayVsChunk(const Ray& ray, float length);
bool RayVsAABB(const Ray& ray, const AABB& box, float& min, Vec3& intersect, Vec3& normal, Vec3& actualMove);
bool RayVsAABB(const Ray& ray, const AABB& box);
