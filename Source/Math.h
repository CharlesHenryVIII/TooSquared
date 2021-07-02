#pragma once
//#include "SDL\include\SDL_pixels.h"
#include "gb_math.h"

#include <vector>
#include <cassert>
#include <cmath>
#include <cstdint>

#define BIT(num) (1<<(num))

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

union Color {
    struct { float r, g, b, a; };
    float e[4];
};



const Color Red = { 1.0f, 0.0f, 0.0f, 1.0f };
const Color Green = { 0.0f, 1.0f, 0.0f, 1.0f };
const Color Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
const Color transRed = { 1.0f, 0.0f, 0.0f, 0.5f };
const Color transGreen = { 0.0f, 1.0f, 0.0f, 0.5f };
const Color transBlue = { 0.0f, 0.0f, 1.0f, 0.5f };
const Color transOrange = { 1.0f, 0.5f, 0.0f, 0.5f };
const Color lightRed = { 1.0f, 0.0f, 0.0f, 0.25f };
const Color lightGreen = { 0.0f, 1.0f, 0.0f, 0.25f };
const Color lightBlue = { 0.0f, 0.0f, 1.0f, 0.25f };
const Color White = { 1.0f, 1.0f, 1.0f, 1.0f };
const Color lightWhite = { 0.58f, 0.58f, 0.58f, 0.58f };
const Color Black = { 0.0f, 0.0f, 0.0f, 1.0f };
const Color lightBlack = { 0.0f, 0.0f, 0.0f, 0.58f };
const Color Brown = { 0.5f, 0.4f, 0.25f, 1.0f };  //used for dirt block
const Color Mint = { 0.0f, 1.0f, 0.5f, 1.0f };   //used for corner block
const Color Orange = { 1.0f, 0.5f, 0.0f, 1.0f };   //used for edge block
const Color Grey = { 0.5f, 0.5f, 0.5f, 1.0f }; //used for floating block

const Color HealthBarBackground = { 0.25f, 0.33f, 0.25f, 1.0f};
constexpr int32 blockSize = 32;

const float pi = 3.14159f;
const float tau = 2 * pi;
const float inf = INFINITY;

const uint32 CollisionNone = 0;
const uint32 CollisionTop = 1;
const uint32 CollisionBot = 2;
const uint32 CollisionRight = 4;
const uint32 CollisionLeft = 8;

typedef gbVec2 Vec2;
typedef gbVec3 Vec3;
typedef gbVec4 Vec4;
typedef gbMat2 Mat2;
typedef gbMat3 Mat3;
typedef gbMat4 Mat4;
typedef gbQuat Quat;

[[nodiscard]] inline Vec4 GetVec4(Vec3 a, float b)
{
    return { a.x, a.y, a.z, b };
}

//union Vert2d {
//    struct { float x, y, z, u, v; };
//    struct { Vec3 xyz; Vec2 uv; };
//    float e[5];
//};

struct Vertex {
    Vec3 p;
    Vec2 uv;
    Vec3 n;
};

struct Vertex_UI {
    Vec2 p;
    Vec2 uv;
};

//Originally 32 Bytes
//TODO: convert to 4 bytes by slaming connected vertices and n together
#pragma pack(push, 1)
struct Vertex_Chunk {
    uint16 blockIndex;
    uint8 spriteIndex;
    uint8 nAndConnectedVertices = 0;
    //uint8 connectedVertices = 0;
};
#pragma pack(pop)


union Vec2Int {
    struct { int32 x, y; };
    int32 e[2];
};

union Vec3Int {
    struct { int32 x, y, z; };

    Vec2Int xy;
    int32 e[3];
};

union Vec2d {
    struct { double x, y; };
    double e[2];
};

struct RectInt {
    Vec2Int botLeft = {};
    Vec2Int topRight = {};

    int32 Width()
    {
        return topRight.x - botLeft.x;
    }

    int32 Height()
    {
        return topRight.y - botLeft.y;
    }

};

struct Rect {
    Vec2 botLeft = {};
    Vec2 topRight = {};

    float Width()
    {
        return topRight.x - botLeft.x;
    }

    float Height()
    {
        return topRight.y - botLeft.y;
    }

};

struct LineSegment {
    Vec2 p0;
    Vec2 p1;

