#pragma once
#include "SDL\include\SDL_pixels.h"
#include "gb_math.h"

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
//using uint64 = uint64_t;
//typedef uint64_t uint64;
//#define uint64_t uint64

struct Color {
    float r;
    float g;
    float b;
    float a;
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

template<typename T>
T Random(const T& min, const T& max)
{
    return min + (max - min) * (rand() / T(RAND_MAX));
}

struct Range {
    float min, max;

    float RandomInRange()
    {
        return Random<float>(min, max);
    }
    void AngleSymetric(float angle, float range)
    {
        min = angle - range / 2;
        max = angle + range / 2;
    }
};

typedef union Vec2Int {
	struct { int32 x, y; };
	int32 e[2];
}Vec2Int;


struct Rectangle {
    Vec2 botLeft;
    Vec2 topRight;

    float Width()
    {
        return topRight.x - botLeft.x;
    }

    float Height()
    {
        return topRight.y - botLeft.y;
    }

    bool Collision(Vec2Int loc)
    {
        bool result = false;
        if (loc.y > botLeft.x && loc.y < topRight.x)
            if (loc.x > botLeft.x && loc.x < topRight.x)
                result = true;
        return result;
    }
};

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



template <typename T>
T Min(T a, T b)
{
    return a < b ? a : b;
}

template <typename T>
T Max(T a, T b)
{
    return a > b ? a : b;
}

template <typename T>
T Clamp(T v, T min, T max)
{
    return Max(min, Min(max, v));
}

inline float RadToDeg(float angle)
{
    return ((angle) / (tau)) * 360;
}

inline float DegToRad(float angle)
{
    return (angle / 360 ) * (tau);
}

//inline float Vec2Distance(Vec2 A, Vec2 B)
//{
//    return sqrtf(powf(B.x - A.x, 2) + powf(B.y - A.y, 2));
//}


//inline Vec2 Normalize(Vec2 v)
//{
//    float hyp = sqrtf(powf(v.x, 2) + powf(v.y, 2));
//    return { (v.x / hyp), (v.y / hyp) };
//}
//inline Vec2 NormalizeZero(Vec2 v)
//{
//    float hyp = sqrtf(powf(v.x, 2) + powf(v.y, 2));
//    if (hyp == 0)
//        return {};
//	return{ (v.x / hyp), (v.y / hyp) };
//}

inline float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}


/*
Atan2f return value:

3pi/4      pi/2        pi/4


            O
+/-pi      /|\          0
           / \


-3pi/4    -pi/2        pi/4
*/

inline float DotProduct(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

inline float Pythags(Vec2 a)
{
    return sqrtf(powf(a.x, 2) + powf(a.y, 2));
}

inline float Distance(Vec2 a, Vec2 b)
{
    return Pythags(a - b);
}

void Swap(void* a, void* b, const int size);
Vec2 CreateRandomVec2(const Vec2& min, const Vec2& max);
Color CreateRandomColor(Color min, Color max);
Color CreateRandomColorShade(float min, float max);
float LinearToAngularVelocity(Vec2 centerOfCircle, Vec2 position, Vec2 velocity);
//gbVec3 Vec2TogbVec3(Vec2 v);
//Vec2 gbMat4ToVec2(gbMat4 m);
Vec2 RotateVec2(Vec2 v, float deg);
