#include <sstream>
#include <vector>
#include <unordered_map>
#include <system_error>

#include <iostream>
#include <chrono>
using namespace std;

#include <cstdio>
#include <stddef.h>
#include "MurmurHash3.h"

namespace FreshCask {

const uint32_t CurrentMajorVersion = 1;
const uint32_t CurrentMinorVersion = 0;

typedef unsigned char Byte, *BytePtr;

class SmartByteArray 
{
public:
	SmartByteArray() : data_(nullptr), size_(0), refCount(new uint32_t(1)) {}
	SmartByteArray(BytePtr data, uint32_t size) : data_(data), size_(size), refCount(new uint32_t(1)) {}
	SmartByteArray(const SmartByteArray& rhs) : data_(rhs.Data()), size_(rhs.Size()), refCount(rhs.refCount) { ++*refCount; }
	~SmartByteArray() { dispose(); }

	std::string ToString() 
	{
		if (Data() != nullptr)
			return std::string(reinterpret_cast<char*>(Data()), Size());
		else
			return std::string();
	}

	BytePtr Data() const { return data_; };
	uint32_t Size() const { return size_; }

	bool operator==(SmartByteArray& rhs)
	{
		if (rhs.Size() != Size()) return false;
		return !memcmp(rhs.Data(), Data(), Size());
	}

private:
	void dispose()
	{
		if (--*refCount == 0)
		{
			if (data_ != nullptr) delete data_;
			delete refCount;
		}
	}

private:
	BytePtr data_;
	uint32_t size_;
	uint32_t *refCount;
};

class Status 
{
public:
	Status() { code = cOK; }
	Status(int code) : code(code) {}
	Status(int code, std::string message1, std::string message2)
		: code(code), message1(message1), message2(message2) {}

	static Status OK() { return Status(); }
	static Status NotFound(const std::string& message1, const std::string& message2 = "")
	{
		return Status(cNotFound, message1, message2);
	}
	static Status InvalidArgument(const std::string& message1, const std::string& message2 = "")
	{
		return Status(cInvalidArgument, message1, message2);
	}
	static Status IOError(const std::string& message1, const std::string& message2 = "")
	{
		return Status(cIOError, message1, message2);
	}

	Status PushSender(const std::string& sender) { traceback.push_back(sender); return *this; }

	bool IsOK() const { return code == cOK; }
	bool IsNotFound() const { return code == cNotFound; }
	bool IsInvaildArgument() const { return code == cInvalidArgument; }
	bool IsIOError() const { return code == cIOError; }

	std::string ToString() 
	{
		std::stringstream result;

		switch (code)
		{
			case cOK:
				return "OK";

			case cNotFound:
				result << "Not found: ";
				break;

			case cInvalidArgument:
				result << "Invalid Argument: ";
				break;

			case cIOError:
				result << "IO Error: ";
				break;

			default:
				result << "Unkown code (" << code << "): ";
				break;
		}

		result << message1;
		if (message2.length() > 0) result << " - " << message2 << std::endl;
		
		if (traceback.size() > 0) 
		{
			result << "Traceback:" << std::endl;
			for (std::string& sender : traceback)
				result << "- " << sender << std::endl;
		}

		return result.str();
	}

private:
	int code;
	std::string message1, message2;
	std::vector<std::string> traceback;

	enum Code 
	{
		cOK = 0,
		cNotFound = 1,
		cInvalidArgument = 2,
		cIOError = 3
	};
};

namespace DataFile {

const uint32_t DefaultMagicNumber = 0x46444346; // FCDF (FreshCask Data File)
const uint32_t MaxFileSize = (uint32_t)(2000 << 20); // 2 GB

enum Flag 
{
	OlderFile  = 0x0,
	ActiveFile = 0x1,
};

struct Header 
{
	uint32_t MagicNumber;
	uint8_t  MajorVersion;
	uint8_t  MinorVersion;
	Byte 	 Flag;
};

struct Record 
{
	uint32_t CRC32;
	uint32_t TimeStamp;
	SmartByteArray Key;
	SmartByteArray Value;

	Record(uint32_t CRC32, uint32_t TimeStamp, SmartByteArray Key, SmartByteArray Value) :
		CRC32(CRC32), TimeStamp(TimeStamp), Key(Key), Value(Value) {}

	/*uint32_t SizeOfKey;
	uint32_t SizeOfValue;		// 0xFFFFFFFF - delete
	BytePtr  Key;
	BytePtr  Value;*/
};

} // namespace DataFile

namespace HashFile {

struct Record 
{
	uint32_t DataFileId;
	uint32_t SizeOfValue;
	uint32_t OffsetOfValue;
	uint32_t TimeStamp;

