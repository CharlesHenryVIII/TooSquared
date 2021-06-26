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

#define ENABLE_PROFILE 0
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

class File {
    FILE* m_handle;

public:
    bool m_isValid = false;

    enum class FileMode : char {
        Read   = 'r',  // read: Open file for input operations.The file must exist.
        Write  = 'w',  // write : Create an empty file for output operations.If a file with the same name already exists, its contents are discardedand the file is treated as a new empty file.
        Append = 'a',  // append : Open file for output at the end of a file.Output operations always write data at the end of the file, expanding it.Repositioning operations(fseek, fsetpos, rewind) are ignored.The file is created if it does not exist.
    };
        
    File(char const* fileName, File::FileMode fileMode, bool updateFile);
    ~File();

    bool Write(const std::string& text)
    {
        int32 result = fputs(text.c_str(), m_handle);
        return (!(result == EOF));
    }
};

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
    std::atomic<bool>  m_gameSaved = false;
    std::atomic<bool>  m_gameSavedSuccessfully = false;
    std::atomic<int32> m_gameSaveProgress  = 0;
    std::atomic<int32> m_gameSaveDataCount = 0;
};
extern GameData g_gameData;

ENUMOPS(File::FileMode);
