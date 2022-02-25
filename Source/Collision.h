#pragma once
#include "Math.h"

struct Capsule
{
    float m_radius = 0;
    float m_height = 0;
    WorldPos m_tip;
    WorldPos m_tail;
    std::vector<Triangle> m_collidedTriangles;

    void UpdateMidTipLocation(const WorldPos& newTipLocation)
    {
        m_tip = newTipLocation.p;
        m_tip.p.y += m_radius;
        m_tail = m_tip;
        m_tail.p.y -= m_height;
    }
    void UpdateTailLocation(const WorldPos& loc)
    {
        m_tail.p = loc.p;
        m_tip.p = m_tail.p;
        m_tip.p.y += m_height;
    }
    void UpdateLocation(const WorldPos& positionDelta)
    {
        m_tip.p += positionDelta.p;
        m_tail.p += positionDelta.p;
    }
};

struct Cube
{
    float m_length = 0;
    WorldPos m_center;
    std::vector<Triangle> m_collidedTriangles;

    void UpdateBottomMiddleLocation(const WorldPos& newLoc)
    {
        m_center = newLoc;
        m_center.p.y += m_length / 2.0f;
    }
};

struct Cuboid
{
    Vec3 m_lengths = {};
    WorldPos m_center;
    std::vector<Triangle> m_collidedTriangles;

    void UpdateBottomMiddleLocation(const WorldPos& newLoc)
    {
        m_center = newLoc;
        m_center.p += m_lengths / 2.0f;
    }
};

struct RigidBody {
    Vec3 m_vel = {};
    Vec3 m_acceleration = {};
    Vec3 m_terminalVel = {};
    bool m_isGrounded = false;

    //float mass = {};
    //Vec3 drag = {};
    //float angularDrag = {};
    //bool hasGravity = {};
    //bool isKinematic = {};

    Vec3 GetDeltaPosition(float deltaTime, Vec3 dragCoefficient, float gravity = -10.0f)
    {
        if (m_isGrounded)
            m_vel.y = Max(m_vel.y, 0.0f);

        m_vel.y += (m_acceleration.y + gravity) * deltaTime;
        //m_vel.y = Clamp(m_vel.y, -m_terminalVel.y, m_terminalVel.y);

        m_vel.x += m_acceleration.x * deltaTime;
        m_vel.z += m_acceleration.z * deltaTime;

        //float zeroTolerance = 0.25f;
        //float mass = 1000.0f; //grams
        //Vec3 dragForce = m_vel * dragCoefficient;//dragCoefficient * ((1.255f * m_vel/* * m_vel*/) / 2) * area;
        Vec3 dragVel;// = dragForce / (mass * deltaTime);

        dragVel = m_vel * dragCoefficient * deltaTime;

        m_vel -= dragVel;
        m_vel.x = Clamp(m_vel.x, -m_terminalVel.x, m_terminalVel.x);
        m_vel.y = Clamp(m_vel.y, -m_terminalVel.y, m_terminalVel.y);
        m_vel.z = Clamp(m_vel.z, -m_terminalVel.z, m_terminalVel.z);

        //Velocity and position will be local for Audio and Particle actors
        return (m_vel * deltaTime);
    }
};

struct Transform {
    WorldPos m_p = {};
    //Quat m_quat = gb_quat_identity();
    float m_yaw   = {};
    float m_pitch = {};
    Vec3 m_scale = {};
};

struct BlockSampler;
bool PointVsAABB(const Vec3& point, const AABB& box);
bool AABBVsAABB(const AABB& box1, const AABB& box2);
bool AABBVsAABB(Vec3& out_intersection, const AABB& box1, const AABB& box2);
bool SphereVsTriangle(const Vec3& center, const float radius, const Triangle& triangle, Vec3& directionToTriangle, float& distance);
bool CapsuleVsBlock(Capsule collider, const BlockSampler& region, Vec3& toOutside, std::vector<Triangle>& debug_triangles);
bool CapsuleVsWorldBlocks(Capsule capsuleCollider, Vec3 in_positionDelta, Vec3& out_positionDelta, std::vector<Triangle>& debug_trianglesToDraw);
bool CubeVsWorldBlocks(Cube collider, Vec3 in_positionDelta, Vec3& out_positionDelta, std::vector<Triangle>& debug_trianglesToDraw);
