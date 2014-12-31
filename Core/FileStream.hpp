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
			RET_BY_SENDER(Status::OK(), "FileReader::Close()");
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
			RET_BY_SENDER(Status::OK(), "FileReader::Read()");
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
			RET_BY_SENDER(Status::OK(), "FileReader::ReadNext()");
		}

	private:
		//std::mutex readMutex;
	};

	class FileWriter : public FileStream
	{
	public:
		FileWriter() : FileStream(nullptr), bufUsed(0) {}
		FileWriter(const HANDLE& fileHandle) : FileStream(fileHandle), bufUsed(0) {}
		virtual ~FileWriter() { Close(); }

		Status Close()
		{
#ifdef WIN32
			if (IsOpen())
			{
				Flush();
				CloseHandle(fileHandle);
			}
#endif

			fileHandle = nullptr;
			RET_BY_SENDER(Status::OK(), "FileWrite::Close()");
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
			RET_BY_SENDER(Status::OK(), "FileWriter::Write()");
		}

		Status WriteNext(const SmartByteArray& bar)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open"), "FileWriter::WriteNext()");

			std::lock_guard<std::mutex> lock(writeMutex);
			RET_BY_SENDER(WriteBuffer(bar), "FileWriter::WriteNext()");
		}

	private:
		Status WriteBuffer(const SmartByteArray& bar)
		{
			if (bar.Size() >= DataFile::WriteBufferSize)
			{
#ifdef WIN32
				DWORD bytesWritten = 0;
				if (FALSE == WriteFile(fileHandle, bar.Data(), bar.Size(), &bytesWritten, NULL) || bytesWritten != bar.Size())
					RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "FileWriter::WriteBuffer()");
#endif
			}
			else
			{
				if (bar.Size() + bufUsed > DataFile::WriteBufferSize)
					RET_IFNOT_OK(Flush(), "FileWriter::WriteBuffer()");

				memcpy(buffer + bufUsed, bar.Data(), bar.Size());
			}

			RET_BY_SENDER(Status::OK(), "FileWriter::WriteBuffer()");
		}

		Status Flush()
		{
			if (bufUsed > 0)
			{
#ifdef WIN32
				// TODO: cope with situations that if write failed and buffer not all flushed into disk.
				DWORD bytesWritten = 0;
				if (FALSE == WriteFile(fileHandle, buffer, bufUsed, &bytesWritten, NULL) || bytesWritten != bufUsed)
					RET_BY_SENDER(Status::IOError(ErrnoTranslator(GetLastError())), "FileWriter::Flush()");
#endif
			}

			bufUsed = 0;
			RET_BY_SENDER(Status::OK(), "FileWriter::Flush()");
		}

	private:
		Byte buffer[DataFile::WriteBufferSize];
		uint32_t bufUsed;

		std::mutex writeMutex;
	};

} // namespace FreshCask

#endif // __CORE_FILESTREAM_HPP__