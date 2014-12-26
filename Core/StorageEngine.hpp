#ifndef __CORE_STORAGEENGINE_HPP__
#define __CORE_STORAGEENGINE_HPP__

#include <Core/DataFile.h>
#include <Core/HashFile.h>
#include <Core/DataFileStream.hpp>

namespace FreshCask
{
	/*class MappingEngine
	{
	public:
		MappingEngine() : hFile(nullptr), hMapping(nullptr), writeBuffer(nullptr) {}
		~MappingEngine() { Close(); }
		
		Status Create(std::string filePath)
		{
#ifdef WIN32
			if ( INVALID_HANDLE_VALUE == ( hFile = CreateFileA(filePath.c_str(), 
										   GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 
										   NULL, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN, NULL)
									 	 ) )
				return Status::IOError("MappingEngine::Create()", ErrnoTranslator(GetLastError()));

			currentFileSize = GetFileSize(hFile, NULL);
			if ( NULL == ( hMapping = CreateFileMapping( hFile, NULL, PAGE_READWRITE, 
														 NULL, DataFile::MaxFileSize, NULL 
													   ) ) )
				return Status::IOError("MappingEngine::Create()", ErrnoTranslator(GetLastError()));
#else

#endif
			return Status::OK();
		}

		Status Read(SmartByteArray &out)
		{
#ifdef WIN32
			
#else
#endif
		}

		Status Write(SmartByteArray bar)
		{
			std::lock_guard<std::mutex> lock(writeMutex);
#ifdef WIN32
			static DWORD writtenSize = 0;
			
			if (writtenSize + bar.Size() > DataFile::MaxPageSize)
			{
				UnmapViewOfFile(writeBuffer);
				writtenSize = 0;
			}

			if (writtenSize == 0)
			{
				DWORD writtenSize = currentFileSize % GetAllocationGranularity();
				writeBuffer = (BytePtr) MapViewOfFile(hMapping, FILE_MAP_WRITE, NULL, currentFileSize / GetAllocationGranularity(), DataFile::MaxPageSize);
			}

			memcpy(&writeBuffer[writtenSize], bar.Data(), bar.Size());
			writtenSize += bar.Size(); currentFileSize += bar.Size();
#else
#endif
			return Status::OK();
		}

		Status Close()
		{
#ifdef WIN32
			if (writeBuffer) UnmapViewOfFile(writeBuffer);
			if (hMapping) CloseHandle(hMapping);
			if (hFile) 
			{
				SetFilePointer(hFile, currentFileSize, NULL, FILE_BEGIN);
				SetEndOfFile(hFile);
				CloseHandle(hFile);
			}
#else

#endif
			return Status::OK();
		}

		bool IsMapped() { return hFile && hMapping; }

	private:
#ifdef WIN32
		static DWORD GetAllocationGranularity()
		{
			static DWORD allocationGranularity = 0xFFFFFFFF;
			if (allocationGranularity == 0xFFFFFFFF)
			{
				SYSTEM_INFO sinf;
				GetSystemInfo(&sinf);
				allocationGranularity = sinf.dwAllocationGranularity;
			}
			return allocationGranularity;
		}
#else
#endif

	private:
#ifdef WIN32
		HANDLE hFile, hMapping;
		DWORD currentFileSize; BytePtr writeBuffer;
		std::mutex writeMutex;
#else
#endif
	};*/
	class StorageEngine
	{
	public:
		StorageEngine(std::string filePath) : filePath(filePath), reader(filePath), writer(filePath), fileFlag(DataFile::Flag::ActiveFile) {}
		~StorageEngine() { Close(); }

		bool IsOpen() 
		{
			switch (fileFlag)
			{
			case DataFile::Flag::OlderFile:
				return reader.IsOpen();

			case DataFile::Flag::ActiveFile:
				return reader.IsOpen() && writer.IsOpen();

			default:
				return false;
			}
		}