	Record() : DataFileId(-1), SizeOfValue(-1), OffsetOfValue(-1), TimeStamp(0) {}

	Record(uint32_t DataFileId, uint32_t SizeOfValue, uint32_t OffsetOfValue, uint32_t TimeStamp) :
		DataFileId(DataFileId), SizeOfValue(SizeOfValue), OffsetOfValue(OffsetOfValue), TimeStamp(TimeStamp) {}
};

typedef uint32_t HashType;
typedef std::unordered_map<HashType, Record> HashMap;

const uint32_t HashSeed = 0x53484346; // FCHS
Status HashFunction(SmartByteArray bar, HashType& out)
{
	if (bar.Size() == 0)
		return Status::InvalidArgument("HashFile::HashFunction()", "bar is null");

	MurmurHash3_x86_32(bar.Data(), bar.Size(), HashSeed, &out);
	return Status::OK();
}

} // namespace DataFile

class ErrnoTranslator 
{
public:	
	ErrnoTranslator(int code) { ec.assign(code, std::generic_category()); }
	operator std::string() { return ec.message(); }

private:
	std::error_condition ec;
};

class Bucket 
{
private:
	class FileManager 
	{
	protected:
		typedef FILE *FilePtr;

	public:
		enum SeekDir {
			Begin   = SEEK_SET,
			Current = SEEK_CUR,
			End     = SEEK_END
		};

	public:
		FileManager() : fp(nullptr) {}
		~FileManager() { Close(); }

		bool IsOpen() { return fp != nullptr; }

		Status Open(std::string filePath)
		{
			if ( nullptr == ( fp = fopen(filePath.c_str(), "ab+") ) )
				return Status::IOError("Bucket::FileManager::Open()", ErrnoTranslator(errno));

			return Status::OK();
		}

		Status Seek(uint32_t offset, SeekDir dir)
		{
			if (!IsOpen())
				return Status::IOError("Bucket::FileManager::Seek()", "File not open");

			if (-1 == fseek(fp, offset, dir))
				return Status::IOError("Bucket::FileManager::Seek()", ErrnoTranslator(errno));

			return Status::OK();
		}

		Status Read(uint32_t size, SmartByteArray out)
		{
			if (!IsOpen())
				return Status::IOError("Bucket::FileManager::Read()", "File not open");

			if (size != fread(out.Data(), sizeof(Byte), size, fp))
				return Status::IOError("Bucket::FileManager::Read()", ErrnoTranslator(errno));

			return Status::OK();
		}

		Status Write(SmartByteArray bar)
		{
			if (!IsOpen())
				return Status::IOError("Bucket::FileManager::Write()", "File not open");

			if (bar.Size() != fwrite(bar.Data(), sizeof(Byte), bar.Size(), fp))
				return Status::IOError("Bucket::FileManager::Write()", ErrnoTranslator(errno));

			return Status::OK();
		}

		Status Close()
		{
			if (!IsOpen())
				return Status::IOError("Bucket::FileManager::Close()", "File not open");

			fclose(fp);
			fp = nullptr;

			return Status::OK();
		}

		Status WriteHeader()
		{
			if (!IsOpen())
				return Status::IOError("Bucket::FileManager::WriteHeader()", "File not open");

			SmartByteArray buffer(new Byte[sizeof(DataFile::Header)], sizeof(DataFile::Header));
			DataFile::Header *header = reinterpret_cast<DataFile::Header*>(buffer.Data());
			header->MagicNumber = DataFile::DefaultMagicNumber;
			header->MajorVersion = CurrentMajorVersion;
			header->MinorVersion = CurrentMinorVersion;
			header->Flag = DataFile::ActiveFile;

			Status s = Write(buffer);
			if (!s.IsOK()) return s.PushSender("Bucket::FileManager::WriteHeader()");

			return Status::OK();
		}

		Status WriteRecord(DataFile::Record record)
		{
			uint32_t bufSize = 4 * sizeof(uint32_t) + record.Key.Size() + record.Value.Size();
			SmartByteArray buffer(new Byte[bufSize], bufSize);
			memcpy(buffer.Data(), &record, 4 * sizeof(uint32_t));
			memcpy(buffer.Data() + 4 * sizeof(uint32_t), record.Key.Data(), record.Key.Size());
			memcpy(buffer.Data() + 4 * sizeof(uint32_t) + record.Key.Size(), record.Value.Data(), record.Value.Size());

			Status s = Write(buffer);
			if (!s.IsOK()) return s.PushSender("Bucket::FileManager::WriteRecord()");
			return Status::OK();
		}

