#ifndef __CORE_STORAGEENGINE_HPP__
#define __CORE_STORAGEENGINE_HPP__

#include <Core/DataFile.h>
#include <Core/HashFile.h>
#include <Core/DataFileStream.hpp>

#include <Util/Misc.hpp>

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
	class DataFileEngine
	{
	public:
		DataFileEngine(std::string filePath) : filePath(filePath), reader(filePath), writer(filePath), fileId(-1), fileFlag(DataFile::Flag::ActiveFile) {}
		~DataFileEngine() { Close(); }

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
				RET_BY_SENDER(Status::NotFound("File doesn't exist."), "DataFileEngine::Open()");

			RET_IFNOT_OK(reader.Open(), "DataFileEngine::Open()");
			
			// check header
			std::function<Status()> CheckHeader = [&]() {
				SmartByteArray buffer(new Byte[sizeof(DataFile::Header)], sizeof(DataFile::Header));
				RET_IFNOT_OK(reader.Read(0, buffer), "DataFileEngine::CheckHeader()");

				DataFile::Header *header = reinterpret_cast<DataFile::Header*>(buffer.Data());
				if (header->MagicNumber != DataFile::DefaultMagicNumber)
					RET_BY_SENDER(Status::InvalidArgument("Incorrect magic number"), "DataFileEngine::CheckHeader()");
				if (header->MajorVersion > CurrentMajorVersion)
					RET_BY_SENDER(Status::NotSupported("DataFile not supported"), "DataFileEngine::CheckHeader()");
				else if (header->MinorVersion > CurrentMinorVersion)
					RET_BY_SENDER(Status::NotSupported("DataFile not supported"), "DataFileEngine::CheckHeader()");
				/*if (header->Flag < DataFile::Flag::OlderFile && header->Flag > DataFile::Flag::ActiveFile)
					return Status::NotSupported("DataFileEngine::CheckHeader()", "Invalid flag.");*/

				fileFlag = header->Flag;
				fileId = header->FileId;
				return Status::OK();
			};

			RET_IFNOT_OK(CheckHeader(), "DataFileEngine::Open()");
			RET_IFNOT_OK(writer.Open(), "DataFileEngine::Open()");

			return Status::OK();
		}

		Status Create(uint32_t _fileId)
		{
			RET_IFNOT_OK(writer.Open(true), "DataFileEngine::Open()");

			// writer header
			SmartByteArray buffer(sizeof(DataFile::Header));
			DataFile::Header *header = reinterpret_cast<DataFile::Header*>(buffer.Data());
			header->MagicNumber = DataFile::DefaultMagicNumber;
			header->MajorVersion = CurrentMajorVersion;
			header->MinorVersion = CurrentMinorVersion;
			header->Flag = DataFile::Flag::ActiveFile;
			header->FileId = _fileId;
			header->Reserved = 0x0;

			RET_IFNOT_OK(writer.WriteNext(buffer), "DataFileEngine::Open()");
			RET_IFNOT_OK(reader.Open(), "DataFileEngine::Open()");

			fileFlag = DataFile::Flag::ActiveFile;
			fileId = _fileId;
			return Status::OK();
		}

		Status Close()
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::NotFound("File not open."), "DataFileEngine::Close()");

			reader.Close(); writer.Close();
			return Status::OK();
		}

		Status ReadValue(HashFile::Record hfRec, SmartByteArray &valueOut)
		{
			RET_BY_SENDER(reader.Read(hfRec.OffsetOfValue, valueOut = SmartByteArray(hfRec.SizeOfValue)), "DataFileEngine::ReadValue()");
		}

		/*Status ReadRecord(HashFile::Record hfRec, DataFile::Record &dfRecOut)
		{
			SmartByteArray header(sizeof(dfRecOut.CRC32) + sizeof(DataFile::RecordHeader));
			RET_IFNOT_OK(reader.Read(hfRec.OffsetOfRecord, header), "DataFileEngine::ReadRecord()");

			memcpy(&dfRecOut.CRC32, header.Data(), sizeof(dfRecOut.CRC32));
			memcpy(&dfRecOut.Header, header.Data() + sizeof(dfRecOut.CRC32), sizeof(DataFile::RecordHeader));

			dfRecOut.Key = SmartByteArray(dfRecOut.Header.SizeOfKey);

			RET_IFNOT_OK(reader.ReadNext(dfRecOut.Key), "DataFileEngine::ReadRecord()");

			dfRecOut.Value = SmartByteArray(dfRecOut.Header.SizeOfValue);
			RET_IFNOT_OK(reader.ReadNext(dfRecOut.Value), "DataFileEngine::ReadRecord()");

			uint32_t CRC32 = CRC32::CalcDataFileRecord(dfRecOut);
			if (dfRecOut.CRC32 != CRC32) RET_BY_SENDER(Status::Corrupted("CRC32 checksum incorrect"), "DataFileEngine::ReadRecord()");

			return Status::OK();
		}*/

		Status WriteRecord(DataFile::Record dfRec, HashFile::Record &hfRecOut)
		{
			if (fileFlag & DataFile::Flag::OlderFile)
				RET_BY_SENDER(Status::NoFreeSpace("Current data file is older file."), "DataFileEngine::WriteRecord()");

			uint32_t curOffset;
			RET_IFNOT_OK(writer.GetOffset(curOffset), "DataFileEngine::WriteRecord()");

			if (curOffset + dfRec.GetSize() > DataFile::MaxFileSize)
			{
				// TODO: Write file header
				fileFlag = DataFile::Flag::OlderFile;
				RET_IFNOT_OK(writer.Write(offsetof(DataFile::Header, Flag), SmartByteArray(&fileFlag, sizeof(fileFlag))), "DataFileEngine::WriteRecord()");

				RET_BY_SENDER(Status::NoFreeSpace("MaxFileSize reached."), "DataFileEngine::WriteRecord()");
			}

			hfRecOut.TimeStamp = dfRec.Header.TimeStamp = GetTimeStamp();
			dfRec.CRC32 = CRC32::CalcDataFileRecord(dfRec);

			SmartByteArray header(sizeof(dfRec.CRC32) + sizeof(DataFile::RecordHeader));
			memcpy(header.Data(), &dfRec.CRC32, sizeof(dfRec.CRC32));
			memcpy(header.Data() + sizeof(dfRec.CRC32), &dfRec.Header, sizeof(DataFile::RecordHeader));

			RET_IFNOT_OK(writer.WriteNext(header), "DataFileEngine::WriteRecord()");
			RET_IFNOT_OK(writer.WriteNext(dfRec.Key), "DataFileEngine::WriteRecord()");
			RET_IFNOT_OK(writer.WriteNext(dfRec.Value, &hfRecOut.OffsetOfValue), "DataFileEngine::WriteRecord()");

			hfRecOut.SizeOfValue = dfRec.Value.Size();
			hfRecOut.DataFileId = fileId;
			return Status::OK();
		}

		uint32_t GetFileId() 
		{
			if (!IsOpen()) return -1;
			else return fileId;
		}

		uint8_t GetFileFlag()
		{
			if (!IsOpen()) return -1;
			else return fileFlag;
		}

	protected:
		DataFileReader reader;
		DataFileWriter writer;
		std::string filePath;
		uint8_t fileFlag;
		uint32_t fileId;
	};
} // namespace FreshCask
#endif // __CORE_STORAGEENGINE_HPP__