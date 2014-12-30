#ifndef __CORE_HINTFILE_H__
#define __CORE_HINTFILE_H__

#include <map>

#include <Algorithm/MurmurHash3.h>

namespace FreshCask
{
	namespace HintFile
	{
		struct Header
		{
			uint32_t  MagicNumber;
			uint8_t   MajorVersion;
			uint8_t   MinorVersion;
			uint16_t  Reserved;
		};

		struct RecordHeader
		{
			uint32_t DataFileId;
			uint32_t TimeStamp;
			uint32_t SizeOfKey;
			uint32_t SizeOfValue;
			uint32_t OffsetOfValue;

			RecordHeader() : DataFileId(-1), SizeOfKey(-1), SizeOfValue(-1), OffsetOfValue(-1), TimeStamp(0) {}
			RecordHeader(uint32_t SizeOfKey) : TimeStamp(-1), DataFileId(-1), SizeOfKey(SizeOfKey), SizeOfValue(-1), OffsetOfValue(-1) {}
		};

		struct Record
		{
			RecordHeader Header;
			SmartByteArray Key;

			Record() {}
			Record(const SmartByteArray& Key) : Key(Key), Header(Key.Size()) {}
		};
	} // namespace HintFile
} // namespace FreshCask

#endif // __CORE_HINTFILE_H__