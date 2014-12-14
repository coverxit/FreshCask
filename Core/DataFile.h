#ifndef __CORE_DATAFILE_H__
#define __CORE_DATAFILE_H__

#include <unordered_map>

#include <Algorithm/MurmurHash3.h>

namespace FreshCask
{
	namespace DataFile
	{
		enum Flag
		{
			OlderFile = 0x0,
			ActiveFile = 0x1,
		};

		struct Header
		{
			uint32_t MagicNumber;
			uint8_t  MajorVersion;
			uint8_t  MinorVersion;
			uint8_t  Flag;
			uint8_t  Reserved;
		};

		struct RecordHeader
		{
			uint32_t CRC32;
			uint32_t TimeStamp;
			uint32_t SizeOfKey;
			uint32_t SizeOfValue;

			RecordHeader() : CRC32(0), TimeStamp(0), SizeOfKey(-1), SizeOfValue(-1) {}
			RecordHeader(uint32_t SizeOfKey, uint32_t SizeOfValue) : SizeOfKey(SizeOfKey), SizeOfValue(SizeOfValue) {}
		};

		struct Record
		{
			RecordHeader Header;

			SmartByteArray Key;
			SmartByteArray Value;

			Record() {}
			Record(SmartByteArray Key, SmartByteArray Value) : Key(Key), Value(Value), Header(Key.Size(), Value.Size()) {}
		};
	} // namespace DataFile
} // namespace FreshCask

#endif // __CORE_DATAFILE_H__