		Status CheckFileValidity()
		{
			if (!IsOpen())
				return Status::IOError("Bucket::FileManager::CheckFileValidity()", "File not open");

			uint32_t curOffset = ftell(fp);
			if (curOffset == 0xFFFFFFFF) // ftell returns -1
				return Status::IOError("Bucket::FileManager::CheckFileValidity()", ErrnoTranslator(errno));

			Status s = Seek(0, Begin);
			if (!s.IsOK()) return s.PushSender("ucket::FileManager::CheckFileValidity()");

			SmartByteArray buffer(new Byte[sizeof(DataFile::Header)], sizeof(DataFile::Header));
			s = Read(sizeof(DataFile::Header), buffer);
			if (!s.IsOK()) return s.PushSender("Bucket::FileManager::CheckFileValidity()");

			DataFile::Header *header = reinterpret_cast<DataFile::Header*>(buffer.Data());
			if (header->MagicNumber != DataFile::DefaultMagicNumber)
				return Status::InvalidArgument("Bucket::FileManager::CheckFileValidity()", "Incorrect magic number");

			if (header->MajorVersion > CurrentMajorVersion)
				return Status::InvalidArgument("Bucket::FileManager::CheckFileValidity()", "DataFile not supported");
			else if (header->MinorVersion > CurrentMinorVersion)
				return Status::InvalidArgument("Bucket::FileManager::CheckFileValidity()", "DataFile not supported");

			s = Seek(curOffset, Begin);
			if (!s.IsOK()) return s.PushSender("Bucket::FileManager::CheckFileValidity()");

			return Status::OK();
		}

		/*Status ReadRecord(uint32_t offset, uint32_t size, DataFile::Record &out)
		{
			Status s = Seek(offset, SeekDir::Begin);
			if (!s.IsOK()) return s;

			std::vector<Byte> buffer(size);
			if (size != fread(&buffer[0], sizeof(Byte), size, fp))
				return Status::IOError("BucketReader::Read()", ErrnoTranslator(errno));

			memcpy(&out, &buffer[0], 4 * sizeof(uint32_t));

			uint32_t SizeOfKey, SizeOfValue;
			memcpy(&SizeOfKey, &buffer[4 * sizeof(uint32_t)], sizeof(uint32_t));
			memcpy(&SizeOfValue, &buffer[4 * sizeof(uint32_t) + uint32_t + SizeOfKey], sizeof(uint32_t));

			out.Key = SmartByteArray(new Byte[SizeOfKey], SizeOfKey);
			memcpy(out.Key.data(), &buffer[4 * sizeof(uint32_t) + uint32_t], SizeOfKey);

			out.Value = SmartByteArray(new Byte[SizeOfValue], SizeOfValue);
			memcpy(out.Value.data(), &buffer[4 * sizeof(uint32_t) + uint32_t + SizeOfKey + uint32_t], SizeOfValue);

			return Status::OK();
		}*/

	protected:
		FilePtr fp;
	};

public:
	Bucket() {}
	~Bucket() { Close(); }

	Status Open(std::string filePath) 
	{ 
		Status s = fileMan.Open(filePath);
		if (!s.IsOK()) return s.PushSender("Bucket::Open()");

		if (!fileMan.CheckFileValidity().IsOK())
			return fileMan.WriteHeader().PushSender("Bucket::Open()");

		return Status::OK();
	}

	Status Close() { return fileMan.Close().PushSender("Bucket::Close()"); }

	Status Get(SmartByteArray key, SmartByteArray out) { return Status::OK(); }
	
	Status Put(SmartByteArray key, SmartByteArray value) 
	{
		HashFile::HashType hash;
		Status s = HashFile::HashFunction(key, hash);
		if (!s.IsOK()) return s.PushSender("Bucket::Put()");
		hashMap[hash] = HashFile::Record(0, value.Size(), 0, 0);

		return fileMan.WriteRecord(DataFile::Record(0, 0, key, value)).PushSender("Bucket::Put()");
	}

private:
	HashFile::HashMap hashMap;
	FileManager fileMan;
};

} // namespace FreshCask

int main()
{
	FreshCask::Bucket bc;
	FreshCask::Status s = bc.Open("test.fc");

	if (!s.IsOK()) std::cout << s.ToString() << std::endl;

	FreshCask::SmartByteArray Key(new FreshCask::Byte[4], 4);
	FreshCask::SmartByteArray Value(new FreshCask::Byte[1024], 1024);
	
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 2000000; i++)
	{	
		bc.Put(Key, Value);
	}

	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
	uint64_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "done in " << duration << " ms" << std::endl;
}