    Vec2 Normal()
    {
        //TODO CHECK IF THIS IS RIGHT
        Vec2 result = (p1 - p0);
        result = { -result.y, result.x };
        return result;
    }
};


template <typename T = float>
struct Range {
    T min, max;

//    float RandomInRange()
//    {
//        return Random<float>(min, max);
//    }
//    void AngleSymetric(float angle, float range)
//    {
//        min = angle - range / 2;
//        max = angle + range / 2;
//    }
};


//struct Rectangle {
//    Vec2 botLeft;
//    Vec2 topRight;
//
//    bool Collision(Vec2Int loc)
//    {
//        bool result = false;
//        if (loc.y > botLeft.x && loc.y < topRight.x)
//            if (loc.x > botLeft.x && loc.x < topRight.x)
//                result = true;
//        return result;
//    }
//};

struct Rectangle_Int {
    Vec2Int bottomLeft;
    Vec2Int topRight;

    int32 Width() const
    {
        return topRight.x - bottomLeft.x;
    }

    int32 Height() const
    {
        return topRight.y - bottomLeft.y;
    }
};

//inline Vec2 operator-(const Vec2& v)
//{
//    return { -v.x, -v.y };
//}
//
//inline Vec2 operator-(const Vec2& lhs, const float rhs)
//{
//    return { lhs.x - rhs, lhs.y - rhs };
//}


[[nodiscard]] inline Vec2Int operator*(const Vec2Int& a, const float b)
{
    return { int(a.x * b),  int(a.y * b) };
}

//inline Vec2 operator/(const Vec2& a, const float b)
//{
//    return { a.x / b,  a.y / b };
//}
//inline Vec2 operator/(const float lhs, const Vec2& rhs)
//{
//    return { lhs / rhs.x,  lhs / rhs.y };
//}
//
//inline bool operator==(const Rectangle& lhs, const Rectangle& rhs)
//{
//    bool bl = lhs.botLeft == rhs.botLeft;
//    bool tr = lhs.topRight == rhs.topRight;
//    return bl && tr;
//}
//
//inline bool operator!=(const Rectangle& lhs, const Rectangle& rhs)
//{
//    return !(lhs == rhs);
//}
[[nodiscard]] inline Vec3 operator+(Vec3 a, float b)
{
    Vec3 r = {a.x + b, a.y + b, a.z + b};
    return r;
}
[[nodiscard]] inline Vec3 operator-(Vec3 a, float b)
{
    Vec3 r = {a.x - b, a.y - b, a.z - b};
    return r;
}
inline void operator+=(Vec3& a, float b)
{
    a = {a.x + b, a.y + b, a.z + b};
}
[[nodiscard]] inline Vec3 operator-=(Vec3& a, float b)
{
    a = {a.x - b, a.y - b, a.z - b};
    return a;
}
[[nodiscard]] inline bool operator==(Vec3Int a, Vec3Int b)
{
    return ((a.x == b.x) && (a.y == b.y) && (a.z == b.z));
}
[[nodiscard]] inline Vec3Int operator-(Vec3Int a, Vec3Int b)
{
    Vec3Int r = {a.x - b.x, a.y - b.y, a.z - b.z};
    return r;
}
[[nodiscard]] inline Vec3Int operator+(Vec3Int a, Vec3Int b)
{
    Vec3Int r = {a.x + b.x, a.y + b.y, a.z + b.z};
    return r;
}
 inline Vec3Int& operator+=(Vec3Int &a, Vec3Int b)
{
    return (a = a + b);
}
inline Vec3Int& operator-=(Vec3Int &a, Vec3Int b)
{
    return (a = a - b);
}
//
[[nodiscard]] inline Vec3Int operator/(Vec3Int a, int32 b)
{
    Vec3Int r = {a.x / b, a.y / b, a.z / b };
    return r;
}
[[nodiscard]] inline Vec3Int operator/=(Vec3Int& a, int32 b)
{
    a = a / b;
}
[[nodiscard]] inline Vec2Int operator/(Vec2Int a, int32 b)
{
    Vec2Int r = {a.x / b, a.y / b };
    return r;
}
[[nodiscard]] inline Vec2Int operator/=(Vec2Int& a, int32 b)
{
    a = a / b;
}
//
[[nodiscard]] inline Vec3Int operator%(Vec3Int a, Vec3Int b)
{
    Vec3Int r = {a.x % b.x, a.y % b.y, a.z % b.z };
    return r;
}
[[nodiscard]] inline Vec3Int operator%=(Vec3Int& a, Vec3Int b)
{
    a = a % b;
}
//
[[nodiscard]] inline Vec3Int operator*(int32 a, Vec3Int b)
{
    Vec3Int r = { a * b.x, a * b.y, a * b.z };
    return r;
}
[[nodiscard]] inline Vec3Int operator*(Vec3Int a, int32 b)
{
    Vec3Int r = { a.x * b, a.y * b, a.z * b };
    return r;
}
[[nodiscard]] inline Vec2Int operator*(int32 a, Vec2Int b)
{
    Vec2Int r = { a * b.x, a * b.y };
    return r;
}
[[nodiscard]] inline Vec2Int operator*(Vec2Int a, int32 b)
{
    Vec2Int r = { a.x * b, a.y * b };
    return r;
}
//
[[nodiscard]] inline Vec3Int operator+(Vec3Int a, int32 b)
{
    Vec3Int r = { a.x + b, a.y + b, a.z + b };
    return r;
}
[[nodiscard]] inline Vec3Int operator+(int32 a, Vec3Int b)
{
    Vec3Int r = { a + b.x, a + b.y, a + b.z };
    return r;
}
[[nodiscard]] inline Vec2Int operator+(Vec2Int a, int32 b)
{
    Vec2Int r = { a.x + b, a.y + b};
    return r;
}
[[nodiscard]] inline Vec2Int operator+(int32 a, Vec2Int b)
{
    Vec2Int r = { a + b.x, a + b.y };
    return r;
}


