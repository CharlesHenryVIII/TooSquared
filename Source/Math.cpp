#include "Math.h"

#include <cmath>

template<typename T>
T Random(const T& min, const T& max)
{
    return min + (max - min) * (rand() / T(RAND_MAX));
}
