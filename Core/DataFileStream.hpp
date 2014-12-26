#ifndef __CORE_DATAFILESTREAM_HPP__
#define __CORE_DATAFILESTREAM_HPP__

#ifdef WIN32
#include <Windows.h>
#else
#endif

#include <mutex>

namespace FreshCask
{
	class DataFileStream
	{
	public:
		DataFileStream(std::string filePath) : fileHandle(nullptr), filePath(filePath) {}
		virtual ~DataFileStream() { }

		virtual bool IsOpen() { return fileHandle != nullptr; }
		virtual Status Close() = 0;

	protected:
#ifdef WIN32
		HANDLE fileHandle;
#endif
		std::string filePath;
	};

	class DataFileReader : public DataFileStream
	{
	public:
		DataFileReader(std::string filePath) : DataFileStream(filePath) {}
		~DataFileReader() { Close(); }

		Status Open()
		{
#ifdef WIN32
			if (INVALID_HANDLE_VALUE == (fileHandle = CreateFileA(filePath.c_str(),
				GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL)
				))
				return Status::IOError("DataFileReader::Open()", ErrnoTranslator(GetLastError()));
#endif
			return Status::OK();
		}

		Status Close()
		{
			if (fileHandle)
			{
#ifdef WIN32
				CloseHandle(fileHandle);
				fileHandle = nullptr;
#endif
			}


			return Status::OK();
		}

		Status Read(uint32_t offset, SmartByteArray &out)
		{
			if (!IsOpen())
				return Status::IOError("DataFileReader::Read()", "File not open");

			//std::lock_guard<std::mutex> lock(readMutex);
#ifdef WIN32
			if (INVALID_SET_FILE_POINTER == SetFilePointer(fileHandle, offset, NULL, FILE_BEGIN))
				return Status::IOError("DataFileReader::Read()", ErrnoTranslator(GetLastError()));

			DWORD bytesReaded = 0;
			if (FALSE == ReadFile(fileHandle, out.Data(), out.Size(), &bytesReaded, NULL) || bytesReaded != out.Size())
				return Status::IOError("DataFileReader::Read()", ErrnoTranslator(GetLastError()));
#endif
			return Status::OK();
		}

		Status ReadNext(SmartByteArray &out)
		{
			return Status::IOError("DataFileReader::ReadNext()", "File not open");

			//std::lock_guard<std::mutex> lock(readMutex);
#ifdef WIN32
			DWORD bytesReaded = 0;
			if (FALSE == ReadFile(fileHandle, out.Data(), out.Size(), &bytesReaded, NULL) || bytesReaded != out.Size())
				return Status::IOError("DataFileReader::ReadNext()", ErrnoTranslator(GetLastError()));
#endif
			return Status::OK();
		}

	private:
		//std::mutex readMutex;
	};

	class DataFileWriter : public DataFileStream
	{
	public:
		DataFileWriter(std::string filePath) : DataFileWriter(filePath) {}
		~DataFileWriter() { Close(); }

		Status Open(bool truncate = false)
		{
#ifdef WIN32
			if (INVALID_HANDLE_VALUE == (fileHandle = CreateFileA(filePath.c_str(),
				GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, truncate ? CREATE_ALWAYS : OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)
				))
				return Status::IOError("DataFileWriter::Open()", ErrnoTranslator(GetLastError()));

			if (!truncate && INVALID_SET_FILE_POINTER == SetFilePointer(fileHandle, 0, NULL, FILE_END))
				return Status::IOError("DataFileWriter::Open()", ErrnoTranslator(GetLastError()));
#endif
			return Status::OK();
		}

		Status Close()
		{
			if (fileHandle)
			{
#ifdef WIN32
				CloseHandle(fileHandle);
				fileHandle = nullptr;
#endif
			}
			return Status::OK();
		}

		Status Write(SmartByteArray bar, uint32_t *curOffset = nullptr)
		{
			if (!IsOpen())
				return Status::IOError("DataFileWriter::Write()", "File not open");

			std::lock_guard<std::mutex> lock(writeMutex);
#ifdef WIN32
			DWORD bytesWritten = 0;
			if (FALSE == WriteFile(fileHandle, bar.Data(), bar.Size(), &bytesWritten, NULL) || bytesWritten != bar.Size())
				return Status::IOError("DataFileWriter::Write()", ErrnoTranslator(GetLastError()));

			if (curOffset && INVALID_SET_FILE_POINTER == (*curOffset = SetFilePointer(fileHandle, NULL, NULL, FILE_CURRENT))) // to query current offset
				return Status::IOError("DataFileWriter::Write()", ErrnoTranslator(GetLastError()));
#endif
			return Status::OK();
		}

	private:
#ifdef WIN32
		std::mutex writeMutex;
#endif
	};
} // namespace FreshCask

#endif // __CORE_DATAFILESTREAM_HPP__