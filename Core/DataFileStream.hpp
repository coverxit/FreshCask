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
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "DataFileReader::Open()");
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
				RET_BY_SENDER(Status::IOError("File not open"), "DataFileReader::Read()");

			//std::lock_guard<std::mutex> lock(readMutex);
#ifdef WIN32
			if (INVALID_SET_FILE_POINTER == SetFilePointer(fileHandle, offset, NULL, FILE_BEGIN))
				RET_BY_SENDER(Status::IOError("Failed to SetFilePointer"), "DataFileReader::Read()");

			DWORD bytesReaded = 0;
			if (FALSE == ReadFile(fileHandle, out.Data(), out.Size(), &bytesReaded, NULL))
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "DataFileReader::Read()");
			else if (bytesReaded != out.Size())
				RET_BY_SENDER(Status::IOError("Bytes readed is less than out.Size()."), "DataFileReader::Read()");
#endif
			return Status::OK();
		}

		Status ReadNext(SmartByteArray &out)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open"), "DataFileReader::ReadNext()");

			//std::lock_guard<std::mutex> lock(readMutex);
#ifdef WIN32
			DWORD bytesReaded = 0;
			if (FALSE == ReadFile(fileHandle, out.Data(), out.Size(), &bytesReaded, NULL) || bytesReaded != out.Size())
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "DataFileReader::ReadNext()");
#endif
			return Status::OK();
		}

	private:
		//std::mutex readMutex;
	};

	class DataFileWriter : public DataFileStream
	{
	public:
		DataFileWriter(std::string filePath) : DataFileStream(filePath) {}
		~DataFileWriter() { Close(); }

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

		Status Write(uint32_t offset, SmartByteArray bar, uint32_t *offsetOut = nullptr)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open"), "DataFileWriter::Write()");

			std::lock_guard<std::mutex> lock(writeMutex);

			if (INVALID_SET_FILE_POINTER == SetFilePointer(fileHandle, offset, NULL, FILE_BEGIN))
				RET_BY_SENDER(Status::IOError("Failed to SetFilePointer"), "DataFileReader::Write()");

			if (offset + bar.Size() > DataFile::MaxFileSize)
				RET_BY_SENDER(Status::NoFreeSpace("MaxFileSize reached"), "DataFileWriter::Write()");

#ifdef WIN32
			DWORD bytesWritten = 0;
			if (FALSE == WriteFile(fileHandle, bar.Data(), bar.Size(), &bytesWritten, NULL) || bytesWritten != bar.Size())
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "DataFileWriter::Write()");

			if (offsetOut) *offsetOut = offset;
#endif
			return Status::OK();
		}

		Status WriteNext(SmartByteArray bar, uint32_t *offsetOut = nullptr)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open"), "DataFileWriter::Write()");

			std::lock_guard<std::mutex> lock(writeMutex);

			uint32_t curOffset;
			if (INVALID_SET_FILE_POINTER == (curOffset = SetFilePointer(fileHandle, NULL, NULL, FILE_CURRENT))) // query current offset
				RET_BY_SENDER(Status::IOError("Failed to SetFilePointer"), "DataFileWriter::Write()");

			if (bar.Size() == 0)
				return Status::OK();

			if (curOffset + bar.Size() > DataFile::MaxFileSize)
				RET_BY_SENDER(Status::NoFreeSpace("MaxFileSize reached"), "DataFileWriter::Writer()");

#ifdef WIN32
			DWORD bytesWritten = 0;
			if (FALSE == WriteFile(fileHandle, bar.Data(), bar.Size(), &bytesWritten, NULL) || bytesWritten != bar.Size())
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "DataFileWriter::Write()");

			if (offsetOut) *offsetOut = curOffset;
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
#ifdef WIN32
		std::mutex writeMutex;
#endif
	};
} // namespace FreshCask

#endif // __CORE_DATAFILESTREAM_HPP__