#include <string>
#include "Math.h"

bool GetFileText(std::string& result, const std::string& fileLoc);
bool GetFileTime(uint64* result, const std::string& fileLoc);
void DebugPrint(const char* fmt, ...);
std::string ToString(const char* fmt, ...);
