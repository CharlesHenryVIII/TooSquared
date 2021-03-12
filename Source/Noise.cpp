#include "Noise.h"
#include "Math.h"

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

    return va/wt;
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
    return Voronoi(x, u, v);
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

int32 GenerateTerrainHeight(int32 min, int32 max, Vec2 input)
{
#if 1
    float vor = (VoronoiNoise(input, 1.0f, 0.5f) + 1.0f) / 2;
    NoiseParams mountainParams = {
        .numOfOctaves = 8,
        .freq = 0.2f,
        .weight = 1.0f,
        .gainFactor = 1.0f,
    };
    NoiseParams plainParams = {
        .numOfOctaves = 7,
        .freq = 0.1f,
        .weight = 1.0f,
        .gainFactor = 0.5f,
    };

    NoiseParams noiseParams = {
        .numOfOctaves   = static_cast<int32>(Lerp<float>(static_cast<float>(mountainParams.numOfOctaves),   
                                                         static_cast<float>(plainParams.numOfOctaves),   vor)),
        .freq           =         Lerp<float>(mountainParams.freq,           plainParams.freq,           vor),
        .weight         =         Lerp<float>(mountainParams.weight,         plainParams.weight,         vor),
        .gainFactor     =         Lerp<float>(mountainParams.gainFactor,     plainParams.gainFactor,     vor),
    };

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

