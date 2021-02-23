#include <string>
#include "Math.h"

struct FileInfo {
	uint64 lastWriteTime;
    std::string text;
};

FileInfo GetFileInfo(const std::string& fileLoc);
