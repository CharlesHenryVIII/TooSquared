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



const Color Red         = { 1.00f, 0.00f, 0.00f, 1.00f };
const Color Green       = { 0.00f, 1.00f, 0.00f, 1.00f };
const Color Blue        = { 0.00f, 0.00f, 1.00f, 1.00f };
const Color transRed    = { 1.00f, 0.00f, 0.00f, 0.50f };
const Color transGreen  = { 0.00f, 1.00f, 0.00f, 0.50f };
const Color transBlue   = { 0.00f, 0.00f, 1.00f, 0.50f };
const Color transOrange = { 1.00f, 0.50f, 0.00f, 0.50f };
const Color lightRed    = { 1.00f, 0.00f, 0.00f, 0.25f };
const Color lightGreen  = { 0.00f, 1.00f, 0.00f, 0.25f };
const Color lightBlue   = { 0.00f, 0.00f, 1.00f, 0.25f };
const Color White       = { 1.00f, 1.00f, 1.00f, 1.00f };
const Color lightWhite  = { 0.58f, 0.58f, 0.58f, 0.58f };
const Color Black       = { 0.00f, 0.00f, 0.00f, 1.00f };
const Color lightBlack  = { 0.00f, 0.00f, 0.00f, 0.58f };
const Color Brown       = { 0.50f, 0.40f, 0.25f, 1.00f };
const Color Mint        = { 0.00f, 1.00f, 0.50f, 1.00f };
const Color Orange      = { 1.00f, 0.50f, 0.00f, 1.00f };
const Color Grey        = { 0.50f, 0.50f, 0.50f, 1.00f };

const Color backgroundColor = { 0.263f, 0.706f, 0.965f, 0.0f };

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

#define MATH_PREFIX [[nodiscard]] inline

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
#pragma pack(push, 1)
struct Vertex_Block {
    Vec3  p;
    Vec3  scale;
    uint8 index;
};
struct Vertex_Cube {
    Vec3  p;
    Vec4  color;
    Vec3  scale;
};
struct Vertex_Complex {
    Vec3  p;
    Vec2  uv;
    Vec3  n;
    uint8 i;
};
#pragma pack(pop)
struct Vertex_UI {
    Vec2 p;
    Vec2 uv;
};

