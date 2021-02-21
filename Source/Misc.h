#pragma once

#define FAIL assert(false);
#define arrsize(arr__) (sizeof(arr__) / sizeof(arr__[0]))

#define ENUMOPS(T) \
constexpr auto operator+(T a)\
{                       \
    return static_cast<typename std::underlying_type<T>::type>(a);\
}

#define AssertOnce(__cond)                      \
do {                                            \
    static bool did_assert = false;             \
    if (!did_assert && (__cond) == false)       \
    {                                           \
        did_assert = true;                      \
        assert(!#__cond);                       \
    }                                           \
} while(0)

void DebugPrint(const char* fmt, ...);
