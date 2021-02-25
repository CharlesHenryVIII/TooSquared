#include "Math.h"

template<typename T>
T Random(const T& min, const T& max)
{
    return min + (max - min) * (rand() / T(RAND_MAX));
}

//static uint32 _RandomU32()
//{
//    uint32 result = rand();
//    result ^= result << 13;
//    result ^= result >> 17;
//    result ^= result << 5;
//    return result;
//}

//uint32 RandomU32(uint32 min, uint32 max)
//{
//	return (_RandomU32() % (max - min)) + min;
//}
