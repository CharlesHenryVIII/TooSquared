#include "Math.h"

#include <string>
#include <thread>

bool GetFileText(std::string& result, const std::string& fileLoc);
bool GetFileTime(uint64* result, const std::string& fileLoc);
void DebugPrint(const char* fmt, ...);
std::string ToString(const char* fmt, ...);
void SetThreadName(std::thread::native_handle_type threadID, std::string name);