		Status Open()
		{
			if (!IsFileExist(filePath))
				return Status::NotFound("StorageEngine::Open()", "File doesn't exist.");

			RET_IFNOT_OK(reader.Open(), "StorageEngine::Open()");
			
			// check header
			std::function<Status()> CheckHeader = [&]() {
				SmartByteArray buffer(new Byte[sizeof(DataFile::Header)], sizeof(DataFile::Header));
				RET_IFNOT_OK(reader.Read(0, buffer), "StorageEngine::CheckHeader()");

				DataFile::Header *header = reinterpret_cast<DataFile::Header*>(buffer.Data());
				if (header->MagicNumber != DataFile::DefaultMagicNumber)
					return Status::InvalidArgument("StorageEngine::CheckHeader()", "Incorrect magic number");
				if (header->MajorVersion > CurrentMajorVersion)
					return Status::NotSupported("StorageEngine::CheckHeader()", "DataFile not supported");
				else if (header->MinorVersion > CurrentMinorVersion)
					return Status::NotSupported("StorageEngine::CheckHeader()", "DataFile not supported");
				if (header->Flag < DataFile::Flag::OlderFile && header->Flag > DataFile::Flag::ActiveFile)
					return Status::NotSupported("StorageEngine::CheckHeader()", "Invalid flag.");

				fileFlag = header->Flag;
				return Status::OK();
			};

			RET_IFNOT_OK(CheckHeader(), "StorageEngine::Open()");
			RET_IFNOT_OK(writer.Open(), "StorageEngine::Open()");

			return Status::OK();
			/*Status s = headerChecker();
			if (s.IsOK()) 
			{ 
				RET_IFNOT_OK(writer.Open(), "StorageEngine::Open()");
				return Status::OK();
			}
			else if (s.IsInvaildArgument()) // magicnumber incorrect
			{

				RET_IFNOT_OK(writer.Open(filePath, true), "StorageEngine::Open()");

				SmartByteArray buffer(new Byte[sizeof(DataFile::Header)], sizeof(DataFile::Header));
				DataFile::Header *header = reinterpret_cast<DataFile::Header*>(buffer.Data());
				header->MagicNumber = DataFile::DefaultMagicNumber;
				header->MajorVersion = CurrentMajorVersion;
				header->MinorVersion = CurrentMinorVersion;
				header->Flag = DataFile::ActiveFile;
				header->Reserved = 0x0;

				RET_IFNOT_OK(writer.Write(buffer), "StorageEngine::Open()");
			}
			else return s; // other status*/
		}

		Status Create()
		{
			RET_IFNOT_OK(writer.Open(true), "StorageEngine::Open()");

			// writer header
			SmartByteArray buffer(new Byte[sizeof(DataFile::Header)], sizeof(DataFile::Header));
			DataFile::Header *header = reinterpret_cast<DataFile::Header*>(buffer.Data());
			header->MagicNumber = DataFile::DefaultMagicNumber;
			header->MajorVersion = CurrentMajorVersion;
			header->MinorVersion = CurrentMinorVersion;
			header->Flag = DataFile::Flag::ActiveFile;
			header->Reserved = 0x0;

			RET_IFNOT_OK(writer.Write(buffer), "StorageEngine::Open()");
			RET_IFNOT_OK(reader.Open(), "StorageEngine::Open()");

			return Status::OK();
		}

		Status Close()
		{
			if (!IsOpen())
				return Status::IOError("StorageEngine::Close()", "File not open");

			reader.Close(); writer.Close();
			return Status::OK();
		}

		Status ReadRecord(uint32_t offset, DataFile::Record &out)
		{
			SmartByteArray buffer((BytePtr) &out.Header, sizeof(DataFile::RecordHeader));
			RET_IFNOT_OK(reader.Read(offset, buffer), "StorageEngine::ReadRecord()");

			out.Key = SmartByteArray(out.Header.SizeOfKey);
			RET_IFNOT_OK(reader.ReadNext(out.Key), "StorageEngine::ReadRecord()");

			out.Value = SmartByteArray(out.Header.SizeOfValue);
			RET_IFNOT_OK(reader.ReadNext(out.Value), "StorageEngine::ReadRecord()");

			return Status::OK();
		}

		Status WriteRecord(DataFile::Record dfRec, HashFile::Record &hfRecOut)
		{
			dfRec.Header.CRC32 = -1; // TODO
			hfRecOut.TimeStamp = dfRec.Header.TimeStamp = GetTimeStamp();

			RET_IFNOT_OK(writer.Write(SmartByteArray((BytePtr)&dfRec.Header, sizeof(DataFile::RecordHeader))), "StorageEngine::WriteRecord()");
			RET_IFNOT_OK(writer.Write(dfRec.Key), "StorageEngine::WriteRecord()");
			RET_IFNOT_OK(writer.Write(dfRec.Value, &hfRecOut.OffsetOfValue), "StorageEngine::WriteRecord()");

			hfRecOut.DataFileId = -1; //TODO
			return Status::OK();
		}

	protected:
		DataFileReader reader;
		DataFileWriter writer;
		std::string filePath;
		uint8_t fileFlag;
	};
} // namespace FreshCask
#endif // __CORE_STORAGEENGINE_HPP__