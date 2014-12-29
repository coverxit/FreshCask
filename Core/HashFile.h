#ifndef __CORE_HASHFILE_H__
#define __CORE_HASHFILE_H__

#include <unordered_map>

#include <Algorithm/MurmurHash3.h>

namespace FreshCask
{
	namespace HashFile
	{
		struct Record
		{
			uint32_t DataFileId;
			//uint32_t SizeOfValue;
			uint32_t OffsetOfRecord;
			uint32_t TimeStamp;

			Record() : DataFileId(-1), OffsetOfRecord(-1), TimeStamp(0) {}

			Record(uint32_t DataFileId, uint32_t OffsetOfRecord, uint32_t TimeStamp) :
				DataFileId(DataFileId), OffsetOfRecord(OffsetOfRecord), TimeStamp(TimeStamp) {}
		};

		typedef uint32_t HashType;
		typedef std::unordered_map<HashType, Record> HashMap;

		Status HashFunction(SmartByteArray bar, HashType& out)
		{
			if (bar.Size() == 0)
				return Status::InvalidArgument("HashFile::HashFunction()", "input is null");

			MurmurHash3_x86_32(bar.Data(), bar.Size(), HashSeed, &out);
			return Status::OK();
		}
	} // namespace HashFile
} // namespace FreshCask

#endif // __CORE_HASHFILE_H__