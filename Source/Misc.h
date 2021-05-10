#pragma once
#include <type_traits>
#include <string>

#define FAIL assert(false)
#define arrsize(arr__) (sizeof(arr__) / sizeof(arr__[0]))

#define ENUMOPS(T) \
constexpr auto operator+(T a)\
{\
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


/**********************************************
 *
 * Defer
 *
 ***************/

template <typename T>
struct ExitScope
{
    T lambda;
    ExitScope(T lambda): lambda(lambda){ }
    ~ExitScope(){ lambda();}
};

struct ExitScopeHelp
{
    template<typename T>
    ExitScope<T> operator+(T t) { return t; }
};

#define _TS_CONCAT(a, b) a ## b
#define TS_CONCAT(a, b) _TS_CONCAT(a, b)

#define Defer auto TS_CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()

#define ENABLE_PROFILE 1
#if ENABLE_PROFILE
float GetTimer();
struct ScopeTimer
{
    const char* name;
    float start;
    static int tabLevel;
    int tabInUse;

    ScopeTimer(const char* name_, int _tabLevel = 0)
        : name(name_)
    {
        start = GetTimer();
        tabInUse = _tabLevel;
        if (tabInUse)
        {
            tabLevel++;
        }
    }

    ~ScopeTimer();
};

#define PROFILE_FUNCTION() ScopeTimer TS_CONCAT(__timer_, __COUNTER__)(__FUNCTION__)
#define PROFILE_SCOPE(name) ScopeTimer TS_CONCAT(__timer_, __COUNTER__)(name)
#define PROFILE_SCOPE_TAB(name) ScopeTimer TS_CONCAT(__timer_, __COUNTER__)(name, 1)

#else
#define PROFILE_FUNCTION() (void)0
#define PROFILE_SCOPE(...) (void)0
#define PROFILE_SCOPE_TAB(...) (void)0


#endif


extern bool g_running;
uint64_t GetCurrentTime();
