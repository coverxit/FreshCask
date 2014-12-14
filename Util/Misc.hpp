#ifndef __UTIL_MISC_HPP__
#define __UTIL_MISC_HPP__

#include <ctime>

namespace FreshCask
{
	uint32_t GetTimeStamp() { return (uint32_t) time(NULL); }

	bool IsFileExist(std::string& filePath)
	{
#ifdef WIN32
		DWORD dwAttrib = GetFileAttributesA(filePath.c_str());
		return dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
#endif
	}

	bool IsDirExist(std::string& filePath)
	{
#ifdef WIN32
		DWORD dwAttrib = GetFileAttributesA(filePath.c_str());
		return dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
#endif
	}
} // namespace FreshCask

#endif // __UTIL_MISC_HPP__
