#ifndef __CORE_HASHFILE_H__
#define __CORE_HASHFILE_H__

#include <map>

#include <Algorithm/MurmurHash3.h>

namespace FreshCask
{
	namespace HashFile
	{
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
		typedef std::map<HashType, std::pair<SmartByteArray, Record>> HashMap;

		Status HashFunction(const SmartByteArray &bar, HashType& out)
		{
			if (bar.Size() == 0)
				RET_BY_SENDER(Status::InvalidArgument("Input is null"), "HashFile::HashFunction()");

			MurmurHash3_x86_32(bar.Data(), bar.Size(), HashSeed, &out);
			RET_BY_SENDER(Status::OK(), "HashFile::HashFunction()");
		}
	} // namespace HashFile
} // namespace FreshCask

#endif // __CORE_HASHFILE_H__