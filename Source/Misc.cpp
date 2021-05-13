#include "Misc.h"
#include "SDL\include\SDL.h"
#include "Math.h"
#include "WinInterop.h"

bool g_running = true;

#if ENABLE_PROFILE
int ScopeTimer::tabLevel = 0;
ScopeTimer::~ScopeTimer()
{
    float duration = GetTimer() - start;
    if (tabInUse)
        tabLevel--;
    std::string tabString(tabLevel, ' ');
    DebugPrint("%s%s took %f ms\n", tabString.c_str(), name, duration);
}
#endif

uint64 f = SDL_GetPerformanceFrequency();
uint64 ct = SDL_GetPerformanceCounter();

float GetTimer()
{
    uint64 c = SDL_GetPerformanceCounter();
    return float(((c - ct) * 1000) / (double)f);
}

uint64 GetCurrentTime()
{
    return ((SDL_GetPerformanceCounter() - ct) * 1000 * 1000 * 1000) / f; //nano seconds
}
