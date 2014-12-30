#ifndef __UTIL_MISC_HPP__
#define __UTIL_MISC_HPP__

#include <ctime>
#include <functional>

namespace FreshCask
{
	uint32_t GetTimeStamp() { return (uint32_t) time(NULL); }

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

	Status ListDir(const std::string& dirPath, std::function<Status(std::string)> func)
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
		return Status::OK();
#else
#endif
	}

	bool EndWith(const std::string &str, const std::string &match)
	{
		return str.substr(str.length() - match.length()) == match;
	}

	class CRC32
	{
	public:
		typedef uint32_t CRCType;

		static CRCType Get(SmartByteArray bar)
		{
			CRCType crc = 0xFFFFFFFF;
			uint32_t len = bar.Size();
			BytePtr buffer = bar.Data();

			initTable();

			while (len--)
				crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *buffer++];

			return crc ^ 0xFFFFFFFF;
		}

		static CRCType CalcDataFileRecord(DataFile::Record dfRec)
		{
			SmartByteArray buffer(sizeof(DataFile::RecordHeader) + dfRec.Key.Size() + dfRec.Value.Size());
			BytePtr ptr = buffer.Data();

			memcpy(ptr, &dfRec.Header, sizeof(DataFile::RecordHeader));
			memcpy(ptr + sizeof(DataFile::RecordHeader), dfRec.Key.Data(), dfRec.Key.Size());
			memcpy(ptr + sizeof(DataFile::RecordHeader) + dfRec.Key.Size(), dfRec.Value.Data(), dfRec.Value.Size());

			return Get(buffer);
		}

	private:
		static void initTable()
		{
			bool init = false;

			if (init) return;

			init = true;
			for (int i = 0; i < 256; i++)
			{
				CRCType crc = i;
				for (int j = 0; j < 8; j++)
				{
					if (crc & 1)
						crc = (crc >> 1) ^ 0xEDB88320;
					else
						crc = crc >> 1;
				}
				CRCTable[i] = crc;
			}
		}
		static CRCType CRCTable[256];
	};
	CRC32::CRCType CRC32::CRCTable[256] = { 0 };
} // namespace FreshCask

#endif // __UTIL_MISC_HPP__