//Originally 32 Bytes
#pragma pack(push, 1)
struct Vertex_Chunk {
    uint16 blockIndex;
    uint8 spriteIndex;
    uint8 nAndConnectedVertices = 0;
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
    //[[nodiscard]] inline T Center()
    MATH_PREFIX T Center() const
    {
        return min + ((max - min) / 2);
    }
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
[[nodiscard]] inline bool operator>(Vec3Int a, Vec3Int b)
{
    return ((a.x > b.x) && (a.y > b.y) && (a.z > b.z));
}
[[nodiscard]] inline bool operator<(Vec3Int a, Vec3Int b)
{
    return ((a.x < b.x) && (a.y < b.y) && (a.z < b.z));
}
[[nodiscard]] inline bool operator>=(Vec3Int a, Vec3Int b)
{
    return ((a.x >= b.x) && (a.y >= b.y) && (a.z >= b.z));
}
[[nodiscard]] inline bool operator<=(Vec3Int a, Vec3Int b)
{
    return ((a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z));
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
[[nodiscard]] inline Vec3 operator/(float a, Vec3 b)
{
    Vec3 r = { a / b.x, a / b.y, a / b.z };
    return r;
}
[[nodiscard]] inline Vec2 operator/(float a, Vec2 b)
{
    Vec2 r = { a / b.x, a / b.y };
    return r;
}
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
[[nodiscard]] inline Vec3Int operator-(Vec3Int a, int32 b)
{
    Vec3Int r = { a.x - b, a.y - b, a.z - b };
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
[[nodiscard]] inline Vec2Int operator-(Vec2Int a, int32 b)
{
    Vec2Int r = { a.x - b, a.y - b};
    return r;
}


[[nodiscard]] inline bool operator==(Vec2Int a, Vec2Int b)
{
    return ((a.x == b.x) && (a.y == b.y));
}
[[nodiscard]] inline bool operator>(Vec2Int a, Vec2Int b)
{
    return ((a.x > b.x) && (a.y > b.y));
}
[[nodiscard]] inline bool operator<(Vec2Int a, Vec2Int b)
{
    return ((a.x < b.x) && (a.y < b.y));
}
[[nodiscard]] inline bool operator>=(Vec2Int a, Vec2Int b)
{
    return ((a.x >= b.x) && (a.y >= b.y));
}
[[nodiscard]] inline bool operator<=(Vec2Int a, Vec2Int b)
{
    return ((a.x <= b.x) && (a.y <= b.y));
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

[[nodiscard]] inline Vec3 Trunc(Vec3 v)
{
    return { truncf(v.x), truncf(v.y), truncf(v.z) };
}

[[nodiscard]] inline Vec4 Trunc(Vec4 v)
{
    return { truncf(v.x), truncf(v.y), truncf(v.z), truncf(v.w)  };
}

[[nodiscard]] inline Vec3 Ceiling(Vec3 v)
{
    return { ceilf(v.x), ceilf(v.y), ceilf(v.z) };
}

[[nodiscard]] inline Vec4 Ceiling(Vec4 v)
{
    return { ceilf(v.x), ceilf(v.y), ceilf(v.z), ceilf(v.w)  };
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
//    return sqrtf(((B.x - A.x) * (B.x - A.x)) + ((B.y - A.y) * (B.y - A.y)));
//}

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
    float hyp = sqrtf((v.x * v.x) + (v.y * v.y));
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
//    float hyp = sqrtf((v.x * v.x) + (v.y * v.y));
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
    return sqrtf((a.x * a.x) + (a.y * a.y));
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
//Multiplication of two vectors without adding each dimension to get the dot product
[[nodiscard]] inline Vec3 HadamardProduct(Vec3 a, Vec3 b)
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


struct VertexFace {

    Vec3 e[4];
};

//static const Vec3 cubeVertices[] = {
//    // +x
//    gb_vec3(1.0f, 1.0f, 1.0f),
//    gb_vec3(1.0f, 0.0f, 1.0f),
//    gb_vec3(1.0f, 1.0f, 0.0f),
//    gb_vec3(1.0f, 0.0f, 0.0f),
//    // -x
//    gb_vec3(0.0f, 1.0f, 0.0f),
//    gb_vec3(0.0f, 0.0f, 0.0f),
//    gb_vec3(0.0f, 1.0f, 1.0f),
//    gb_vec3(0.0f, 0.0f, 1.0f),
//    // +y
//    gb_vec3(1.0f, 1.0f, 1.0f),
//    gb_vec3(1.0f, 1.0f, 0.0f),
//    gb_vec3(0.0f, 1.0f, 1.0f),
//    gb_vec3(0.0f, 1.0f, 0.0f),
//    // -y
//    gb_vec3(0.0f, 0.0f, 1.0f),
//    gb_vec3(0.0f, 0.0f, 0.0f),
//    gb_vec3(1.0f, 0.0f, 1.0f),
//    gb_vec3(1.0f, 0.0f, 0.0f),
//    // z
//    gb_vec3(0.0f, 1.0f, 1.0f),
//    gb_vec3(0.0f, 0.0f, 1.0f),
//    gb_vec3(1.0f, 1.0f, 1.0f),
//    gb_vec3(1.0f, 0.0f, 1.0f),
//    // -z
//    gb_vec3(1.0f, 1.0f, 0.0f),
//    gb_vec3(1.0f, 0.0f, 0.0f),
//    gb_vec3(0.0f, 1.0f, 0.0f),
//    gb_vec3(0.0f, 0.0f, 0.0f),
//};

const VertexFace cubeVertices[6] = {
    // +x
    VertexFace( {
    Vec3(1.0,  1.0,  1.0),
    Vec3(1.0,  0.0,  1.0),
    Vec3(1.0,  1.0,  0.0),
    Vec3(1.0,  0.0,  0.0)
    }),
    // -x
    VertexFace({
    Vec3(0.0,  1.0,  0.0),
    Vec3(0.0,  0.0,  0.0),
    Vec3(0.0,  1.0,  1.0),
    Vec3(0.0,  0.0,  1.0)
    }),
    // +y
    VertexFace({
    Vec3(1.0,  1.0,  1.0 ),
    Vec3(1.0,  1.0,  0.0 ),
    Vec3(0.0,  1.0,  1.0 ),
    Vec3(0.0,  1.0,  0.0 )
    }),
    // -y
    VertexFace({
    Vec3(0.0,  0.0,  1.0 ),
    Vec3(0.0,  0.0,  0.0 ),
    Vec3(1.0,  0.0,  1.0 ),
    Vec3(1.0,  0.0,  0.0 )
    }),
    // z
    VertexFace({
    Vec3(0.0,  1.0,  1.0 ),
    Vec3(0.0,  0.0,  1.0 ),
    Vec3(1.0,  1.0,  1.0 ),
    Vec3(1.0,  0.0,  1.0 )
    }),
    // -z
    VertexFace({
    Vec3(1.0,  1.0,  0.0 ),
    Vec3(1.0,  0.0,  0.0 ),
    Vec3(0.0,  1.0,  0.0 ),
    Vec3(0.0,  0.0,  0.0 )
    })
};

const VertexFace smallCubeVertices[6] = {
    // +x
    VertexFace( {
    Vec3(1.0,  0.5,  1.0),
    Vec3(1.0,  0.0,  1.0),
    Vec3(1.0,  0.5,  0.0),
    Vec3(1.0,  0.0,  0.0)
    }),
    // -x
    VertexFace({
    Vec3(0.0,  0.5,  0.0),
    Vec3(0.0,  0.0,  0.0),
    Vec3(0.0,  0.5,  1.0),
    Vec3(0.0,  0.0,  1.0)
    }),
    // +y
    VertexFace({
    Vec3(1.0,  0.5,  1.0 ),
    Vec3(1.0,  0.5,  0.0 ),
    Vec3(0.0,  0.5,  1.0 ),
    Vec3(0.0,  0.5,  0.0 )
    }),
    // -y
    VertexFace({
    Vec3(0.0,  0.0,  1.0 ),
    Vec3(0.0,  0.0,  0.0 ),
    Vec3(1.0,  0.0,  1.0 ),
    Vec3(1.0,  0.0,  0.0 )
    }),
    // z
    VertexFace({
    Vec3(0.0,  0.5,  1.0 ),
    Vec3(0.0,  0.0,  1.0 ),
    Vec3(1.0,  0.5,  1.0 ),
    Vec3(1.0,  0.0,  1.0 )
    }),
    // -z
    VertexFace({
    Vec3(1.0,  0.5,  0.0 ),
    Vec3(1.0,  0.0,  0.0 ),
    Vec3(0.0,  0.5,  0.0 ),
    Vec3(0.0,  0.0,  0.0 )
    })
};


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


Vec3 ClosestPointOnLineSegment(const Vec3& A, const Vec3& B, const Vec3& Point);

void QuickSort(uint8* data, const int32 length, const int32 itemSize, int32 (*compare)(const void* a, const void* b));
