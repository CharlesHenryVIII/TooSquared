#pragma once
//#include "SDL\include\SDL_pixels.h"
#include "gb_math.h"

#include <cassert>
#include <cmath>
#include <cstdint>

//#define IMPLIMENTATION1
//#define IMPLIMENTATION2
//#define IMPLIMENTATION3
#define IMPLIMENTATION4

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

struct Vertex_Chunk {
    Vec3 p;
    uint8 spriteIndex;
    uint8 n;
};


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


struct Range {
    float min, max;

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


//inline Vec2Int operator*(const Vec2Int& a, const float b)
//{
//    return { int(a.x * b),  int(a.y * b) };
//}
//
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

inline Vec3 operator+(Vec3 a, float b)
{
    Vec3 r = {a.x + b, a.y + b, a.z + b};
    return r;
}

inline Vec3 operator+(float a, Vec3 b)
{
    Vec3 r = { a + b.x, a + b.y, a + b.z };
    return r;
}

inline Vec3 operator-(Vec3 a, float b)
{
    Vec3 r = {a.x - b, a.y - b, a.z - b};
    return r;
}

inline Vec3 operator-(float a, Vec3 b)
{
    Vec3 r = { a - b.x, a - b.y, a - b.z };
    return r;
}

inline Vec2 operator+(Vec2 a, float b)
{
    Vec2 r = { a.x + b, a.y + b };
    return r;
}

inline Vec2 operator+(float a, Vec2 b)
{
    Vec2 r = { a + b.x, a + b.y };
    return r;
}

inline Vec2 operator-(Vec2 a, float b)
{
    Vec2 r = { a.x - b, a.y - b };
    return r;
}

inline Vec2 operator-(float a, Vec2 b)
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

[[nodiscard]] inline Vec3 Normalize(Vec3 v)
{
    float hyp = sqrtf(powf(v.x, 2) + powf(v.y, 2) + powf(v.z, 2));
    return { (v.x / hyp), (v.y / hyp) , (v.z / hyp)};
}
[[nodiscard]] inline Vec2 Normalize(Vec2 v)
{
    float hyp = sqrtf(powf(v.x, 2) + powf(v.y, 2));
    return { (v.x / hyp), (v.y / hyp) };
}
//inline Vec2 NormalizeZero(Vec2 v)
//{
//    float hyp = sqrtf(powf(v.x, 2) + powf(v.y, 2));
//    if (hyp == 0)
//        return {};
//	return{ (v.x / hyp), (v.y / hyp) };
//}

template <typename T>
[[nodiscard]] inline T Lerp(T a, T b, T t)
{
    return a + (b - a) * t;
}

//[[nodiscard]] inline float Lerp(float a, float b, float t)
//{
//    return a + (b - a) * t;
//}


[[nodiscard]] float Bilinear(Vec2 p, Rect loc, float bl, float br, float tl, float tr);
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
    return a.x * b.x + a.y * b.y;
}

[[nodiscard]] inline float Pythags(Vec2 a)
{
    return sqrtf(powf(a.x, 2) + powf(a.y, 2));
}

[[nodiscard]] inline float Distance(Vec2 a, Vec2 b)
{
    return Pythags(a - b);
}

[[nodiscard]] inline uint32 RandomU32(uint32 min, uint32 max)
{
    uint32 result = rand();
    result ^= result << 13;
    result ^= result >> 17;
    result ^= result << 5;
	return (result % (max - min)) + min;
}

#ifdef IMPLIMENTATION1
double Perlin(double x, double y, double z);
#endif
#ifdef IMPLIMENTATION2
float Terrain(Vec2 p);
float FBM(Vec2 x, float H = 0.5f);
#endif
#ifdef IMPLIMENTATION3
float PerlinNoise(Vec2 v);
#endif
#ifdef IMPLIMENTATION4
void PerlinInit();
float Noise(Vec2 v);
inline float Noise(Vec2d v)
{
    return Noise(Vec2({ static_cast<float>(v.x), static_cast<float>(v.y) }));
}
#endif
