#include "Noise.h"
#include "Math.h"
#include "WinInterop.h"

#include "Chunk.h"
#define STB_PERLIN_IMPLEMENTATION
#include "STB\stb_perlin.h"

#define STB_Define
#include "STB\stb.h"

//#include "FastNoiseLite.h"

//____________________
//
//______PERLIN________
//
//____________________

static const uint8 permutationSet[] = {
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,140, 36,103, 30, 69,142,
      8, 99, 37,240, 21, 10, 23,190,  6,148,247,120,234, 75,  0, 26,197, 62, 94,252,219,203,
    117, 35, 11, 32, 57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175, 74,165,
     71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122, 60,211,133,230,220,105, 92, 41,
     55, 46,245, 40,244,102,143, 54, 65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89,
     18,169,200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64, 52,217,226,250,
    124,123,  5,202, 38,147,118,126,255, 82, 85,212,207,206, 59,227, 47, 16, 58, 17,182,189,
     28, 42,223,183,170,213,119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,218,246, 97,228,251, 34,
    242,193,238,210,144, 12,191,179,162,241, 81, 51,145,235,249, 14,239,107, 49,192,214, 31,
    181,199,106,157,184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,222,114,
     67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180,
};


uint8 GetRandomUint8(uint8 a)
{
    return permutationSet[a];
}

#if NOISETYPE == 1
//// Below are 4 influence values in the arrangement:
//// [g1] | [g2]
//// -----------
//// [g3] | [g4]
//int g1, g2, g3, g4;
//int u, v;   // These coordinates are the location of the input coordinate in its unit square.
//            // For example a value of (0.5,0.5) is in the exact center of its unit square.
//
//int x1 = Lerp(u, g1,g2);
//int x2 = Lerp(u, g3,h4);
//
//int average = lerp(x1,x2,v);

// Hash lookup table as defined by Ken Perlin.  This is a randomly
// arranged array of all numbers from 0-255 inclusive.
static const int32 permutation[] = {
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,140, 36,103, 30, 69,142,
      8, 99, 37,240, 21, 10, 23,190,  6,148,247,120,234, 75,  0, 26,197, 62, 94,252,219,203,
    117, 35, 11, 32, 57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175, 74,165,
     71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122, 60,211,133,230,220,105, 92, 41,
     55, 46,245, 40,244,102,143, 54, 65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89,
     18,169,200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64, 52,217,226,250,
    124,123,  5,202, 38,147,118,126,255, 82, 85,212,207,206, 59,227, 47, 16, 58, 17,182,189,
     28, 42,223,183,170,213,119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,218,246, 97,228,251, 34,
    242,193,238,210,144, 12,191,179,162,241, 81, 51,145,235,249, 14,239,107, 49,192,214, 31,
    181,199,106,157,184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,222,114,
     67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180,

    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,140, 36,103, 30, 69,142,
      8, 99, 37,240, 21, 10, 23,190,  6,148,247,120,234, 75,  0, 26,197, 62, 94,252,219,203,
    117, 35, 11, 32, 57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175, 74,165,
     71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122, 60,211,133,230,220,105, 92, 41,
     55, 46,245, 40,244,102,143, 54, 65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89,
     18,169,200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64, 52,217,226,250,
    124,123,  5,202, 38,147,118,126,255, 82, 85,212,207,206, 59,227, 47, 16, 58, 17,182,189,
     28, 42,223,183,170,213,119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,218,246, 97,228,251, 34,
    242,193,238,210,144, 12,191,179,162,241, 81, 51,145,235,249, 14,239,107, 49,192,214, 31,
    181,199,106,157,184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,222,114,
     67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180,
};

