#ifndef __UTIL_MISC_HPP__
#define __UTIL_MISC_HPP__

#include <functional>

namespace FreshCask
{
	bool IsFileExist(const std::string& filePath)
	{
#ifdef WIN32
		DWORD dwAttrib = GetFileAttributesA(filePath.c_str());
		return dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
#endif
	}

	bool IsDirExist(const std::string& dirPath)
	{
#ifdef WIN32
		DWORD dwAttrib = GetFileAttributesA(dirPath.c_str());
		return dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
#endif
	}

	Status ListDir(const std::string& dirPath, std::function<Status(const std::string&)> func)
	{
#ifdef WIN32
		std::string query = dirPath + "\\*.*";
		WIN32_FIND_DATAA fileData;
		HANDLE hFind;
		Status funcRet;

		if ((hFind = FindFirstFileA(query.c_str(), &fileData)) == INVALID_HANDLE_VALUE)
			return Status::IOError("Utils::ListDir()", "Failed to FindFirstFile.");

		do
		{
			if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				RET_IFNOT_OK(func(dirPath + "\\" + fileData.cFileName), "Utils::ListDir()");
		} while (FindNextFileA(hFind, &fileData));

		FindClose(hFind);
		RET_BY_SENDER(Status::OK(), "Utils::ListDir()");
#else
#endif
	}

	bool EndWith(const std::string &str, const std::string &match)
	{
		return str.substr(str.length() - match.length()) == match;
	}

	Status RemoveFile(const std::string& path)
	{
#ifdef WIN32
		if (::DeleteFileA(path.c_str())) RET_BY_SENDER(Status::OK(), "Utils::RemoveFile()");
		else RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "Utils::RemoveFile()");
#endif
	}

	Status RemoveDir(const std::string& path)
	{
		auto processFile = [&](const std::string &file) -> Status {
			RET_BY_SENDER(RemoveFile(file), "Utils::RemoveDir()::ProcessFile()");
		};

		RET_IFNOT_OK(ListDir(path, processFile), "Utils::RemoveDir()");

		if (::RemoveDirectoryA(path.c_str())) RET_BY_SENDER(Status::OK(), "Utils::RemoveDir()");
		else RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "Utils::RemoveDir()");
	}

	Status RenameFile(const std::string& oldPath, const std::string& newPath)
	{
#ifdef WIN32
		if (::MoveFileA(oldPath.c_str(), newPath.c_str())) RET_BY_SENDER(Status::OK(), "Utils::RenameFile()");
		else RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "Utils::RenameFile()");
#endif
	}

	Status MakeDir(const std::string& path)
	{
#ifdef WIN32
		if (::CreateDirectoryA(path.c_str(), NULL)) RET_BY_SENDER(Status::OK(), "Utils::MakeDir()");
		else RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "Utils::MakeDir()");
#endif
	}
} // namespace FreshCask

#endif // __UTIL_MISC_HPP__
