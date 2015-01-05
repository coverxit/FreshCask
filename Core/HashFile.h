#ifndef __CORE_HASHFILE_H__
#define __CORE_HASHFILE_H__

#include <map>

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

		typedef std::map<SmartByteArray, Record> HashTree;
	} // namespace HashFile
} // namespace FreshCask

#endif // __CORE_HASHFILE_H__