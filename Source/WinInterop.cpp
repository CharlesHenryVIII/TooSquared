#include "WinInterop.h"
#include "Misc.h"
#include "Math.h"

#include <Windows.h>
#include <shlobj_core.h>
#include <string>
#include <thread>

void InitializeWinInterop()
{
    char szPath[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath)))
    {
        g_gameData.m_folderPath = szPath;
        g_gameData.m_folderPath += "\\TooSquared";
        g_gameData.m_saveFolderPath = g_gameData.m_folderPath + "\\Saves\\";
        //PathAppend(szPath, TEXT("New Doc.txt"));
    }
}


File::File(char const* filename, File::Mode fileMode, bool updateFile)
{
    std::string sFileName = std::string(filename);
    Init(sFileName, fileMode, updateFile);
}

File::File(const std::string& filename, File::Mode fileMode, bool updateFile)
{
    Init(filename, fileMode, updateFile);
}

void File::GetHandle()
{
    m_handle = CreateFileA(m_filename.c_str(), m_accessType, m_shareType,
        NULL, m_openType, FILE_ATTRIBUTE_NORMAL, NULL);
}

void File::Init(const std::string& filename, File::Mode fileMode, bool createIfNotFound)
{
    m_filename = std::string(filename);
    m_accessType = GENERIC_READ;
    m_shareType  = FILE_SHARE_READ;
    m_openType   = OPEN_EXISTING;
    //m_fileAttribute = FILE_ATTRIBUTE_NORMAL;

    switch (fileMode)
    {
    case File::Mode::Read:
        m_accessType = GENERIC_READ;
        m_shareType  = FILE_SHARE_READ;
        //m_fileAttribute = FILE_ATTRIBUTE_READONLY;
        break;
    case File::Mode::Write:
        m_openType = TRUNCATE_EXISTING;
        [[fallthrough]];
    case File::Mode::Append:
        m_accessType = GENERIC_WRITE;
        m_shareType = FILE_SHARE_WRITE;
        break;
    default:
        break;
    }
    GetHandle();

    if (createIfNotFound && m_handle == INVALID_HANDLE_VALUE)
    {
        m_openType = CREATE_NEW;
        GetHandle();
    }
    m_handleIsValid = (m_handle != INVALID_HANDLE_VALUE);
    //assert(m_handleIsValid);
    auto filePointerLocation = FILE_END;

    if (m_handleIsValid)
    {
        switch (fileMode)
        {
        case File::Mode::Read:
            filePointerLocation = FILE_BEGIN;
            [[fallthrough]];
        case File::Mode::Append:
            GetText();
            GetData();
            break;
        default:
            break;
        }
    }

    DWORD newFilePointer = SetFilePointer(m_handle, 0, NULL, filePointerLocation);
}

bool File::FileDestructor()
{
    return CloseHandle(m_handle);
}

File::~File()
{   
    if (m_handleIsValid)
    {
        FileDestructor();
    }
}


void File::GetText()
{
    if (!m_handleIsValid)
        return;

    uint32 bytesRead;
    static_assert(sizeof(DWORD) == sizeof(uint32));
    static_assert(sizeof(LPVOID) == sizeof(void*));

    const uint32 fileSize = GetFileSize(m_handle, NULL);
    m_contents.resize(fileSize, 0);
    m_textIsValid = true;
    if (!ReadFile(m_handle, (LPVOID)m_contents.c_str(), (DWORD)fileSize, reinterpret_cast<LPDWORD>(&bytesRead), NULL))
    {
        //assert(false);
        m_textIsValid = false;
    }
}

void File::GetData()
{
    if (!m_handleIsValid)
        return;

    uint32 bytesRead;
    static_assert(sizeof(DWORD) == sizeof(uint32));
    static_assert(sizeof(LPVOID) == sizeof(void*));

    const uint32 fileSize = GetFileSize(m_handle, NULL);
    m_dataBinary.resize(fileSize, 0);
    m_binaryDataIsValid = true;
    if (ReadFile(m_handle, (LPVOID)m_dataBinary.data(), (DWORD)fileSize, reinterpret_cast<LPDWORD>(&bytesRead), NULL) == 0)
    {
        m_binaryDataIsValid = false;
        DWORD error = GetLastError();
    }
}
bool File::Write(void* data, size_t sizeInBytes)
{
    LPDWORD bytesWritten = {};
    BOOL result = WriteFile(m_handle, data, (DWORD)sizeInBytes, bytesWritten, NULL);
    return result != 0;
}

bool File::Write(const void* data, size_t sizeInBytes)
{
    LPDWORD bytesWritten = {};
    BOOL result = WriteFile(m_handle, data, (DWORD)sizeInBytes, bytesWritten, NULL);
    return result != 0;
}

bool File::Write(const std::string& text)
{
    LPDWORD bytesWritten = {};
    BOOL result = WriteFile(m_handle, text.c_str(), (DWORD)text.size(), bytesWritten, NULL);
    return result != 0;
    //int32 result = fputs(text.c_str(), m_handle);
    //return (!(result == EOF));
    //return false;
}

void File::GetTime()
{
    FILETIME creationTime;
    FILETIME lastAccessTime;
    FILETIME lastWriteTime;
    if (!GetFileTime(m_handle, &creationTime, &lastAccessTime, &lastWriteTime))
    {
        DebugPrint("GetFileTime failed with %d\n", GetLastError());
        m_timeIsValid = false;
    }
    else
    {
        ULARGE_INTEGER actualResult;
        actualResult.LowPart = lastWriteTime.dwLowDateTime;
        actualResult.HighPart = lastWriteTime.dwHighDateTime;
        m_time = actualResult.QuadPart;
        m_timeIsValid = true;
    }
}

bool File::Delete()
{
    if (m_handleIsValid)
    {
        FileDestructor();
        std::wstring fuckingWide(m_filename.begin(), m_filename.end());
        bool result = DeleteFile(fuckingWide.c_str());
        m_handleIsValid = false;
        return result;
    }
    return false;
}

bool CreateFolder(const std::string& folderLocation)
{
    BOOL result = CreateDirectoryA(folderLocation.c_str(), NULL);
    return !(result == 0);
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

//
// https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2017
// Usage: SetThreadName ((DWORD)-1, "MainThread");
//
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
 } THREADNAME_INFO;
#pragma pack(pop)
void SetThreadName(DWORD dwThreadID, const char* threadName) 
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try{
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER){
    }
#pragma warning(pop)
}

void SetThreadName(std::thread::native_handle_type threadID, std::string name)
{
    SetThreadName(GetThreadId(threadID), name.c_str());
}
