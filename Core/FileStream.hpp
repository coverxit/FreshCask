#ifndef __CORE_FILESTREAM_HPP__
#define __CORE_FILESTREAM_HPP__

#ifdef WIN32
#include <Windows.h>
#else
#endif

#include <mutex>

namespace FreshCask
{
	class FileStream
	{
	public:
#ifdef WIN32
		FileStream() : fileHandle(nullptr) {}
		FileStream(const HANDLE& fileHandle) : fileHandle(fileHandle) {}
#else
#endif
		virtual ~FileStream() { }
#ifdef WIN32
		virtual bool IsOpen() { return fileHandle != nullptr && fileHandle != INVALID_HANDLE_VALUE; }
#else
#endif
		virtual Status Close() = 0;

	protected:
#ifdef WIN32
		HANDLE fileHandle;
#else
#endif
	};

	class FileReader : public FileStream
	{
	public:
		FileReader() : FileStream(nullptr) {}
		FileReader(const HANDLE& fileHandle) : FileStream(fileHandle) {}
		virtual ~FileReader() { Close(); }

		Status Close()
		{
#ifdef WIN32
			if (IsOpen()) CloseHandle(fileHandle);
#endif

			fileHandle = nullptr;
			return Status::OK();
		}

		Status Read(uint32_t offset, SmartByteArray &out)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open"), "FileReader::Read()");

			//std::lock_guard<std::mutex> lock(readMutex);
#ifdef WIN32
			if (INVALID_SET_FILE_POINTER == SetFilePointer(fileHandle, offset, NULL, FILE_BEGIN))
				RET_BY_SENDER(Status::IOError("Failed to SetFilePointer"), "FileReader::Read()");

			DWORD bytesReaded = 0;
			if (FALSE == ReadFile(fileHandle, out.Data(), out.Size(), &bytesReaded, NULL))
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "FileReader::Read()");
			else if (bytesReaded != out.Size())
			{
				if (bytesReaded > 0)
					RET_BY_SENDER(Status::IOError("Bytes readed is less than out.Size()."), "FileReader::Read()");
				else
					RET_BY_SENDER(Status::EndOfFile("End Of File reached."), "FileReader::Read()");
			}
#endif
			return Status::OK();
		}

		Status ReadNext(SmartByteArray &out)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open"), "FileReader::ReadNext()");

			//std::lock_guard<std::mutex> lock(readMutex);
#ifdef WIN32
			DWORD bytesReaded = 0;
			if (FALSE == ReadFile(fileHandle, out.Data(), out.Size(), &bytesReaded, NULL))
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "FileReader::ReadNext()");
			else if (bytesReaded != out.Size())
			{
				if (bytesReaded > 0)
					RET_BY_SENDER(Status::IOError("Bytes readed is less than out.Size()."), "FileReader::ReadNext()");
				else
					RET_BY_SENDER(Status::EndOfFile("End Of File reached."), "FileReader::ReadNext()");
			}
#endif
			return Status::OK();
		}

	private:
		//std::mutex readMutex;
	};

	class FileWriter : public FileStream
	{
	public:
		FileWriter() : FileStream(nullptr) {}
		FileWriter(const HANDLE& fileHandle) : FileStream(fileHandle) {}
		virtual ~FileWriter() { Close(); }

		Status Close()
		{
#ifdef WIN32
			if (IsOpen()) CloseHandle(fileHandle);
#endif

			fileHandle = nullptr;
			return Status::OK();
		}

		Status Write(uint32_t offset, const SmartByteArray& bar)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open"), "FileWriter::Write()");

			std::lock_guard<std::mutex> lock(writeMutex);

#ifdef WIN32
			DWORD bytesWritten = 0;
			if (FALSE == WriteFile(fileHandle, bar.Data(), bar.Size(), &bytesWritten, NULL) || bytesWritten != bar.Size())
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "FileWriter::Write()");
#endif
			return Status::OK();
		}

		Status WriteNext(const SmartByteArray& bar)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open"), "FileWriter::Write()");

			std::lock_guard<std::mutex> lock(writeMutex);

#ifdef WIN32
			DWORD bytesWritten = 0;
			if (FALSE == WriteFile(fileHandle, bar.Data(), bar.Size(), &bytesWritten, NULL) || bytesWritten != bar.Size())
				RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "FileWriter::Write()");
#endif
			return Status::OK();
		}

	private:
#ifdef WIN32
		std::mutex writeMutex;
#endif
	};

} // namespace FreshCask

#endif // __CORE_FILESTREAM_HPP__