[[nodiscard]] inline bool operator==(Vec2Int a, Vec2Int b)
{
    return ((a.x == b.x) && (a.y == b.y));
}
[[nodiscard]] inline Vec2Int operator-(Vec2Int a, Vec2Int b)
{
    Vec2Int r = {a.x - b.x, a.y - b.y };
    return r;
}
[[nodiscard]] inline Vec2Int operator+(Vec2Int a, Vec2Int b)
{
    Vec2Int r = {a.x + b.x, a.y + b.y };
    return r;
}
 inline Vec2Int &operator+=(Vec2Int &a, Vec2Int b)
{
    return (a = a + b);
}
inline Vec2Int &operator-=(Vec2Int &a, Vec2Int b)
{
    return (a = a - b);
}
[[nodiscard]] inline Vec2Int operator%(Vec2Int a, Vec2Int b)
{
    Vec2Int r = { a.x % b.x, a.y % b.y };
    return r;
}
[[nodiscard]] inline Vec2Int operator%=(Vec2Int& a, Vec2Int b)
{
    a = a % b;
}


[[nodiscard]] inline Vec3 operator+(float a, Vec3 b)
{
    Vec3 r = { a + b.x, a + b.y, a + b.z };
    return r;
}
[[nodiscard]] inline Vec3 operator-(float a, Vec3 b)
{
    Vec3 r = { a - b.x, a - b.y, a - b.z };
    return r;
}
[[nodiscard]] inline Vec2 operator+(Vec2 a, float b)
{
    Vec2 r = { a.x + b, a.y + b };
    return r;
}
[[nodiscard]] inline Vec2 operator+(float a, Vec2 b)
{
    Vec2 r = { a + b.x, a + b.y };
    return r;
}
[[nodiscard]] inline Vec2 operator-(Vec2 a, float b)
{
    Vec2 r = { a.x - b, a.y - b };
    return r;
}
[[nodiscard]] inline Vec2 operator-(float a, Vec2 b)
{
    Vec2 r = { a - b.x, a - b.y };
    return r;
}



template <typename T>
[[nodiscard]] T Min(T a, T b)
{
    return a < b ? a : b;
}

 template <typename T>
[[nodiscard]] T Max(T a, T b)
{
    return a > b ? a : b;
}

template <typename T>
[[nodiscard]] T Clamp(T v, T min, T max)
{
    return Max(min, Min(max, v));
}

[[nodiscard]] inline Vec2 Floor(Vec2 v)
{
    return { floorf(v.x), floorf(v.y) };
}

[[nodiscard]] inline Vec3 Floor(Vec3 v)
{
    return { floorf(v.x), floorf(v.y), floorf(v.z) };
}

