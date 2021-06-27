#pragma once
#include "Math.h"

#include <string>
#include <thread>

struct File {
    enum class FileMode {
        Read,   //Read:   Open file for read access.
        Write,  //Write:  Open and empty file for output.
        Append, //Append: Open file for output.
    };

    bool m_handleIsValid = false;
    bool m_textIsValid   = false;
    bool m_timeIsValid   = false;
    std::string m_filename;
    std::string m_contents;
    uint64 m_time = {};

    File(char const* fileName,        File::FileMode fileMode, bool updateFile);
    File(const std::string& fileName, File::FileMode fileMode, bool updateFile);
    ~File();

    bool Write(const std::string& text);
    bool Write(void* data, size_t sizeInBytes);
    void GetText();
    void GetTime();

private:
        
    void* m_handle;
    uint32  m_accessType;
    uint32  m_shareType;
    uint32  m_openType;

    void GetHandle();
    void Init(const std::string& filename, File::FileMode fileMode, bool updateFile);
};

void InitializeWinInterop();
void DebugPrint(const char* fmt, ...);
std::string ToString(const char* fmt, ...);
void SetThreadName(std::thread::native_handle_type threadID, std::string name);
bool CreateFolder(const std::string& folderLocation);