// Fade function as defined by Ken Perlin.  This eases coordinate values
// so that they will ease towards integral values.  This ends up smoothing
// the final output.
// 6t^5 - 15t^4 + 10t^3
static double Fade(double t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

#if 0
static double Grad(int hash, double x, double y, double z) {
    int h = hash & 15;                                    // Take the hashed value and take the first 4 bits of it (15 == 0b1111)
    double u = h < 8 /* 0b1000 */ ? x : y;                // If the most significant bit (MSB) of the hash is 0 then set u = x.  Otherwise y.

    double v;                                             // In Ken Perlin's original implementation this was another conditional operator (?:).  I
                                                          // expanded it for readability.

    if(h < 4 /* 0b0100 */)                                // If the first and second significant bits are 0 set v = y
        v = y;
    else if(h == 12 /* 0b1100 */ || h == 14 /* 0b1110*/)  // If the first and second significant bits are 1 set v = x
        v = x;
    else                                                  // If the first and second significant bits are not equal (0/1, 1/0) set v = z
        v = z;

    return ((h&1) == 0 ? u : -u)+((h&2) == 0 ? v : -v); // Use the last 2 bits to decide if u and v are positive or negative.  Then return their addition.
}

#else
// Source: http://riven8192.blogspot.com/2010/08/calculate-perlinnoise-twice-as-fast.html
static double Grad(int hash, double x, double y, double z)
{
    switch(hash & 0xF)
    {
        case 0x0: return  x + y;
        case 0x1: return -x + y;
        case 0x2: return  x - y;
        case 0x3: return -x - y;
        case 0x4: return  x + z;
        case 0x5: return -x + z;
        case 0x6: return  x - z;
        case 0x7: return -x - z;
        case 0x8: return  y + z;
        case 0x9: return -y + z;
        case 0xA: return  y - z;
        case 0xB: return -y - z;
        case 0xC: return  y + x;
        case 0xD: return -y + z;
        case 0xE: return  y - x;
        case 0xF: return -y - z;
        default:
        {
            assert(false);
            return 0; // never happens
        }
    }
}
#endif

int32 Inc(int32 num)
{
    num++;
    //if (repeat > 0) num %= repeat;

    return num;
}

double Perlin(Vec3 a)
{

    //// If we have any repeat on, change the coordinates to their "local" repetitions
    //if(repeat > 0) {
    //    x = x%repeat;
    //    y = y%repeat;
    //    z = z%repeat;
    //}


    // Calculate the "unit cube" that the point asked will be located in
    // The left bound is ( |_x_|,|_y_|,|_z_| ) and the right bound is that
    // plus 1.  Next we calculate the location (from 0.0 to 1.0) in that cube.
    int32 _x = (int32)a.x;
    int32 _y = (int32)a.y;
    int32 _z = (int32)a.z;

    int32 xi = _x & 255;
    int32 yi = _y & 255;
    int32 zi = _z & 255;
    double xf = a.x-_x;
    double yf = a.y-_y;
    double zf = a.z-_z;

    //HASH:
    const int32* p = permutation;
#if 0

    int32 a[8] = {};
    a[0] = p[p[p[    xi ]+    yi ]+    zi ];
    a[1] = p[p[p[    xi ]+Inc(yi)]+    zi ];
    a[2] = p[p[p[    xi ]+    yi ]+Inc(zi)];
    a[3] = p[p[p[    xi ]+Inc(yi)]+Inc(zi)];
    a[4] = p[p[p[Inc(xi)]+    yi ]+    zi ];
    a[5] = p[p[p[Inc(xi)]+Inc(yi)]+    zi ];
    a[6] = p[p[p[Inc(xi)]+    yi ]+Inc(zi)];
    a[7] = p[p[p[Inc(xi)]+Inc(yi)]+Inc(zi)];

#else

    int32 aaa, aba, aab, abb, baa, bba, bab, bbb;
    aaa = p[p[p[    xi ]+    yi ]+    zi ];
    aba = p[p[p[    xi ]+Inc(yi)]+    zi ];
    aab = p[p[p[    xi ]+    yi ]+Inc(zi)];
    abb = p[p[p[    xi ]+Inc(yi)]+Inc(zi)];
    baa = p[p[p[Inc(xi)]+    yi ]+    zi ];
    bba = p[p[p[Inc(xi)]+Inc(yi)]+    zi ];
    bab = p[p[p[Inc(xi)]+    yi ]+Inc(zi)];
    bbb = p[p[p[Inc(xi)]+Inc(yi)]+Inc(zi)];

#endif

    double u = Fade(xf);
    double v = Fade(yf);
    double w = Fade(zf);

    // The gradient function calculates the dot product between a pseudorandom
    // gradient vector and the vector from the input coordinate to the 8
    // surrounding points in its unit cube.
    // This is all then lerped together as a sort of weighted average based on the faded (u,v,w)
    // values we made earlier.
    double x1, x2, y1, y2;

#if 0

    x1 = Lerp(u,
        Grad(a[0], xf, yf, zf),
        Grad(a[4], xf - 1, yf, zf));
    x2 = Lerp(u,
        Grad(a[1], xf, yf - 1, zf),
        Grad(a[5], xf - 1, yf - 1, zf));
    y1 = Lerp(v, x1, x2);

    x1 = Lerp(u,
        Grad(a[2], xf, yf, zf - 1),
        Grad(a[6], xf - 1, yf, zf - 1));
    x2 = Lerp(u,
        Grad(a[3], xf, yf - 1, zf - 1),
        Grad(a[7], xf - 1, yf - 1, zf - 1));
    y2 = Lerp (v, x1, x2);

#else
    x1 = Lerp(u,
        Grad(aaa, xf, yf, zf),
        Grad(baa, xf - 1, yf, zf));
    x2 = Lerp(u,
        Grad(aba, xf, yf - 1, zf),
        Grad(bba, xf - 1, yf - 1, zf));
    y1 = Lerp(v, x1, x2);

    x1 = Lerp(u,
        Grad(aab, xf, yf, zf - 1),
        Grad(bab, xf - 1, yf, zf - 1));
    x2 = Lerp(u,
        Grad(abb, xf, yf - 1, zf - 1),
        Grad(bbb, xf - 1, yf - 1, zf - 1));
    y2 = Lerp(v, x1, x2);

#endif
    // For convenience we bind the result to 0 - 1 (theoretical min/max before is [-1, 1])
    return (Lerp(w, y1, y2)+1)/2;
}

#endif

#if NOISETYPE == 2


float Perlin3D(Vec3 in, Vec3Int wrap)
{
    return stb_perlin_noise3(in.x, in.y, in.z, wrap.x, wrap.y, wrap.z);
}

float Hash(Vec3 p)  // replace this by something better
{
    Vec3 p2 = 50.0f * Fract(p * 0.3183099f + Vec3({ 0.71f, 0.113f, 0.419f }));
    return -1.0f + 2.0f * Fract(p2.x * p2.y * p2.z * (p2.x + p2.y + p2.z) );
}

float Hash(Vec2 p)  // replace this by something better
{
    Vec2 p2 = 50.0f * Fract(p * 0.3183099f + Vec2({ 0.71f, 0.113f }));
    float result = -1.0f + 2.0f * Fract(p2.x * p2.y * (p2.x + p2.y));
    return result;
}

Vec2 Hash2D(Vec2 p)  // replace this by something better
{
#if 0
    return Fract(Sine(Vec2(DotProduct(p, Vec2(127.1f, 311.7f)),DotProduct(p, Vec2(269.5f, 183.3f)))) * 43758.5453f) * Vec2(0.1f, 1.0f);
#else
    Vec2 p2 = 50.0f * Fract(p * 0.3183099f + Vec2({ 0.71f, 0.113f }));
    return -1.0f + 2.0f * Fract(p2 * (p2.x + p2.y) );
#endif
}

Vec4 _Noise(Vec3 x)
 {
    Vec3 p = Floor(x);
    Vec3 w = Fract(x);

    Vec3 u = w * w * w * (w * (w * 6.0f - 15.0f) + 10.0f);
    Vec3 du = 30.0 * w * w * (w * (w - 2.0f) + 1.0f);

    float a = Hash(p + Vec3({ 0, 0, 0 }) );
    float b = Hash(p + Vec3({ 1, 0, 0 }) );
    float c = Hash(p + Vec3({ 0, 1, 0 }) );
    float d = Hash(p + Vec3({ 1, 1, 0 }) );
    float e = Hash(p + Vec3({ 0, 0, 1 }) );
    float f = Hash(p + Vec3({ 1, 0, 1 }) );
    float g = Hash(p + Vec3({ 0, 1, 1 }) );
    float h = Hash(p + Vec3({ 1, 1, 1 }) );

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    Vec3 test = { k1 + k4 * u.y + k6 * u.z + k7 * u.y * u.z,
                  k2 + k5 * u.z + k4 * u.x + k7 * u.z * u.x,
                  k3 + k6 * u.x + k5 * u.y + k7 * u.x * u.y };

    return { -1.0f + 2.0f * (k0 + k1 * u.x + k2 * u.y + k3 * u.z + k4 * u.x * u.y + k5 * u.y * u.z + k6 * u.z * u.x + k7 * u.x * u.y * u.z),
                 2.0f * du.x * test.x,
                 2.0f * du.y * test.y,
                 2.0f * du.z * test.z };
}

Vec4 _Noise(Vec2 x)
 {
    Vec2 p = Floor(x);
    Vec2 w = Fract(x);

    Vec2 u = w * w * w * (w * (w * 6.0f - 15.0f) + 10.0f);
    Vec2 du = 30.0 * w * w * (w * (w - 2.0f) + 1.0f);

    float a = Hash(p + Vec2({ 0, 0 }) );
    float b = Hash(p + Vec2({ 1, 0 }) );
    float c = Hash(p + Vec2({ 0, 1 }) );
    float d = Hash(p + Vec2({ 1, 1 }) );

    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k4 = a - b - c + d;

    Vec2 test = { k1 + k4 * u.y,
                  k2 + k4 * u.x };

    return { -1.0f + 2.0f * (k0 + k1 * u.x + k2 * u.y + k4 * u.x * u.y),
              2.0f * du.x * test.x,
              2.0f * du.y * test.y };
}

float Terrain(Vec2 p)
{
    static const Mat2 m = { 0.8f, -0.6f, 0.6f, 0.8f };
    float a = 0.0f;
    float b = 1.0f;
    Vec2  d = {};
    const int32 octaves = 16;
    for (int32 i = 0; i < octaves - 1; i++)
    {
        Vec3 n = _Noise(Vec3({ p.x, p.y, 0.0f })).xyz;
        d += { n.y, n.z };
        a += b * n.x / (1.0f + DotProduct(d, d));
        b *= 0.5f;
        p= m * p * 2.0f;
    }
    a = fabsf(a);
    const float lo = 1.0f;
    const float hi = 6.0f;
    float la = a / hi;//Lerp(lo, hi, a / hi);
    float ca = Clamp<float>(la, 0.0f, 1.0f);
    return ca;
}

//struct NoiseData {
//    int32 numOfOctaves = 8;
//    float freq = 0.1f;
//    float a = 1.0f;
//    float t = 0;
//    float gainFactor; //"H" 0.5 to 1.0 Generally 0.5
//};

float FBM(Vec2 x, NoiseParams params)
{
#if 0
    x = x;// / 10.0f;
    int32 numOfOctaves = 8;
    float freq = 0.1f;   //originally f
    float weight = 1.0f; //originally a
    Vec3 noiseSum = {};  //originally t
    float G = exp2(-H);
    for(int32 i = 0; i < numOfOctaves; i++)
    {
        noiseSum += weight * _Noise(freq * x).xyz;
        freq *= 2.0f;
        weight *= G;
    }
    noiseSum = -noiseSum;
    noiseSum /= 8;
    return Clamp(noiseSum.x * (1 + noiseSum.y) * (1 + noiseSum.z), 0.0f, 1.0f);
#else
    x = x;// / 10.0f;
    //int32 numOfOctaves = 8;
    float freq = params.freq;//0.2f;   //originally f
    float weight = params.weight; //1.0f; //originally a
    float noiseSum = 0;  //originally t
    float G = exp2(-params.gainFactor);
    for(int32 i = 0; i < params.numOfOctaves; i++)
    {
        noiseSum += weight * _Noise(freq * x).x;
        freq *= 2.0f;
        weight *= G;
    }
    noiseSum = -noiseSum;
    noiseSum /= 8;
    return Clamp(noiseSum, 0.0f, 1.0f);
#endif
}

float smoothstep(float edge0, float edge1, float x)
{
    x = Clamp<float>((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3 - 2 * x);
}

float Voronoi_Distance(Vec2 x, float u = 1, float v = 0)
{
    //u = Grid Control
    //v = Metric Controler
    Vec2 p = { floorf(x.x), floorf(x.y) };
    Vec2 f = Fract(x);

    float k = 1.0f + 63.0f * powf(1.0f - v, 4.0f);
    float va = 0.0f;
    float wt = 0.0f;
    for (int32 j = -2; j <= 2; j++)
    {
        for (int32 i = -2; i <= 2; i++)
        {
            Vec2  g = Vec2(float(i), float(j));
            Vec3  o = Hash(p + g) * Vec3(u, u, 1.0f);
            Vec2  r = g - f + o.xy;
            float d = DotProduct(r, r);
            float w = pow(1.0f - smoothstep(0.0f, 1.414f, sqrt(d)), k);
            va += w * o.z;
            wt += w;
        }
    }
    float result = va / wt;//-1 to 1
    return (result + 1) / 2;
}

Vec2 Random2F(Vec2 p) 
{
    Vec2 dotResult = { DotProduct(p,Vec2(127.1f, 311.7f)),DotProduct(p,Vec2(269.5f, 183.3f)) };
    return Fract(Sine(dotResult) * 43758.5453f);
}

float voronoiDistance(Vec2 x )
{
    Vec2Int p = { static_cast<int32>(floorf(x.x)), static_cast<int32>(floorf(x.y)) };
    Vec2  f = Fract( x );

    Vec2Int mb;
    Vec2 mr;

    float res = 8.0;
    for (int j = -1; j <= 1; j++)
    {
        for (int i = -1; i <= 1; i++)
        {
            Vec2Int b = Vec2Int(i, j);
            Vec2  r = Vec2({ float(b.x), float(b.y) }) + Random2F(Vec2({ float(p.x + b.x), float(p.y + b.y) })) - f;
            float d = DotProduct(r, r);

            if (d < res)
            {
                res = d;
                mr = r;
                mb = b;
            }
        }
    }

    res = 8.0;
    for (int j = -2; j <= 2; j++)
    {
        for (int i = -2; i <= 2; i++)
        {
            Vec2Int b = mb + Vec2Int({ i, j });
            Vec2  r = Vec2({ float(b.x), float(b.y) }) + Random2F(Vec2({ float(p.x + b.x), float(p.y + b.y) })) - f;
            float d = DotProduct(0.5 * (mr + r), Normalize(r - mr));

            res = Min(res, d);
        }
    }

    return res;
}



#endif


Vec2 HashTest(Vec2 in)
{
    Vec2 result;
    Vec2Int temp = { *(int32*)(&in.x), *(int32*)(&in.y) };
    //result.x = ((temp.x ^ (temp.x >> 22)) >> (22 + (temp.x >> 61)));
    //result.y = ((temp.y ^ (temp.y >> 22)) >> (22 + (temp.y >> 61)));
    //return result;

    temp.x ^= temp.x << 13;
    temp.x ^= temp.x >> 17;
    temp.x ^= temp.x << 5;

    temp.y ^= temp.y << 13;
    temp.y ^= temp.y >> 17;
    temp.y ^= temp.y << 5;

    result = { *(float*)(&temp.x), *(float*)(&temp.y) };
    return result;
}

Vec3 Voronoi_DistanceAndPositionOfCenter(Vec2 v)
{
    Vec2 n = Floor(v);
    Vec2 f = Fract(v);

    //----------------------------------
    // first pass: regular voronoi
    //----------------------------------
    Vec2 mg, center;

    float md = inf;//8.0;
    const int32 searchSize = 1;
    for (int32 j = -searchSize; j <= searchSize; j++)
    {
        for (int32 i = -searchSize; i <= searchSize; i++)
        {
            Vec2 g = Vec2(float(i), float(j));
            Vec2 o = Hash2D(n + g);
            //Vec2 o = HashTest(n + g);
            //DebugPrint("o: %+f\n", o);
            Vec2 r = g + o - f;
            float d = DotProduct(r, r);

            if (d < md)
            {
                md = d;
                center = r;
                mg = g;
            }
        }
    }
    
    //----------------------------------
    // second pass: distance to borders
    //----------------------------------
    md = inf;//8.0;
    for (int32 j = -2; j <= 2; j++)
    {
        for (int32 i = -2; i <= 2; i++)
        {
            Vec2 g = mg + Vec2(float(i), float(j));
            Vec2 o = Hash2D(n + g);
            //Vec2 o = HashTest(n + g);
            Vec2 r = g + o - f;

            if (DotProduct(center - r, center - r) > 0.00001)
                md = Min(md, DotProduct(0.5 * (center + r), Normalize(r - center)));
        }
    }

    return { center.x, center.y, md };
}


Texture* s_randomNoise = nullptr;
Vec3 RandomNoiseFromImage(Vec2 a)
{
    Vec2Int aTransformed = Abs(Vec2Int({ int32(a.x * 8.0f), int32(a.y * 8.0f) }));
    //Vec2Int pixelLocation = aTransformed % s_randomNoise->m_size;

    //uint32 pixelIndex = (pixelLocation.y * s_randomNoise->m_size.x + pixelLocation.x) % 
    //                    s_randomNoise->m_size.x * s_randomNoise->m_size.y;
    uint32 pixelIndex = (aTransformed.y * s_randomNoise->m_size.x + aTransformed.x) % 
                        s_randomNoise->m_size.x * s_randomNoise->m_size.y;
    Vec3 result = {};
    result.x = s_randomNoise->m_data[pixelIndex + 0] / float(UCHAR_MAX);
    result.y = s_randomNoise->m_data[pixelIndex + 1] / float(UCHAR_MAX);
    result.z = s_randomNoise->m_data[pixelIndex + 2] / float(UCHAR_MAX);

    return result;
}

Vec3 Voronoi_DistanceAndPositionOfCenter2(Vec2 v)
{
    Vec2 n = Floor(v);
    Vec2 f = Fract(v);

	float id, le;

    float md = inf;// 10.0f;
    
    for(int32 j =- 1; j <= 1; j++)
    { 
        for (int32 i = -1; i <= 1; i++)
        {
            Vec2 g1 = n + Vec2(float(i), float(j));
            Vec3 rr = RandomNoiseFromImage(g1);
            Vec2 o = g1 + rr.xy;
            Vec2 r = v - o;
            float d = DotProduct(r, r);
            float z = rr.z;

            if (d < md)
            {
                md = d;
                id = z + g1.x + g1.y * 7.0f;
                le = 0.0f;
            }
        }
    }

    return Vec3(md, le, id);
}

//void mainImage(out vec4 fragColor, in vec2 fragCoord)
//{
//    vec2 p = fragCoord.xy / iResolution.x;
//    p += 0.5 * cos(0.1 * iTime + vec2(0.0, 1.57));
//
//    const float scale = 8.0;
//
//    vec3 c = voronoi(scale * p);
//
//    vec3 col = 0.6 + 0.4 * cos(c.y * 0.6 + vec3(0.0, 0.9, 1.5));
//    col *= 0.96 + 0.04 * sin(10.0 * c.z);
//    col *= smoothstep(0.008, 0.015, sqrt(c.x));
//    vec2 e = vec2(2.0, 0.0) / iResolution.x;
//    vec3 ca = voronoi(scale * (p + e.xy));
//    vec3 cb = voronoi(scale * (p + e.yx));
//    col *= 1.0 - clamp(abs(2.0 * c.z - ca.z - cb.z) * 1000.0, 0.0, 1.0);
//
//    col = rand3(c.yz);
//}

void NoiseInit()
{
    s_randomNoise = new Texture("Assets/noise_texture_0002.png", GL_RGBA);
}

Vec3 Voronoi_DAndP(Vec2 v)
{
#if 0
    return Voronoi_DistanceAndPositionOfCenter2(v);
#else
    return Voronoi_DistanceAndPositionOfCenter(v);
#endif
}


float VoronoiNoise(Vec2 x, float u, float v)
{
#if NOISETYPE == 2
    //x /= 10;
    float result = Voronoi_Distance(x, u, v);//voronoiDistance(x);
    //DebugPrint("voronoi val: %f0.5\n", result);
    return result;
    //return Voronoi(x, u, v);
#else
    static_assert(false, "No Voronoi implimentation selected");
#endif
}

float Perlin2D(Vec2 a, NoiseParams np)
{
#if NOISETYPE == 1
    return Perlin({a.x, a.y, 0});
#elif NOISETYPE == 2
    return FBM(a, np);
#elif NOISETYPE == 3
    return PerlinNoise(Vec2 v);
#else
    static_assert(false, "No noise implimentation selected");
#endif

}

int stb_BigNoise(int32 x, int32 y, int32 s, uint32 seed)
{
   int r0,r1,r00,r01,r10,r11,x0,y0,x1,y1,r;
   int bx0,by0,bx1,by1;
   //int temp = (seed=0);
   int seed1 = seed & 255;
   int seed2 = (seed >> 4) & 255;
   int seed3 = (seed >> 8) & 255;
   int seed4 = (seed >> 12) & 255;
   int ix = x >> s;
   int iy = y >> s;
   int m = (1 << s)-1;
   x &= m;
   y &= m;
   x0 = (ix & 255);
   y0 = (iy & 255);

   x1 = ix+1;
   y1 = iy+1;
   bx0 = (ix >> 8) & 255;
   by0 = (iy >> 8) & 255;
   bx1 = (x1 >> 8) & 255;
   by1 = (y1 >> 8) & 255;
   x1 &= 255;
   y1 &= 255;

   // it would be "more random" to run these throught the permutation table, but this should be ok
   x0 ^= seed1;
   x1 ^= seed1;
   bx0 ^= seed2;
   bx1 ^= seed2;
   y0 ^= seed3;
   y1 ^= seed3;
   by0 ^= seed4;
   by1 ^= seed4;

   r0 = stb__perlin_randtab[x0];
   r1 = stb__perlin_randtab[x1];
   r0 = stb__perlin_randtab[r0+bx0];
   r1 = stb__perlin_randtab[r1+bx1];

   r00 = stb__perlin_randtab[r0 + y0];
   r01 = stb__perlin_randtab[r0 + y1];
   r10 = stb__perlin_randtab[r1 + y0];
   r11 = stb__perlin_randtab[r1 + y1];

   r00 = stb__perlin_randtab[r00 + by0];
   r01 = stb__perlin_randtab[r01 + by1];
   r10 = stb__perlin_randtab[r10 + by0];
   r11 = stb__perlin_randtab[r11 + by1];

   // weight factor is from 0..2^10-1
   if (s <= 10) {
      x <<= (10-s);
      y <<= (10-s);
   } else {
      x >>= (s-10);
      y >>= (s-10);
   }

   r0 = (r00<<10) + (r01-r00)*y;
   r1 = (r10<<10) + (r11-r10)*y;

   r  = (r0 <<10) + (r1 -r0 )*x;

   return r>>12;
}
int32 BigNoise(Vec2Int p, int32 s, uint32 seed)
{
    return stb_BigNoise(p.x, p.y, s, seed);
}

/*
public class XXHash : HashFunction {
	private uint seed;
	
	const uint PRIME32_1 = 2654435761U;
	const uint PRIME32_2 = 2246822519U;
	const uint PRIME32_3 = 3266489917U;
	const uint PRIME32_4 = 668265263U;
	const uint PRIME32_5 = 374761393U;
	
	public XXHash (int seed) {
		this.seed = (uint)seed;
	}
	
	public uint GetHash (byte[] buf) {
		uint h32;
		int index = 0;
		int len = buf.Length;
		
		if (len >= 16) {
			int limit = len - 16;
			uint v1 = seed + PRIME32_1 + PRIME32_2;
			uint v2 = seed + PRIME32_2;
			uint v3 = seed + 0;
			uint v4 = seed - PRIME32_1;
			
			do {
				v1 = CalcSubHash (v1, buf, index);
				index += 4;
				v2 = CalcSubHash (v2, buf, index);
				index += 4;
				v3 = CalcSubHash (v3, buf, index);
				index += 4;
				v4 = CalcSubHash (v4, buf, index);
				index += 4;
			} while (index <= limit);
			
			h32 = RotateLeft (v1, 1) + RotateLeft (v2, 7) + RotateLeft (v3, 12) + RotateLeft (v4, 18);
		}
		else {
			h32 = seed + PRIME32_5;
		}
		
		h32 += (uint)len;
		
		while (index <= len - 4) {
			h32 += BitConverter.ToUInt32 (buf, index) * PRIME32_3;
			h32 = RotateLeft (h32, 17) * PRIME32_4;
			index += 4;
		}
		
		while (index<len) {
			h32 += buf[index] * PRIME32_5;
			h32 = RotateLeft (h32, 11) * PRIME32_1;
			index++;
		}
		
		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;
		
		return h32;
	}
	
	public uint GetHash (params uint[] buf) {
		uint h32;
		int index = 0;
		int len = buf.Length;
		
		if (len >= 4) {
			int limit = len - 4;
			uint v1 = seed + PRIME32_1 + PRIME32_2;
			uint v2 = seed + PRIME32_2;
			uint v3 = seed + 0;
			uint v4 = seed - PRIME32_1;
			
			do {
				v1 = CalcSubHash (v1, buf[index]);
				index++;
				v2 = CalcSubHash (v2, buf[index]);
				index++;
				v3 = CalcSubHash (v3, buf[index]);
				index++;
				v4 = CalcSubHash (v4, buf[index]);
				index++;
			} while (index <= limit);
			
			h32 = RotateLeft (v1, 1) + RotateLeft (v2, 7) + RotateLeft (v3, 12) + RotateLeft (v4, 18);
		}
		else {
			h32 = seed + PRIME32_5;
		}
		
		h32 += (uint)len * 4;
		
		while (index < len) {
			h32 += buf[index] * PRIME32_3;
			h32 = RotateLeft (h32, 17) * PRIME32_4;
			index++;
		}
		
		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;
		
		return h32;
	}
	
	public override uint GetHash (params int[] buf) {
		uint h32;
		int index = 0;
		int len = buf.Length;
		
		if (len >= 4) {
			int limit = len - 4;
			uint v1 = (uint)seed + PRIME32_1 + PRIME32_2;
			uint v2 = (uint)seed + PRIME32_2;
			uint v3 = (uint)seed + 0;
			uint v4 = (uint)seed - PRIME32_1;
			
			do {
				v1 = CalcSubHash (v1, (uint)buf[index]);
				index++;
				v2 = CalcSubHash (v2, (uint)buf[index]);
				index++;
				v3 = CalcSubHash (v3, (uint)buf[index]);
				index++;
				v4 = CalcSubHash (v4, (uint)buf[index]);
				index++;
			} while (index <= limit);
			
			h32 = RotateLeft (v1, 1) + RotateLeft (v2, 7) + RotateLeft (v3, 12) + RotateLeft (v4, 18);
		}
		else {
			h32 = (uint)seed + PRIME32_5;
		}
		
		h32 += (uint)len * 4;
		
		while (index < len) {
			h32 += (uint)buf[index] * PRIME32_3;
			h32 = RotateLeft (h32, 17) * PRIME32_4;
			index++;
		}
		
		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;
		
		return h32;
	}
	
	public override uint GetHash (int buf) {
		uint h32 = (uint)seed + PRIME32_5;
		h32 += 4U;
		h32 += (uint)buf * PRIME32_3;
		h32 = RotateLeft (h32, 17) * PRIME32_4;
		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;
		return h32;
	}
	
	private static uint CalcSubHash (uint value, byte[] buf, int index) {
		uint read_value = BitConverter.ToUInt32 (buf, index);
		value += read_value * PRIME32_2;
		value = RotateLeft (value, 13);
		value *= PRIME32_1;
		return value;
	}
	
	private static uint CalcSubHash (uint value, uint read_value) {
		value += read_value * PRIME32_2;
		value = RotateLeft (value, 13);
		value *= PRIME32_1;
		return value;
	}
	
	private static uint RotateLeft (uint value, int count) {
		return (value << count) | (value >> (32 - count));
	}
}
*/


const uint PRIME32_1 = 2654435761U;
const uint PRIME32_2 = 2246822519U;
const uint PRIME32_3 = 3266489917U;
const uint PRIME32_4 = 668265263U;
const uint PRIME32_5 = 374761393U;

uint32 RotateLeft(uint32 value, int32 count)
{
    return (value << count) | (value >> (32 - count));
}
	
uint32 XXSeedHash(uint64 seed, int32 buf)
{
    uint32 h32 = (uint32)seed + PRIME32_5;
    h32 += 4U;
    h32 += (uint32)buf * PRIME32_3;
    h32 = RotateLeft(h32, 17) * PRIME32_4;
    h32 ^= h32 >> 15;
    h32 *= PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= PRIME32_3;
    h32 ^= h32 >> 16;
    return h32;
}

uint32 XXSeedHash(uint64 seed, int64 buf)
{
		uint32 h32;
		int32 index = 0;

        h32 = (uint32)seed + PRIME32_5;
		h32 += (uint32)2 * 4;
		
        h32 += (uint32)((buf >> 32) & 0xFFFFFFFF) * PRIME32_3;
        h32 = RotateLeft(h32, 17) * PRIME32_4;
        h32 += (uint32)(buf & 0xFFFFFFFF) * PRIME32_3;
        h32 = RotateLeft(h32, 17) * PRIME32_4;

		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;
		
		return h32;
}











//
//VORONOI CODE: 
//

Vec2Int vornoiEdges[+VoronoiEdges::Count] = {
{ 0, 0 },
{ 0, 1 },
{ 1, 1 },
{ 1, 0 },
};





void VoronoiCell::BuildCell(GamePos* corners, GamePos cellCenter)
{
    m_center = cellCenter;
    for (int32 i = 0; i < arrsize(m_lines); i++)
    {
        m_lines[i].p0 = { float(corners[i].p.x),                        float(corners[i].p.z) };
        m_lines[i].p1 = { float(corners[(i + 1) % arrsize(m_lines)].p.x), float(corners[(i + 1) % arrsize(m_lines)].p.z) };
    }
}

[[nodiscard]] bool VoronoiCell::Contains(GamePos blockP)
{
    for (int32 i = 0; i < arrsize(m_lines); i++)
    {
        Vec2 normal = m_lines[i].Normal();
        Vec2 point = { float(blockP.p.x), float(blockP.p.z) };
        point -= m_lines[i].p0;
        if (DotProduct(normal, point) > 0)
            return false;
    }
    return true;
}

[[nodiscard]] std::vector<float> VoronoiCell::DistanceToEachLineSegment(const GamePos& blockP)
{
    std::vector<float> distances;
    for (int32 i = 0; i < arrsize(m_lines); i++)
    {
        Vec2 lineDest = m_lines[i].p1 - m_lines[i].p0;
        Vec2 point = Vec2({ float(blockP.p.x), float(blockP.p.z) }) - m_lines[i].p0;

        //https://mathinsight.org/dot_product
        //This was derived from the code in the #else and was found that we could remove the sqrtf() call
        //TODO: THIS NEEDS TO BE TESTED
        FAIL;
        //__asm {
            //int 0x03
        //}
        float pythagsInner = lineDest.x * lineDest.x + lineDest.y * lineDest.y;
        float distanceRatioAlongLine = DotProduct(lineDest, point) / pythagsInner;

        //first implementation
        float lineDistance = Distance({}, lineDest);
        float distanceAlongLine = DotProduct(lineDest, point) / lineDistance;
        float distanceRatioAlongLine2 = distanceAlongLine / lineDistance;
        
        //test both implementations
        assert(distanceRatioAlongLine == distanceRatioAlongLine2);

        Vec2 pointOnLine = Lerp<Vec2>(Vec2({}), lineDest, distanceRatioAlongLine);
        //Vec2 hi = { Max(0.0f, lineDest.x), Max(0.0f, lineDest.y) };
        //Vec2 lo = { Min(0.0f, lineDest.x), Min(0.0f, lineDest.y) };
        //pointOnLine.x = Clamp<float>(pointOnLine.x, lo.x, hi.x);
        //pointOnLine.y = Clamp<float>(pointOnLine.y, lo.y, hi.y);

        distances.push_back(Distance(pointOnLine, point));
    }
    return distances;
}

[[nodiscard]] uint32 VoronoiCell::GetHash(uint64 worldSeed)
{
    uint32 xHash = XXSeedHash(worldSeed, m_center.p.x);
    uint32 zHash = XXSeedHash(worldSeed, m_center.p.z);
    union TempMerge {
        struct {
            uint32 x;
            uint32 y;
        };
        uint64 big;
    } a;
    a.x = xHash;
    a.y = zHash;

    uint32 result = PCG_Random(a.big);
    return result;
}

[[nodiscard]] VoronoiCell* VoronoiRegion::GetCell(GamePos p)
{
    for (VoronoiCell& cell : cells)
    {
        if (cell.Contains(p))
            return &cell;
    }
    return nullptr;
}

[[nodiscard]] ChunkPos VoronoiRegion::CellToChunk(Vec2Int cellP)
{
    ChunkPos result = { cellP.x * m_cellSize, 0, cellP.y * m_cellSize };
    return result;
}

[[nodiscard]] GamePos VoronoiRegion::CellToGame(Vec2Int cellP)
{
    GamePos result = ToGame(ChunkPos({ cellP.x * m_cellSize, 0, cellP.y * m_cellSize }));
    return result;
}

[[nodiscard]] Vec2Int VoronoiRegion::ToCell(ChunkPos p)
{
    Vec2Int result = Vec2ToVec2Int(Floor(Vec2({ float(p.p.x) , float(p.p.z) }) / float(m_cellSize)));
    return result;
}

[[nodiscard]] Vec2Int VoronoiRegion::ToCell(GamePos p)
{
    ChunkPos chunkPos = ToChunk(p);
    return ToCell(chunkPos);
}

//VoronoiCell* VoronoiRegion::GetNeighbors(GamePos blockP)
//{
//    VoronoiCell* thisCell = nullptr;
//    VoronoiCell results[+VoronoiEdges::Count] = {};
//    for (VoronoiCell& cell : cells)
//    {
//        if (cell.Contains(blockP))
//            thisCell = &cell;
//    }
//
//    if (thisCell)
//    {
//        GamePos neighborChunkOffsets[] = {
//            CellToGame(Vec2Int({ -1,  0 })),
//            CellToGame(Vec2Int({  0,  1 })),
//            CellToGame(Vec2Int({  1,  0 })),
//            CellToGame(Vec2Int({  0, -1 })),
//        };
//
//        for (int32 i = 0; i < +VoronoiEdges::Count; i++)
//        {
//
//            for (VoronoiCell& cell : cells)
//            {
//                if (cell.m_center.p == (thisCell->m_center.p + neighborChunkOffsets[i].p))
//                    results[i] = cell;
//            }
//        }
//        return results;
//    }
//    else
//        return nullptr;
//}
//
VoronoiCell* VoronoiRegion::HuntForBlock(VoronoiCell* baseCell, const Vec2& basePos, const Vec2& _direction)
{
    Vec2 direction = Normalize(_direction);

    Vec2 checkLocation = basePos;
    GamePos gameCheckLocation = {};
    VoronoiCell* checkCell = nullptr;
    int32 j = 0;
    do {
        checkLocation = checkLocation + direction;
        gameCheckLocation = GamePos({ int32(floorf(checkLocation.x + 0.5f)), 0, int32(floorf(checkLocation.y + 0.5f)) });
        checkCell = GetCell(gameCheckLocation);
        if (j++ >= 50)
        {
            assert(false);
            break;
        }
    } while (checkCell->m_center.p == baseCell->m_center.p);

    return checkCell;
}

//TODO: Improve
std::vector<VoronoiResult> VoronoiRegion::GetVoronoiDistancesAndNeighbors(VoronoiCell* blockCell, const GamePos& blockP)
{
    std::vector<float> distances = blockCell->DistanceToEachLineSegment(blockP);
    std::vector<VoronoiResult> results;
    for (int32 i = 0; i < distances.size(); i++)
    {
        //Get cell over line

        Vec2 halfwayPoint = blockCell->m_lines[i].p0 + ((blockCell->m_lines[i].p1 - blockCell->m_lines[i].p0) / 2);
        VoronoiCell* checkCell = HuntForBlock(blockCell, halfwayPoint, blockCell->m_lines[i].Normal());
        VoronoiResult result = {};
        result.cell = *checkCell;
        result.distance = distances[i];
        results.push_back(result);


        //Get cell over point
        result = {};


        //Get Distance to point
        Vec2 cornerLoc = blockCell->m_lines[i].p1;
        //GamePos pointLoc = { uint32(_pointLoc.x), 0, uint32(_pointLoc.y) };
        Vec2 blockLoc = { float(blockP.p.x), float(blockP.p.z) };
        result.distance = Distance(blockLoc, cornerLoc);

        //find the cell over the point
        Vec2 cellCenter = { float(blockCell->m_center.p.x), float(blockCell->m_center.p.z) };
        Vec2 _toCorner = cornerLoc - cellCenter;
        //Vec2 toCorner = Normalize(_toCorner);
        result.cell = *HuntForBlock(blockCell, cornerLoc, _toCorner);
        results.push_back(result);


    }
    return results;

}

void VoronoiRegion::BuildRegion(ChunkPos chunkP, uint64 worldSeed)
{
    int32 currentRegionGatherSize = 2;
    Vec2Int cell_chunkP = ToCell(chunkP);
    bool DEBUGTEST_firstTimeThrough = true;

    Debug_referenceChunkP = chunkP;
    Debug_referenceCellP = cell_chunkP;
    for (int32 cell_x = -currentRegionGatherSize; cell_x <= currentRegionGatherSize; cell_x++)
    {
        for (int32 cell_y = -currentRegionGatherSize; cell_y <= currentRegionGatherSize; cell_y++)
        {
            GamePos cellCorners[4] = {};
            GamePos cellCenter = {};
            Vec2Int cell_basePoint = cell_chunkP + Vec2Int({ cell_x, cell_y });

            //Create corner location
            for (int32 i = 0; i < +VoronoiEdges::Count; i++)
            {
                cellCorners[i] = CellToGame(cell_basePoint + vornoiEdges[i]);
            }

            //Create center point
            cellCenter = ToGame(ChunkPos(CellToChunk(cell_basePoint).p + m_cellSize / 2));
            cellCenter.p.y = 0;

            //Jitter corners
            //create an offset that is between 1 and 1 less than half the size of the cell
            //this makes sure we dont have to check further cells
            float cellOffsetSizeInChunks = Max(m_cellSize / 2.0f - 1.0f, 1.0f);

            for (int32 i = 0; i < +VoronoiEdges::Count; i++)
            {
                const Vec2Int& cell_corner = ToCell(cellCorners[i]);
                int64 z_positionSeed = PositionHash({ cell_corner.x, 0, cell_corner.y });
                uint32 x_positionSeed = PCG_Random(z_positionSeed);

                Vec3 offsetRatio = {};
                ChunkPos offsetInChunks = {};
                Vec2Int loc = {};

                offsetRatio.x = Clamp((XXSeedHash(x_positionSeed + worldSeed, cell_corner.x) & 0xFFFF) / float(0xFFFF), -1.0f, 1.0f);
                offsetRatio.z = Clamp((XXSeedHash(z_positionSeed + worldSeed, cell_corner.y) & 0xFFFF) / float(0xFFFF), -1.0f, 1.0f);
                offsetInChunks = ChunkPos(Vec3ToVec3Int(offsetRatio * cellOffsetSizeInChunks));

                cellCorners[i].p += ToGame(offsetInChunks).p;

#if 0
                if (DEBUGTEST_firstTimeThrough)
                {
                    WorldPos blockLoc = ToWorld(cellCorners[i]);
                    blockLoc.p.y = 240.0f;
                    cubesToDraw.push_back(blockLoc);
                    DEBUGTEST_firstTimeThrough = false;
                }
#endif
            }



            VoronoiCell cell;
            cell.BuildCell(cellCorners, cellCenter);
            cells.push_back(cell);
        }
    }
}
