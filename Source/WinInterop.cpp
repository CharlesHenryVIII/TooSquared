#include "WinInterop.h"
#include "Misc.h"
#include "Math.h"

#include <Windows.h>
#include <string>


static HANDLE GetFileHandle(const std::string& fileLoc)
{
    HANDLE handle = CreateFileA(fileLoc.c_str(), GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return handle;
}

bool GetFileText(std::string& result, const std::string& fileLoc)
{
    HANDLE handle = GetFileHandle(fileLoc);
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    Defer {
        CloseHandle(handle);
    };

    uint32 bytesRead;
    static_assert(sizeof(DWORD) == sizeof(uint32));
    static_assert(sizeof(LPVOID) == sizeof(void*));
    
    const uint32 fileSize = GetFileSize(handle, NULL);
    result.resize(fileSize, 0);
    if (!ReadFile(handle, (LPVOID)result.c_str(), (DWORD)fileSize, reinterpret_cast<LPDWORD>(&bytesRead), NULL))
    {
        assert(false);
        return false;
    }
    return true;
}

bool GetFileTime(uint64* result, const std::string& fileLoc)
{
    HANDLE handle = GetFileHandle(fileLoc);
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    Defer {
        CloseHandle(handle);
    };

    FILETIME creationTime;
    FILETIME lastAccessTime;
    FILETIME lastWriteTime;
    if (!GetFileTime(handle, &creationTime, &lastAccessTime, &lastWriteTime))
    {
        DebugPrint("GetFileTime failed with %d\n", GetLastError());
        return false;
    }
    ULARGE_INTEGER actualResult;
    actualResult.LowPart = lastWriteTime.dwLowDateTime;
    actualResult.HighPart = lastWriteTime.dwHighDateTime;
    *result = actualResult.QuadPart;
    return true;
}

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

