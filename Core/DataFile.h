#ifndef __CORE_DATAFILE_H__
#define __CORE_DATAFILE_H__

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
			uint16_t Reserved;
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

			Record(const SmartByteArray &Key, const SmartByteArray &Value) : CRC32(-1), Key(Key), Value(Value), Header(Key.Size(), Value.Size()) {}
			uint32_t GetSize() { return sizeof(CRC32) + sizeof(Header) + Key.Size() + Value.Size(); }
		};

		class CRC32
		{
		public:
			typedef uint32_t CRCType;

			static CRCType Get(const SmartByteArray &bar)
			{
				CRCType crc = 0xFFFFFFFF;
				uint32_t len = bar.Size();
				BytePtr buffer = bar.Data();

				initTable();

				while (len--)
					crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *buffer++];

				return crc ^ 0xFFFFFFFF;
			}

			static CRCType CalcDataFileRecord(DataFile::Record dfRec)
			{
				SmartByteArray buffer(sizeof(DataFile::RecordHeader) + dfRec.Key.Size() + dfRec.Value.Size());
				BytePtr ptr = buffer.Data();

				memcpy(ptr, &dfRec.Header, sizeof(DataFile::RecordHeader));
				memcpy(ptr + sizeof(DataFile::RecordHeader), dfRec.Key.Data(), dfRec.Key.Size());
				memcpy(ptr + sizeof(DataFile::RecordHeader) + dfRec.Key.Size(), dfRec.Value.Data(), dfRec.Value.Size());

				return Get(buffer);
			}

		private:
			static void initTable()
			{
				bool init = false;

				if (init) return;

				init = true;
				for (int i = 0; i < 256; i++)
				{
					CRCType crc = i;
					for (int j = 0; j < 8; j++)
					{
						if (crc & 1)
							crc = (crc >> 1) ^ 0xEDB88320;
						else
							crc = crc >> 1;
					}
					CRCTable[i] = crc;
				}
			}
			static CRCType CRCTable[256];
		};
		CRC32::CRCType CRC32::CRCTable[256] = { 0 };
	} // namespace DataFile
} // namespace FreshCask

#endif // __CORE_DATAFILE_H__