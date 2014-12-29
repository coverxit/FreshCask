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
			OlderFile = 0x1,
			ActiveFile = 0x2,
		};

		struct Header
		{
			uint32_t MagicNumber;
			uint8_t  MajorVersion;
			uint8_t  MinorVersion;
			uint32_t FileId;
			uint8_t  Flag;
			uint8_t  Reserved1;
			uint8_t  Reserved2;
		};

		struct RecordHeader
		{
			uint32_t TimeStamp;
			uint32_t SizeOfKey;
			uint32_t SizeOfValue;

			RecordHeader() : TimeStamp(0), SizeOfKey(-1), SizeOfValue(-1) {}
			RecordHeader(uint32_t SizeOfKey, uint32_t SizeOfValue) : TimeStamp(-1), SizeOfKey(SizeOfKey), SizeOfValue(SizeOfValue) {}
		};

		struct Record
		{
			uint32_t CRC32;
			RecordHeader Header;

			SmartByteArray Key;
			SmartByteArray Value;

			Record() {}
			Record(SmartByteArray Key, SmartByteArray Value) : CRC32(-1), Key(Key), Value(Value), Header(Key.Size(), Value.Size()) {}
			uint32_t GetSize() { return sizeof(CRC32) + sizeof(Header) + Key.Size() + Value.Size(); }
		};
	} // namespace DataFile
} // namespace FreshCask

#endif // __CORE_DATAFILE_H__