#ifndef __CORE_DATAFILESTREAM_HPP__
#define __CORE_DATAFILESTREAM_HPP__

#include <mutex>

namespace FreshCask
{
	class DataFileReader : public FileReader
	{
	public:
		DataFileReader(const std::string &filePath) : filePath(filePath) {}

		Status Open()
		{
#ifdef WIN32
			if (INVALID_HANDLE_VALUE == (fileHandle = CreateFileA(filePath.c_str(),
				GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL)
				))
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "DataFileReader::Open()");
#endif
			return Status::OK();
		}

	private:
		std::string filePath;
		//std::mutex readMutex;
	};

	class DataFileWriter : public FileWriter
	{
	public:
		DataFileWriter(const std::string &filePath) : filePath(filePath) {}

		Status Open(bool truncate = false)
		{
#ifdef WIN32
			if (INVALID_HANDLE_VALUE == (fileHandle = CreateFileA(filePath.c_str(),
				GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, truncate ? CREATE_ALWAYS : OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)
				))
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "DataFileWriter::Open()");

			if (!truncate && INVALID_SET_FILE_POINTER == SetFilePointer(fileHandle, 0, NULL, FILE_END))
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "DataFileWriter::Open()");
#endif
			return Status::OK();
		}

		Status GetOffset(uint32_t &out)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open"), "DataFileWriter::GetOffset()");

			if (INVALID_SET_FILE_POINTER == (out = SetFilePointer(fileHandle, NULL, NULL, FILE_CURRENT))) // query current offset
				RET_BY_SENDER(Status::IOError("Failed to SetFilePointer"), "DataFileWriter::GetOffset()");

			return Status::OK();
		}

	private:
		std::string filePath;
#ifdef WIN32
		std::mutex writeMutex;
#endif
	};
} // namespace FreshCask

#endif // __CORE_DATAFILESTREAM_HPP__