[[nodiscard]] inline Vec4 Floor(Vec4 v)
{
    return { floorf(v.x), floorf(v.y), floorf(v.z), floorf(v.w)  };
}

[[nodiscard]] inline float Fract(float a)
{
    return a - floorf(a);
}

[[nodiscard]] inline Vec2 Fract(Vec2 a)
{
    return a - Floor(a);
}

[[nodiscard]] inline Vec3 Fract(Vec3 a)
{
    return a - Floor(a);
}

[[nodiscard]] inline Vec4 Fract(Vec4 a)
{
    return a - Floor(a);
}

[[nodiscard]] inline float Abs(float a)
{
    return fabs(a);
}
[[nodiscard]] inline Vec3 Abs(Vec3 a)
{
    return { fabs(a.x), fabs(a.y), fabs(a.z) };
}
[[nodiscard]] inline Vec3 Abs(Vec2 a)
{
    return { abs(a.x), abs(a.y) };
}
[[nodiscard]] inline Vec3Int Abs(Vec3Int a)
{
    return { abs(a.x), abs(a.y), abs(a.z) };
}
[[nodiscard]] inline Vec2Int Abs(Vec2Int a)
{
    return { abs(a.x), abs(a.y) };
}

[[nodiscard]] inline Vec2 Sine(Vec2 v)
{
    Vec2 r = { sinf(v.x), sinf(v.y) };
    return r;
}

[[nodiscard]] inline Vec3 Sine(Vec3 v)
{
    Vec3 r = { sinf(v.x), sinf(v.y), sinf(v.z) };
    return r;
}

//MATH AND CONVERSIONS

[[nodiscard]] inline float RadToDeg(float angle)
{
    return ((angle) / (tau)) * 360;
}

[[nodiscard]] inline float DegToRad(float angle)
{
    return (angle / 360 ) * (tau);
}

//inline float cos(float t)
//{
//    return cosf(t);
//}
//
//inline float sine(float t)
//{
//    return sinf(t);
//}
//
//inline float tan(float t)
//{
//    return tanf(t);
//}

[[nodiscard]] inline Vec3 Cross(Vec3 a, Vec3 b)
{
    Vec3 result;
    gb_vec3_cross(&result, a, b);
    return result;
}

//inline float Vec2Distance(Vec2 A, Vec2 B)
//{
//    return sqrtf(powf(B.x - A.x, 2) + powf(B.y - A.y, 2));
//}

//TODO: Remove powf and replace with * operator
//TODO: Improve
[[nodiscard]] inline Vec3 NormalizeZero(Vec3 v)
{
    float prod = v.x * v.x + v.y * v.y + v.z * v.z;
    if (prod == 0.0f)
        return {};
    float hyp = sqrtf(prod);
    Vec3 result = { (v.x / hyp), (v.y / hyp) , (v.z / hyp)};
    return result;
}
[[nodiscard]] inline Vec3 Normalize(Vec3 v)
{
    float hyp = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    Vec3 result = { (v.x / hyp), (v.y / hyp) , (v.z / hyp)};
    return result;
}
[[nodiscard]] inline Vec2 Normalize(Vec2 v)
{
    float hyp = sqrtf(powf(v.x, 2) + powf(v.y, 2));
    return { (v.x / hyp), (v.y / hyp) };
}
inline float* Normalize(float* v, size_t length)
{
    float total = 0;
    for (int32 i = 0; i < length; i++)
    {
        total += (v[i] * v[i]);
    }
    float hyp = sqrtf(total);
    for (int32 i = 0; i < length; i++)
    {
        v[i] = v[i] / hyp;
    }
    return v;
}
inline double* Normalize(double* v, size_t length)
{
    double total = 0;
    for (int32 i = 0; i < length; i++)
    {
        total += (v[i] * v[i]);
    }
    double hyp = sqrt(total);
    for (int32 i = 0; i < length; i++)
    {
        v[i] = v[i] / hyp;
    }
    return v;
}
//inline Vec2 NormalizeZero(Vec2 v)
//{
//    float hyp = sqrtf(powf(v.x, 2) + powf(v.y, 2));
//    if (hyp == 0)
//        return {};
//	return{ (v.x / hyp), (v.y / hyp) };
//}

template <typename T>
[[nodiscard]] inline T Lerp(T a, T b, float t)
{
    return a + (b - a) * t;
}

