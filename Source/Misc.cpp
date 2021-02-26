#include "Misc.h"
#include "SDL\include\SDL.h"
#include "Math.h"
#include "WinInterop.h"

bool g_running = true;

#if ENABLE_PROFILE
ScopeTimer::~ScopeTimer()
{
    float duration = GetTimer() - start;
    DebugPrint("%s took %f ms\n", name, duration);
}
#endif

uint64 f = SDL_GetPerformanceFrequency();
uint64 ct = SDL_GetPerformanceCounter();

float GetTimer()
{
	uint64 c = SDL_GetPerformanceCounter();
	return float(((c - ct) * 1000) / (double)f);
}
