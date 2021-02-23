#include "WinInterop.h"
#include "Misc.h"
#include "Math.h"

#include <Windows.h>
#include <string>

FileInfo GetFileInfo(const std::string& fileLoc)
{
	FileInfo result;
	HANDLE bufferHandle = CreateFileA(fileLoc.c_str(), GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	{
		if (bufferHandle == INVALID_HANDLE_VALUE)
			FAIL;
	}//HANDLE should be valid

	uint32 bytesRead;
	static_assert(sizeof(DWORD) == sizeof(uint32));
	static_assert(sizeof(LPVOID) == sizeof(void*));
	
	const uint32 fileSize = GetFileSize(bufferHandle, NULL);
	result.text.resize(fileSize, 0);
	if (!ReadFile(bufferHandle, (LPVOID)result.text.c_str(), (DWORD)fileSize, reinterpret_cast<LPDWORD>(&bytesRead), NULL))
	{
		DebugPrint("Read File failed");
		DWORD error = GetLastError();
		if (error == ERROR_INSUFFICIENT_BUFFER)
			FAIL;
	}

	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;
	if (!GetFileTime(bufferHandle, &creationTime, &lastAccessTime, &lastWriteTime))
	{
		DebugPrint("GetFileTime failed with %d\n", GetLastError());
	}
	CloseHandle(bufferHandle);

	result.lastWriteTime = uint64(lastWriteTime.dwHighDateTime) << 32;
	result.lastWriteTime |= (uint64(lastWriteTime.dwLowDateTime) & 0xffffffff);

	return result;
}
