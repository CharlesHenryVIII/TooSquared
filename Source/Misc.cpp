#include "Misc.h"
#include "SDL\include\SDL.h"
#include "Math.h"
#include "WinInterop.h"

bool g_running = true;
char* g_ClipboardTextData = NULL;


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

std::string GetSaveFilePathFromChunkPos(const std::string subFolderDataName, const ChunkPos& p)
{
    assert(subFolderDataName[0] == '\\' && subFolderDataName[subFolderDataName.size() - 1] == '\\');
    if (subFolderDataName[0] == '\\' && subFolderDataName[subFolderDataName.size() - 1] == '\\')
        return g_gameData.m_saveFolderPath + g_gameData.m_saveFilename + subFolderDataName + ToString("%i_%i.wad", p.p.x, p.p.z);
    return {};
}

std::string GetChunkSaveFilePathFromChunkPos(const ChunkPos& p)
{
    return GetSaveFilePathFromChunkPos("\\Chunk_Data\\", p);
}

std::string GetEntitySaveFilePathFromChunkPos(const ChunkPos& p)
{
    return GetSaveFilePathFromChunkPos("\\Entity_Data\\", p);
}

GameData g_gameData = {};

