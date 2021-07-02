#pragma once
#include "WinInterop.h"

#include "imgui.h"

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
extern char* g_ClipboardTextData;
uint64_t GetCurrentTime();

template <typename T>
void GenericImGuiTable(const std::string& title, const std::string& fmt, T* firstValue, int32 length = 3)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(title.data());
    for (int32 column = 0; column < length; column++)
    {
        ImGui::TableSetColumnIndex(column + 1);
        std::string string = ToString(fmt.c_str(), firstValue[column]);
        ImGui::TextUnformatted(string.data());
    }
}

enum class TimeOfDay : int32 {
    Midnight,
    Morning,
    Afternoon,
    Evening,
    Count,
};
ENUMOPS(TimeOfDay);

struct GameData {
    float m_currentTime = 12.0f;
    float m_timeScale = 1.0f;
    TimeOfDay m_timeOfDay = TimeOfDay::Afternoon;
    bool m_gameTimePlaying = false;
    std::atomic<bool>  m_gameSaveAttempt = false;
    std::atomic<bool>  m_gameSavedSuccessfully = false;
    std::atomic<int32> m_gameSaveProgress  = 0;
    std::atomic<int32> m_gameSaveDataCount = 0;
    std::string m_saveFilename = "TestGame";
    std::string m_folderPath;
    std::string m_saveFolderPath;
};
extern GameData g_gameData;

std::string GetChunkSaveFilePathFromChunkPos(const ChunkPos& p);
std::string GetEntitySaveFilePathFromChunkPos(const ChunkPos& p);
