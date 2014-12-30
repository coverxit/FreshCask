#ifndef __CORE_HINTFILESTREAM_HPP__
#define __CORE_HINTFILESTREAM_HPP__

#include <mutex>

namespace FreshCask
{
	class HintFileReader : public FileReader
	{
	public:
		HintFileReader(const std::string &filePath) : filePath(filePath) {}

		Status Open()
		{
#ifdef WIN32
			if (INVALID_HANDLE_VALUE == (fileHandle = CreateFileA(filePath.c_str(),
				GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)
				))
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "HintFileReader::Open()");
#endif
			return Status::OK();
		}

	private:
		std::string filePath;
		//std::mutex readMutex;
	};

	class HintFileWriter : public FileWriter
	{
	public:
		HintFileWriter(const std::string &filePath) : filePath(filePath) {}

		Status Open()
		{
#ifdef WIN32
			if (INVALID_HANDLE_VALUE == (fileHandle = CreateFileA(filePath.c_str(),
				GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL)
				))
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "HintFileWriter::Open()");

#endif
			return Status::OK();
		}

	private:
		std::string filePath;
#ifdef WIN32
		std::mutex writeMutex;
#endif
	};
} // namespace FreshCask

#endif // __CORE_HINTFILESTREAM_HPP__