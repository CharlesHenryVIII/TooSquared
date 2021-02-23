#include <string>
#include "Math.h"

struct FileInfo {
	uint64 lastWriteTime;
    std::string text;
};

FileInfo GetFileInfo(const std::string& fileLoc);
void DebugPrint(const char* fmt, ...);
std::string ToString(const char* fmt, ...);