[[nodiscard]] inline Vec3 Converge(Vec3 value, Vec3 target, float rate, float dt)
{
    return Lerp(target, value, exp2(-rate * dt));
}

//[[nodiscard]] inline float Lerp(float a, float b, float t)
//{
//    return a + (b - a) * t;
//}

#if 1
float Bilinear(float p00, float p10, float p01, float p11, float x, float y);
#else
[[nodiscard]] float Bilinear(Vec2 p, Rect loc, float bl, float br, float tl, float tr);
#endif
[[nodiscard]] float Cubic(Vec4 v, float x);
[[nodiscard]] float Bicubic(Mat4 p, Vec2 pos);

/*
Atan2f return value:

3pi/4      pi/2        pi/4


            O
+/-pi      /|\          0
           / \


-3pi/4    -pi/2        pi/4
*/

[[nodiscard]] inline float DotProduct(Vec2 a, Vec2 b)
{
    float r = a.x * b.x + a.y * b.y;
    return r;
}
[[nodiscard]] inline float DotProduct(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
[[nodiscard]] inline Vec3 CrossProduct(Vec3 a, Vec3 b)
{
    Vec3 r = {};
    gb_vec3_cross(&r, a, b);
    return r;
}
[[nodiscard]] inline float CrossProduct(Vec2 a, Vec2 b)
{
    float r = {};
    gb_vec2_cross(&r, a, b);
    return r;
}

[[nodiscard]] inline float Pythags(const Vec2& a)
{
    return sqrtf(powf(a.x, 2) + powf(a.y, 2));
}
[[nodiscard]] inline float Pythags(const Vec3& a)
{
    return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}
[[nodiscard]] inline float Pythags(const Vec2Int& a)
{
    return Pythags(Vec2({ float(a.x), float(a.y) }));
}
[[nodiscard]] inline float Pythags(const Vec3Int& a)
{
    return Pythags(Vec3({ float(a.x), float(a.y), float(a.z) }));
}

[[nodiscard]] inline float Distance(Vec2 a, Vec2 b)
{
    return Pythags(a - b);
}
[[nodiscard]] inline double DistanceD(Vec2 a, Vec2 b)
{
    return Pythags(a - b);
}
[[nodiscard]] inline double Distance(Vec3Int a, Vec3Int b)
{
    return Pythags(a - b);
}
[[nodiscard]] inline double Distance(Vec2Int a, Vec2Int b)
{
    return Pythags(a - b);
}
[[nodiscard]] inline float Distance(Vec3 a, Vec3 b)
{
    return Pythags(a - b);
}

[[nodiscard]] inline float Length(Vec2 a)
{
    return Pythags(a);
}
[[nodiscard]] inline double LengthD(Vec2 a)
{
    return Pythags(a);
}
[[nodiscard]] inline double Length(Vec3Int a)
{
    return Pythags(a);
}
[[nodiscard]] inline double Length(Vec2Int a)
{
    return Pythags(a);
}
[[nodiscard]] inline float Length(Vec3 a)
{
    return Pythags(a);
}
[[nodiscard]] inline Vec3 Acos(Vec3 a)
{
    return { acos(a.x), acos(a.y), acos(a.z) };
}

[[nodiscard]] inline Vec3 Round(const Vec3& a)
{
    Vec3 r = { roundf(a.x), roundf(a.y), roundf(a.z) };
    return r;
}
[[nodiscard]] inline float Sign(float value)
{
    return value < 0.0f ? -1.0f : 1.0f;
}
[[nodiscard]] inline int32 Sign(int32 value)
{
    return value < 0 ? -1 : 1;
}


[[nodiscard]] uint32 PCG_Random(uint64 state);
[[nodiscard]] inline uint32 RandomU32(uint64 state, uint32 min, uint32 max)
{
    uint32 result = PCG_Random(state);
    //uint32 result = state;
    result ^= result << 13;
    result ^= result >> 17;
    result ^= result << 5;
    return (result % (max - min)) + min;
}

[[nodiscard]] inline float RandomFloat(const float min, const float max)
{
    return min + (max - min) * (rand() / float(RAND_MAX));
}


[[nodiscard]] inline Vec3Int Vec3ToVec3Int(Vec3 a)
{
    return { static_cast<int32>(a.x), static_cast<int32>(a.y), static_cast<int32>(a.z) };
}

[[nodiscard]] inline Vec3 Vec3IntToVec3(Vec3Int a)
{
    return { static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z) };
}

