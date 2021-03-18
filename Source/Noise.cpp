#include "Noise.h"
#include "Math.h"
#include "WinInterop.h"

#include "Block.h"
#define STB_PERLIN_IMPLEMENTATION
#include "STB\stb_perlin.h"

#define STB_Define
#include "STB\stb.h"

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
    return -1.0f + 2.0f * Fract(p2.x * p2.y * (p2.x + p2.y) );
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

float Voronoi(Vec2 x, float u = 1, float v = 0)
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

#if NOISETYPE == 3

static const int32 perm[] = {
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

static int32 const size = arrsize(perm);
static int32 const mask = size - 1;
float grads_x[] = { -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
                     0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f };
float grads_y[] = { -1.0f,  0.0f,  0.0f,  1.0f, -1.0f, -1.0f,
                     1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f };

float SmoothStep(float t)
{
    t = fabsf(t); // float fabsf( float arg );
    return t >= 1.0f ? 0.0f : 1.0f - ( 3.0f - 2.0f * t ) * t * t;
}

float surflet( float x, float y, float grad_x, float grad_y )
{
    return SmoothStep(x) * SmoothStep(y) * (grad_x * x + grad_y * y);
}

float PerlinNoise(Vec2 v)
{
    float result = 0.0f;
    int32 cell_x = static_cast<int32>(floorf(v.x)); // provide surflet grids
    int32 cell_y = static_cast<int32>(floorf(v.y)); // provide surflet grids
    for (int32 grid_y = cell_y; grid_y <= cell_y + 1; ++grid_y)
    {
        for (int32 grid_x = cell_x; grid_x <= cell_x + 1; ++grid_x)
        {
            // random hash
            int32 hash = perm[(perm[grid_x & mask] + grid_y) & mask];
            // grads_x[hash], grads_y[hash] provide random vector
            result += surflet(v.x - grid_x, v.y - grid_y, grads_x[hash & sizeof(grads_x)], grads_y[hash & sizeof(grads_x)]);
        }
    }
    return result;
}

#endif


void NoiseInit()
{

}

float VoronoiNoise(Vec2 x, float u, float v)
{
#if NOISETYPE == 2
    //x /= 10;
    float result = Voronoi(x, u, v);//voronoiDistance(x);
    //DebugPrint("voronoi val: %f0.5\n", result);
    return result;
    //return Voronoi(x, u, v);
#else
    static_assert(false, "No Voronoi implimentation selected");
#endif
}

float PerlinNoise(Vec2 a, NoiseParams np)
{
#if NOISETYPE == 1
    return Perlin({a.x, a.y, 0});
#elif NOISETYPE == 2
    //return Terrain(a);
    return FBM(a, np);
#elif NOISETYPE == 3
    return PerlinNoise(Vec2 v);
#else
    static_assert(false, "No noise implimentation selected");
#endif

}

#define VORONOI 2

int32 GenerateTerrainHeight(int32 min, int32 max, Vec2 input)
{

#if VORONOI == 2
    float vor = (VoronoiNoise(input / 10, 1.0f, 0.0f) + 1.0f) / 2;
    NoiseParams mountainParams = {
        .numOfOctaves = 8,
        .freq = 0.4f,
        .weight = 1.0f,
        .gainFactor = 1.0f,
    };
    NoiseParams plainParams = {
        .numOfOctaves = 8,
        .freq = 0.1f,
        .weight = 1.0f,
        .gainFactor = 0.5f,
    };
    NoiseParams noiseParams = {};

    float mountainSetpoint = 0.6f;
    float plainsSetpoint = 0.4f;

    if (vor > mountainSetpoint)
    {
        noiseParams = mountainParams;
    }
    else if (vor < plainsSetpoint)
    {
        noiseParams = plainParams;
    }
    else
    {
        //DebugPrint("B Vor: %f\n", vor);
        vor = (vor - plainsSetpoint) / (mountainSetpoint - plainsSetpoint);
        vor = 0.5f;
        //DebugPrint("A Vor: %f\n", vor);//only getting 0.566 to 1.0
        //noiseParams = {
        //.numOfOctaves = static_cast<int32>(Lerp<float>(static_cast<float>(mountainParams.numOfOctaves),
        //                                                 static_cast<float>(plainParams.numOfOctaves),   vor)),
        //.freq = Lerp<float>(mountainParams.freq,           plainParams.freq,           vor),
        //.weight = Lerp<float>(mountainParams.weight,         plainParams.weight,         vor),
        //.gainFactor = Lerp<float>(mountainParams.gainFactor,     plainParams.gainFactor,     vor),
        //};
        uint32 mountainHeight = Clamp<uint32>(static_cast<int32>(PerlinNoise(input, mountainParams) * max), min, max - 1);
        uint32 plainsHeight   = Clamp<uint32>(static_cast<int32>(PerlinNoise(input, plainParams)    * max), min, max - 1);
        return static_cast<uint32>(Lerp<float>(static_cast<float>(plainsHeight), static_cast<float>(mountainHeight), vor));
    }

    return Clamp<uint32>(static_cast<int32>(PerlinNoise(input, noiseParams) * max), min, max - 1);

#elif VORONOI == 1
    float vor = (VoronoiNoise(input / 10, 1.0f, 0.0f) + 1.0f) / 2;
    NoiseParams mountainParams = {
        .numOfOctaves = 8,
        .freq = 0.4f,
        .weight = 1.0f,
        .gainFactor = 1.0f,
    };
    NoiseParams plainParams = {
        .numOfOctaves = 8,
        .freq = 0.1f,
        .weight = 1.0f,
        .gainFactor = 0.5f,
    };
    NoiseParams noiseParams = {};

    float mountainSetpoint = 0.6f;
    float plainsSetpoint = 0.4f;

    if (vor > mountainSetpoint)
    {
        noiseParams = mountainParams;
    }
    else if (vor < plainsSetpoint)
    {
        noiseParams = plainParams;
    }
    else
    {
        //DebugPrint("B Vor: %f\n", vor);
        vor = (vor - plainsSetpoint) / (mountainSetpoint - plainsSetpoint);
        //DebugPrint("A Vor: %f\n", vor);//only getting 0.566 to 1.0
        noiseParams = {
        .numOfOctaves = static_cast<int32>(Lerp<float>(static_cast<float>(mountainParams.numOfOctaves),
                                                         static_cast<float>(plainParams.numOfOctaves),   vor)),
        .freq = Lerp<float>(mountainParams.freq,           plainParams.freq,           vor),
        .weight = Lerp<float>(mountainParams.weight,         plainParams.weight,         vor),
        .gainFactor = Lerp<float>(mountainParams.gainFactor,     plainParams.gainFactor,     vor),
        };
    }

    return Clamp<uint32>(static_cast<int32>(PerlinNoise(input, noiseParams) * max), min, max - 1);
#else
    float vor = VoronoiNoise(input, 1.0f, 0.5f);
    NoiseParams noiseParams;
    if (vor >= 0)
    {//Mountains
        noiseParams = {
            .numOfOctaves = 16,
            .freq = 0.2f,
            .weight = 1.0f,
            .gainFactor = 1.0f,
        };
    }
    else// if (vor >= 0)
    {//Plains
        noiseParams = {
            .numOfOctaves = 8,
            .freq = 0.2f,
            .weight = 1.0f,
            .gainFactor = 0.5f,
        };
    }
    return Clamp<uint32>(static_cast<int32>(PerlinNoise(input, noiseParams) * max), min, max - 1);
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

//float Noise(Vec3 a) 
//{
//#if IMPLIMENTATION == 1
//	return Perlin(a);
//#elif IMPLIMENTATION == 2
//    return float Terrain(Vec2 p);
//	float FBM(Vec2 x, float H = 0.5f);
//#elif IMPLIMENTATION == 3
//	float PerlinNoise(Vec2 v);
//#elif IMPLIMENTATION == 4
//    return Noise4(a);
//#else
//	static_assert(false, "No noise implimentation selected");
//#endif
//
//}

#if NOISETYPE == 4

float F2 = 0.5f * (sqrt(3.0f) - 1.0f);
float G2 = (3.0f - sqrt(3.0f)) / 6.0f;
float F3 = 1.0f / 3.0f;
float G3 = 1.0f / 6.0f;
float F4 = (sqrt(5.0f) - 1.0f) / 4.0f;
float G4 = (5.0f - sqrt(5.0f)) / 20.0f;

SimplexNoise* _N1 = new SimplexNoise(2);
SimplexNoise* _N2 = new SimplexNoise(3);
SimplexNoise* _N3 = new SimplexNoise(4);

//SimplexNoise* s_simplex = nullptr;
void Init()
{
    _N1 = new SimplexNoise(/*2*/);
    _N2 = new SimplexNoise(/*3*/);
    _N3 = new SimplexNoise(/*4*/);
}





const uint32 _VOXEL_HEIGHT = 128;
const uint32 _OCEAN_LEVEL = floor(_VOXEL_HEIGHT * 0.12);
const uint32 _BEACH_LEVEL = _OCEAN_LEVEL + 2;
const uint32 _SNOW_LEVEL = floor(_VOXEL_HEIGHT * 0.8);
const uint32 _MOUNTAIN_LEVEL = floor(_VOXEL_HEIGHT * 0.5);

  // HACKY TODO: Pass a terrain generation object through instead of these
  // loose functions.
  //const _N1 = new SimplexNoise(2);
  //const _N2 = new SimplexNoise(3);
  //const _N3 = new SimplexNoise(4);
float _SimplexNoise(SimplexNoise* gen, float nx, float ny) 
{
    return gen->noise(nx, ny) * 0.5 + 0.5;
}

float Noise(SimplexNoise* gen, float x, float y, uint32 scale, int32 octaves, float persistence, float exponentiation) 
{
    const float xs = x / float(scale);
    const float ys = y / float(scale);
    float amplitude = 1.0;
    float frequency = 1.0;
    float normalization = 0;
    float total = 0;
    for (int32 o = 0; o < octaves; o++) {
        total += _SimplexNoise(gen, xs * frequency, ys * frequency) * amplitude;
        normalization += amplitude;
        amplitude *= persistence;
        frequency *= 2.0;
    }
    total /= normalization;
    return powf(total, exponentiation);
}



Biome GetBiome(uint32 elevation, float moisture)
{
    if (elevation < _OCEAN_LEVEL)
        return Biome::Ocean;
    if (elevation < _BEACH_LEVEL)
        return Biome::Desert;

    if (elevation > _SNOW_LEVEL) {
        return Biome::Tundra;
    }

    if (elevation > _MOUNTAIN_LEVEL) {
        if (moisture < 0.1) {
            return Biome::Mountain;// 'stone';
        }
        else if (moisture < 0.25) {
            return Biome::Mountain;// 'hills';
        }
    }

    return Biome::Plains;
}

    //_Create(game) {
    //  const pxGeometry = new THREE.PlaneBufferGeometry(1, 1);
    //  pxGeometry.rotateY(Math.PI / 2);
    //  pxGeometry.translate(0.5, 0, 0);

    //  const nxGeometry = new THREE.PlaneBufferGeometry(1, 1);
    //  nxGeometry.rotateY(-Math.PI / 2);
    //  nxGeometry.translate(-0.5, 0, 0);

    //  const pyGeometry = new THREE.PlaneBufferGeometry(1, 1);
    //  pyGeometry.attributes.uv.array[5] = 3.0 / 4.0;
    //  pyGeometry.attributes.uv.array[7] = 3.0 / 4.0;
    //  pyGeometry.attributes.uv.array[1] = 4.0 / 4.0;
    //  pyGeometry.attributes.uv.array[3] = 4.0 / 4.0;
    //  pyGeometry.rotateX(-Math.PI / 2);
    //  pyGeometry.translate(0, 0.5, 0);

    //  const nyGeometry = new THREE.PlaneBufferGeometry(1, 1);
    //  nyGeometry.attributes.uv.array[5] = 1.0 / 4.0;
    //  nyGeometry.attributes.uv.array[7] = 1.0 / 4.0;
    //  nyGeometry.attributes.uv.array[1] = 2.0 / 4.0;
    //  nyGeometry.attributes.uv.array[3] = 2.0 / 4.0;
    //  nyGeometry.rotateX(Math.PI / 2);
    //  nyGeometry.translate(0, -0.5, 0);

    //  const pzGeometry = new THREE.PlaneBufferGeometry(1, 1);
    //  pzGeometry.translate(0, 0, 0.5);

    //  const nzGeometry = new THREE.PlaneBufferGeometry(1, 1);
    //  nzGeometry.rotateY( Math.PI );
    //  nzGeometry.translate(0, 0, -0.5);

    //  const flipGeometries = [
    //    pxGeometry, nxGeometry, pzGeometry, nzGeometry
    //  ];

    //  for (let g of flipGeometries) {
    //    g.attributes.uv.array[5] = 2.0 / 4.0;
    //    g.attributes.uv.array[7] = 2.0 / 4.0;
    //    g.attributes.uv.array[1] = 3.0 / 4.0;
    //    g.attributes.uv.array[3] = 3.0 / 4.0;
    //  }

    //  this._geometries = [
    //    pxGeometry, nxGeometry,
    //    pyGeometry, nyGeometry,
    //    pzGeometry, nzGeometry
    //  ];

    //  this._geometries = {
    //    cube: BufferGeometryUtils.mergeBufferGeometries(this._geometries),
    //    plane: pyGeometry,
    //  };
    //}

    RebuildFromCellBlock(cells) {
      const cellsOfType = {};

      for (let k in cells) {
        const c = cells[k];
        if (!(c.type in cellsOfType)) {
          cellsOfType[c.type] = [];
        }
        if (c.visible) {
          cellsOfType[c.type].push(c);
        }
      }

      for (let k in cellsOfType) {
        this._RebuildFromCellType(cellsOfType[k], k);
      }

      for (let k in this._geometryBuffers) {
        if (!(k in cellsOfType)) {
          this._RebuildFromCellType([], k);
        }
      }
    }

    _RebuildFromCellType(cells, cellType) {
      const textureInfo = this._game._atlas.Info[cellType];

      if (!(cellType in this._geometryBuffers)) {
        this._geometryBuffers[cellType] = new THREE.InstancedBufferGeometry();

        this._materials[cellType] = new THREE.RawShaderMaterial({
          uniforms: {
            diffuseTexture: {
              value: textureInfo.texture
            },
            skybox: {
              value: this._game._graphics._scene.background
            },
            fogDensity: {
              value:  0.005
            },
            cloudScale: {
              value: [1, 1, 1]
            }
          },
          vertexShader: voxels_shader.VS,
          fragmentShader: voxels_shader.PS,
          side: THREE.FrontSide
        });

        // HACKY: Need to have some sort of material manager and pass
        // these params.
        if (cellType == 'water') {
          this._materials[cellType].blending = THREE.NormalBlending;
          this._materials[cellType].depthWrite = false;
          this._materials[cellType].depthTest = true;
          this._materials[cellType].transparent = true;
        }

        if (cellType == 'cloud') {
          this._materials[cellType].uniforms.fogDensity.value = 0.001;
          this._materials[cellType].uniforms.cloudScale.value = [64, 10, 64];
        }

        this._meshes[cellType] = new THREE.Mesh(
            this._geometryBuffers[cellType], this._materials[cellType]);
        this._game._graphics._scene.add(this._meshes[cellType]);
      }

      this._geometryBuffers[cellType].maxInstancedCount = cells.length;

      const baseGeometry = this._GetBaseGeometryForCellType(cellType);

      this._geometryBuffers[cellType].setAttribute(
          'position', new THREE.Float32BufferAttribute(
              [...baseGeometry.attributes.position.array], 3));
      this._geometryBuffers[cellType].setAttribute(
          'uv', new THREE.Float32BufferAttribute(
              [...baseGeometry.attributes.uv.array], 2));
      this._geometryBuffers[cellType].setAttribute(
          'normal', new THREE.Float32BufferAttribute(
              [...baseGeometry.attributes.normal.array], 3));
      this._geometryBuffers[cellType].setIndex(
          new THREE.BufferAttribute(
              new Uint32Array([...baseGeometry.index.array]), 1));

      const offsets = [];
      const uvOffsets = [];
      const colors = [];

      const box = new THREE.Box3();

      for (let c in cells) {
        const curCell = cells[c];

        let randomLuminance = Noise(
            _N2, curCell.position[0], curCell.position[2], 16, 8, 0.6, 2) * 0.2 + 0.8;
        if (curCell.luminance !== undefined) {
          randomLuminance = curCell.luminance;
        } else if (cellType == 'cloud') {
          randomLuminance = 1;
        }

        const colour = textureInfo.colourRange[0].clone();
        colour.r *= randomLuminance;
        colour.g *= randomLuminance;
        colour.b *= randomLuminance;

        colors.push(colour.r, colour.g, colour.b);
        offsets.push(...curCell.position);
        uvOffsets.push(...textureInfo.uvOffset);
        box.expandByPoint(new THREE.Vector3(
            curCell.position[0],
            curCell.position[1],
            curCell.position[2]));
      }

      this._geometryBuffers[cellType].setAttribute(
          'color', new THREE.InstancedBufferAttribute(
              new Float32Array(colors), 3));
      this._geometryBuffers[cellType].setAttribute(
          'offset', new THREE.InstancedBufferAttribute(
              new Float32Array(offsets), 3));
      this._geometryBuffers[cellType].setAttribute(
          'uvOffset', new THREE.InstancedBufferAttribute(
              new Float32Array(uvOffsets), 2));
      this._geometryBuffers[cellType].attributes.offset.needsUpdate = true;
      this._geometryBuffers[cellType].attributes.uvOffset.uvOffset = true;
      this._geometryBuffers[cellType].attributes.color.uvOffset = true;

      this._geometryBuffers[cellType].boundingBox = box;
      this._geometryBuffers[cellType].boundingSphere = new THREE.Sphere();
      box.getBoundingSphere(this._geometryBuffers[cellType].boundingSphere);
    }

    Update() {
    }
  };

    struct BiomePiece {
        Biome b;
        uint32 height;
    };

    BiomePiece _GenerateNoise(float x, float y) 
    {
      const uint32 elevation = floor(Noise(_N1, x, y, 1024, 6, 0.4, 5.65) * 128);
      const float moisture = Noise(_N2, x, y, 512, 6, 0.5, 4);

      return { GetBiome(elevation, moisture), elevation };
    }

    _Init() {
      this._cells = {};

      for (uint32 x = 0; x < CHUNK_X; x++) 
      {
          for (uint32 y = 0; y < CHUNK_Y; y++)
          {
            for (uint32 z = 0; z < CHUNK_Z; z++)
            {
            GamePos Pos = { x + chunkP.x, 0, z + chunkP.z };
            BiomePiece bp = _GenerateNoise(Pos.p.x, Pos.p.z);

          this._cells[xPos + '.' + yOffset + '.' + zPos] = {
            position: [xPos, yOffset, zPos],
            type: atlasType,
            visible: true
          };

          if (atlasType == 'ocean') {
            this._cells[xPos + '.' + _OCEAN_LEVEL + '.' + zPos] = {
              position: [xPos, _OCEAN_LEVEL, zPos],
              type: 'water',
              visible: true
            };
          } else {
            // Possibly have to generate cliffs
            let lowestAdjacent = yOffset;
            for (let xi = -1; xi <= 1; xi++) {
              for (let zi = -1; zi <= 1; zi++) {
                const [_, otherOffset] = this._GenerateNoise(xPos + xi, zPos + zi);
                lowestAdjacent = Math.min(otherOffset, lowestAdjacent);
              }
            }

            if (lowestAdjacent < yOffset) {
              const heightDifference = yOffset - lowestAdjacent;
              for (let yi = lowestAdjacent + 1; yi < yOffset; yi++) {
                this._cells[xPos + '.' + yi + '.' + zPos] = {
                  position: [xPos, yi, zPos],
                  type: 'dirt',
                  visible: true
                };
              }
            }
          }
        }
      }
    }


    AsVoxelArray(pos, radius) {
      const x = Math.floor(pos.x);
      const y = Math.floor(pos.y);
      const z = Math.floor(pos.z);

      const voxels = [];
      for (let xi = -radius; xi <= radius; xi++) {
        for (let yi = -radius; yi <= radius; yi++) {
          for (let zi = -radius; zi <= radius; zi++) {
            const xPos = xi + x;
            const yPos = yi + y;
            const zPos = zi + z;
            const k = xPos + '.' + yPos + '.' + zPos;
            if (k in this._cells) {
              const cell = this._cells[k];
              if (!cell.visible) {
                continue;
              }

              if (cell.blinker !== undefined) {
                continue;
              }

              const position = new THREE.Vector3(
                  cell.position[0], cell.position[1], cell.position[2]);
              const half = new THREE.Vector3(0.5, 0.5, 0.5);

              const m1 = new THREE.Vector3();
              m1.copy(position);
              m1.sub(half);

              const m2 = new THREE.Vector3();
              m2.copy(position);
              m2.add(half);

              const box = new THREE.Box3(m1, m2);
              const voxelData = {...cell};
              voxelData.aabb = box;
              voxelData.key = k;
              voxels.push(voxelData);
            }
          }
        }
      }

      return voxels;
    }

    AsBox3Array(pos, radius) {
      const x = Math.floor(pos.x);
      const y = Math.floor(pos.y);
      const z = Math.floor(pos.z);

      const boxes = [];
      for (let xi = -radius; xi <= radius; xi++) {
        for (let yi = -radius; yi <= radius; yi++) {
          for (let zi = -radius; zi <= radius; zi++) {
            const xPos = xi + x;
            const yPos = yi + y;
            const zPos = zi + z;
            const k = xPos + '.' + yPos + '.' + zPos;
            if (k in this._cells) {
              const cell = this._cells[k];
              if (!cell.visible) {
                continue;
              }

              const position = new THREE.Vector3(
                  cell.position[0], cell.position[1], cell.position[2]);
              const half = new THREE.Vector3(0.5, 0.5, 0.5);

              const m1 = new THREE.Vector3();
              m1.copy(position);
              m1.sub(half);

              const m2 = new THREE.Vector3();
              m2.copy(position);
              m2.add(half);

              const box = new THREE.Box3(m1, m2);
              boxes.push(box);
            }
          }
        }
      }

      return boxes;
    }
  };

  class SparseVoxelCellManager {
    constructor(game) {
      this._game = game;
      this._cells = {};
      this._cellDimensions = new THREE.Vector3(32, 32, 32);
      this._visibleDimensions = [32, 32];
      this._dirtyBlocks = {};
      this._ids = 0;

      this._tools = [
        null,
        new voxels_tool.InsertTool(this),
        new voxels_tool.DeleteTool(this),
      ];
      this._activeTool = 0;
    }

    _Key(x, y, z) {
      return x + '.' + y + '.' + z;
    }
    MarkDirty(block) {
      this._dirtyBlocks[block.ID] = block;
    }

    _FindIntersections(ray, maxDistance) {
      const camera = this._game._graphics._camera;
      const cells = this.LookupCells(camera.position, maxDistance);
      const intersections = [];

      for (let c of cells) {
        const voxels = c.AsVoxelArray(camera.position, maxDistance);

        for (let v of voxels) {
          const intersectionPoint = new THREE.Vector3();

          if (ray.intersectBox(v.aabb, intersectionPoint)) {
            intersections.push({
                cell: c,
                voxel: v,
                intersectionPoint: intersectionPoint,
                distance: intersectionPoint.distanceTo(camera.position)
            });
          }
        }
      }

      intersections.sort((a, b) => {
        const d1 = a.intersectionPoint.distanceTo(camera.position);
        const d2 = b.intersectionPoint.distanceTo(camera.position);
        if (d1 < d2) {
          return -1;
        } else if (d2 < d1) {
          return 1;
        } else {
          return 0;
        }
      });

      return intersections;
    }

    Update(timeInSeconds) {
      if (this._tools[this._activeTool]) {
        this._tools[this._activeTool].Update(timeInSeconds);
      }

      this._UpdateDirtyBlocks();
      this._UpdateTerrain();
    }

    _UpdateDirtyBlocks() {
      for (let k in this._dirtyBlocks) {
        const b = this._dirtyBlocks[k];
        b.Build();
        delete this._dirtyBlocks[k];
        break;
      }
    }

    _UpdateTerrain() {
      const cameraPosition = this._game._graphics._camera.position;
      const cellIndex = this._CellIndex(cameraPosition.x, cameraPosition.z);

      const xs = Math.floor((this._visibleDimensions[0] - 1 ) / 2);
      const zs = Math.floor((this._visibleDimensions[1] - 1) / 2);
      let cells = {};

      for (let x = -xs; x <= xs; x++) {
        for (let z = -zs; z <= zs; z++) {
          const xi = x + cellIndex[0];
          const zi = z + cellIndex[1];

          const key = this._Key(xi, 0, zi);
          cells[key] = [xi, zi];
        }
      }

      const intersection = utils.DictIntersection(this._cells, cells);
      const difference = utils.DictDifference(cells, this._cells);
      const recycle = Object.values(utils.DictDifference(this._cells, cells));

      cells = intersection;

      for (let k in difference) {
        const [xi, zi] = difference[k];
        const offset = new THREE.Vector3(
            xi * this._cellDimensions.x, 0, zi * this._cellDimensions.z);

        let block = recycle.pop();
        if (block) {
          // TODO MAKE PUBLIC API
          block._blockOffset = offset;
          block._Init();
        } else {
          block = new voxels.SparseVoxelCellBlock(
              this._game, this, offset, this._cellDimensions, this._ids++);
        }

        this.MarkDirty(block);

        cells[k] = block;
      }

      this._cells = cells;
    }
  }

  return {
    InstancedBlocksManager: InstancedBlocksManager,
    SparseVoxelCellBlock: SparseVoxelCellBlock,
    SparseVoxelCellManager: SparseVoxelCellManager,
  };
})();
*/
#endif
