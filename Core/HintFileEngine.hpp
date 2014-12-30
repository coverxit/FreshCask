#ifndef __CORE_HINTSTORAGEENGINE_HPP__
#define __CORE_HINTSTORAGEENGINE_HPP__

#include <Core/HintFile.h>
#include <Core/HintFileStream.hpp>

namespace FreshCask
{
	class HintFileEngine
	{
	public:
		enum OpenMode {
			Read = 0,
			Write = 1,
		};

	public:
		HintFileEngine(OpenMode openMode, std::string filePath) : filePath(filePath), openMode(openMode) {}
		~HintFileEngine() { Close(); }

		bool IsOpen()
		{
			switch (openMode)
			{
			case Read:
				return reader != nullptr && reader->IsOpen();

			case Write:
				return writer != nullptr && writer->IsOpen();

			default:
				return false;
			}
		}

		Status Open()
		{
			switch (openMode)
			{
			case Read:
				if (!IsFileExist(filePath))
					RET_BY_SENDER(Status::NotFound("File doesn't exist."), "HintFileEngine::Open()");

				reader = std::shared_ptr<HintFileReader>(new HintFileReader(filePath));
				RET_BY_SENDER(readOpen(), "HintFileEngine::Open()");

			case Write:
				writer = std::shared_ptr<HintFileWriter>(new HintFileWriter(filePath));
				RET_BY_SENDER(writeOpen(), "HintFileEngine::Open()");

			default:
				RET_BY_SENDER(Status::InvalidArgument("openMode is invalid."), "HintFileEngine::Open()");
			}
		}

		Status Close()
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open."), "HintFileEngine::Close()");

			switch (openMode)
			{
			case Read:
				reader->Close();
				break;

			case Write:
				writer->Close();
				break;

			default:
				break;
			}

			return Status::OK();
		}

		Status ReadRecord(HintFile::Record &hfRecOut)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open."), "HintFileEngine::ReadRecord()");

			RET_IFNOT_OK(reader->ReadNext(SmartByteArray((BytePtr)&hfRecOut.Header, sizeof(HintFile::RecordHeader))), "HintFileEngine::ReadRecord()");

			hfRecOut.Key = SmartByteArray(hfRecOut.Header.SizeOfKey);
			RET_BY_SENDER(reader->ReadNext(hfRecOut.Key), "HintFileEngine::ReadRecord()");
		}

		Status WriteRecord(HintFile::Record hfRec)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("File not open."), "HintFileEngine::WriteRecord()");

			hfRec.Header.TimeStamp = GetTimeStamp();
			RET_IFNOT_OK(writer->WriteNext(SmartByteArray((BytePtr)&hfRec.Header, sizeof(HintFile::RecordHeader))), "HintFileEngine::WriteRecord()");
			RET_BY_SENDER(writer->WriteNext(hfRec.Key), "HintFileEngine::WriteRecord()");
		}

	private:
		Status readOpen()
		{
			RET_IFNOT_OK(reader->Open(), "HintFileEngine::Open()");

			// check header
			SmartByteArray buffer(new Byte[sizeof(HintFile::Header)], sizeof(HintFile::Header));
			RET_IFNOT_OK(reader->ReadNext(buffer), "HintFileEngine::readOpen()");

			HintFile::Header *header = reinterpret_cast<HintFile::Header*>(buffer.Data());
			if (header->MagicNumber != HintFile::DefaultMagicNumber)
				RET_BY_SENDER(Status::InvalidArgument("Incorrect magic number"), "HintFileEngine::readOpen()");
			if (header->MajorVersion > CurrentMajorVersion)
				RET_BY_SENDER(Status::NotSupported("DataFile not supported"), "HintFileEngine::readOpen()");
			else if (header->MinorVersion > CurrentMinorVersion)
				RET_BY_SENDER(Status::NotSupported("DataFile not supported"), "HintFileEngine::readOpen()");

			return Status::OK();
		}

		Status writeOpen()
		{
			RET_IFNOT_OK(writer->Open(), "HintFileEngine::writeOpen()");

			// writer header
			SmartByteArray buffer(sizeof(HintFile::Header));
			HintFile::Header *header = reinterpret_cast<HintFile::Header*>(buffer.Data());
			header->MagicNumber = HintFile::DefaultMagicNumber;
			header->MajorVersion = CurrentMajorVersion;
			header->MinorVersion = CurrentMinorVersion;
			header->Reserved = 0x0;

			RET_BY_SENDER(writer->WriteNext(buffer), "HintFileEngine::writeOpen()");
		}

	protected:
		std::shared_ptr<HintFileReader> reader;
		std::shared_ptr<HintFileWriter> writer;

		OpenMode openMode;
		std::string filePath;
	};
} // namespace FreshCask
#endif // __CORE_HINTSTORAGEENGINE_HPP__