#include "Misc.h"
#include "Math.h"

#include <Windows.h>
#include <cstdio>

void DebugPrint(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    OutputDebugStringA(buffer);
    va_end(list);
}

std::string ToString(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
	char buffer[4096];
	int32 i = vsnprintf(buffer, arrsize(buffer), fmt, args);
	va_end(args);
	return buffer;
}

bool g_running = true;