[[nodiscard]] inline Vec2Int Vec2ToVec2Int(Vec2 a)
{
    return { static_cast<int32>(a.x), static_cast<int32>(a.y) };
}

[[nodiscard]] inline Vec2 Vec2IntToVec2(Vec2Int a)
{
    return { static_cast<float>(a.x), static_cast<float>(a.y) };
}
//Multiplication of two vectors without adding each dimension to get the dot product
[[nodiscard]] inline Vec3Int HadamardProduct(Vec3Int a, Vec3Int b)
{
    return { a.x * b.x, a.y * b.y, a.z * b.z };
}

struct ChunkPos {
    Vec3Int p;
};

struct GamePos {
    Vec3Int p;
};

struct WorldPos {
    Vec3 p;
    WorldPos() = default;
    WorldPos(Vec3 a)
    {
        p.x = a.x;
        p.y = a.y;
        p.z = a.z;
    }
    WorldPos(float a, float b, float c)
    {
        p.x = a;
        p.y = b;
        p.z = c;
    }
};

[[nodiscard]] WorldPos ToWorld(GamePos a);
[[nodiscard]] WorldPos ToWorld(ChunkPos a);
[[nodiscard]] GamePos ToGame(ChunkPos a);
[[nodiscard]] GamePos ToGame(WorldPos a);
[[nodiscard]] ChunkPos ToChunk(GamePos a);
[[nodiscard]] ChunkPos ToChunk(WorldPos a);

[[nodiscard]] inline WorldPos Floor(WorldPos v)
{
    return { floorf(v.p.x), floorf(v.p.y), floorf(v.p.z) };
}

struct Plane
{
   float x,y,z,w;
};

struct Frustum {
    Plane e[6];
};

//typedef union gbVec3 {
//	struct { float x, y, z; };
//	struct { float r, g, b; };
//
//	gbVec2 xy;
//	float e[3];
//} gbVec3;

union Triangle {
    struct { WorldPos p0, p1, p2; };
    WorldPos e[3];
        
    Vec3 Normal() const
    {
        Vec3 r = Normalize(CrossProduct(p1.p - p0.p, p2.p - p0.p));
        return r;
    }
    Vec3 Center() const
    {
        Vec3 r = (p0.p + p1.p + p2.p) / 3;
        return r;
    }
};

Frustum ComputeFrustum(const Mat4& mvProj);
bool IsBoxInFrustum(const Frustum& f, float* bmin, float* bmax);
int32 ManhattanDistance(Vec3Int a, Vec3Int b);

struct AABB {
    Vec3 min = {};
    Vec3 max = {};

    [[nodiscard]] Vec3 GetLengths() const
    {
        Vec3 result = {};
        result.x = Abs(max.x - min.x);
        result.y = Abs(max.y - min.y);
        result.z = Abs(max.z - min.z);
        return result;
    }

    [[nodiscard]] Vec3 Center() const
    {
        Vec3 result = {};
        result = min + ((max - min) / 2);
        return result;
    }
};

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

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
bool RayVsAABB(const Ray& ray, const AABB& box, float& min, Vec3& intersect, Vec3& normal);
bool RayVsAABB(const Ray& ray, const AABB& box);
bool AABBVsAABB(const AABB& box1, const AABB& box2);
bool AABBVsAABB(Vec3& out_intersection, const AABB& box1, const AABB& box2);
bool SphereVsTriangle(const Vec3& center, const float radius, const Triangle& triangle, Vec3& directionToTriangle, float& distance);
Vec3 ClosestPointOnLineSegment(const Vec3& A, const Vec3& B, const Vec3& Point);
bool CapsuleVsBlock(Capsule collider, const BlockSampler& region, Vec3& toOutside, std::vector<Triangle>& debug_triangles);
bool CapsuleVsWorldBlocks(Capsule capsuleCollider, Vec3 in_positionDelta, Vec3& out_positionDelta, std::vector<Triangle>& debug_trianglesToDraw);
bool CubeVsWorldBlocks(Cube collider, Vec3 in_positionDelta, Vec3& out_positionDelta, std::vector<Triangle>& debug_trianglesToDraw);

void QuickSort(uint8* data, const int32 length, const int32 itemSize, int32 (*compare)(const void* a, const void* b));
