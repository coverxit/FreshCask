#ifndef __CORE_STORAGEENGINE_HPP__
#define __CORE_STORAGEENGINE_HPP__

#include <map>
#include <memory>

#include <Core/FileStream.hpp>
#include <Core/DataFileEngine.hpp>
#include <Core/HintFileEngine.hpp>

namespace FreshCask
{
	class StorageEngine
	{
	private:
		typedef DataFileEngine* DataFileEnginePtr;
		typedef std::map<uint32_t, std::shared_ptr<DataFileEngine>> DataFileEngineMap;

	public:
		StorageEngine(std::string bucketDir, HashFile::HashTree& hashTree) 
			: bucketDir(bucketDir), hashTree(hashTree), lastFileId(0), 
#ifndef _M_CEE // fuck C++/CLI!!!
			dfActiveEngine(std::pair<uint32_t, DataFileEnginePtr>((uint32_t)-1, nullptr)) {}
#else
			dfActiveEngine(std::pair<uint32_t, DataFileEnginePtr>((uint32_t)-1, __nullptr)) {}
#endif
		~StorageEngine() { Close(true); }

		Status Open()
		{
			if (!IsDirExist(bucketDir))
				return Status::NotFound("StorageEngine::Open()", "Directory doesn't exist.");

			RET_IFNOT_OK(ListDir(bucketDir, [&](const std::string &filePath) -> Status {
				if (EndWith(filePath, DataFile::FileNameSuffix))
				{
					std::shared_ptr<DataFileEngine> engine = std::shared_ptr<DataFileEngine>(new DataFileEngine(filePath));
					RET_IFNOT_OK(engine->Open(), "StorageEngine::Open()::ProcessFile()");
					dfEngineMap[engine->GetFileId()] = engine;

					uint32_t curFileId = engine->GetFileId();
					if (engine->GetFileFlag() & DataFile::Flag::ActiveFile)
						dfActiveEngine = std::pair<uint32_t, DataFileEnginePtr>(curFileId, engine.get());

					if (curFileId > lastFileId) lastFileId = curFileId;
				}
				else if (EndWith(filePath, HintFile::FileNameSuffix))
				{
					// load hint file
					HintFileEngine engine(HintFileEngine::OpenMode::Read, filePath);
					RET_IFNOT_OK(engine.Open(), "StorageEngine::Open()::ProcessFile()");

					while (true)
					{
						HintFile::Record hfRec;
						Status s = engine.ReadRecord(hfRec);

						if (!s.IsOK())
						{
							if (s.IsEndOfFile()) break;
							else RET_BY_SENDER(s, "StorageEngine::Open()::ProcessFile()");
						}
							
						//HashFile::HashType hash;
						//RET_IFNOT_OK(HashFile::HashFunction(hfRec.Key, hash), "StorageEngine::Open()::ProcessFile()");
						hashTree[hfRec.Key] = HashFile::Record(hfRec.Header.DataFileId, hfRec.Header.SizeOfValue, hfRec.Header.OffsetOfValue, hfRec.Header.TimeStamp);
					}

					RET_IFNOT_OK(engine.Close(), "StorageEngine::Open()::ProcessFile()");
					// delete hint file when re-creation done
					// RET_IFNOT_OK(RemoveFile(filePath), "StorageEngine::Open()::ProcessFile()");
				}

				RET_BY_SENDER(Status::OK(), "StorageEngine::Open()::ProcessFile()");
			}), "StorageEngine::Open()");

			RET_BY_SENDER(Status::OK(), "StorageEngine::Open()");
		}

		Status Close(bool makeHintFile)
		{
			if (dfEngineMap.size() > 0)
			{
				for (auto &engine : dfEngineMap)
					RET_IFNOT_OK(engine.second->Close(), "StorageEngine::Close()");

				dfEngineMap.clear();
				if (makeHintFile) RET_BY_SENDER(CreateHintFile(bucketDir, hashTree), "StorageEngine::Close()");
			}

			// that means already closed
			RET_BY_SENDER(Status::OK(), "StorageEngine::Close()");
		}

		Status ReadValue(HashFile::Record hfRec, SmartByteArray &valueOut)
		{
			DataFileEngineMap::iterator it = dfEngineMap.find(hfRec.DataFileId);
			if (it == dfEngineMap.end())
				return Status::NotFound("StorageEngine::ReadValue()", "Invalid File ID.");

			RET_BY_SENDER(it->second->ReadValue(hfRec, valueOut), "StorageEngine::ReadValue()");
		}

		/*Status ReadRecord(HashFile::Record hfRec, DataFile::Record &dfRecOut)
		{
			DataFileEngineMap::iterator it = engineMap.find(hfRec.DataFileId);
			if (it == engineMap.end())
				return Status::NotFound("StorageEngine::ReadRecord()", "Invalid File ID.");

			RET_BY_SENDER(it->second->ReadRecord(hfRec, dfRecOut), "StorageEngine::ReadRecord()");
		}*/

		Status WriteRecord(DataFile::Record dfRec, HashFile::Record &hfRecOut)
		{
			if (dfActiveEngine.first != -1 && dfActiveEngine.second != nullptr)
			{
				Status ret = dfActiveEngine.second->WriteRecord(dfRec, hfRecOut);
				if (!ret.IsNoFreeSpace())
					RET_BY_SENDER(ret, "StorageEngine::WriteRecord()");
			}

			// create new data file
			dfEngineMap[lastFileId] = std::shared_ptr<DataFileEngine>(new DataFileEngine(genDataFilePath(++lastFileId)));
			RET_IFNOT_OK(dfEngineMap[lastFileId]->Create(lastFileId), "StorageEngine::WriteRecord()");

			dfActiveEngine = std::pair<uint32_t, DataFileEnginePtr>(lastFileId, dfEngineMap[lastFileId].get());
			RET_BY_SENDER(dfActiveEngine.second->WriteRecord(dfRec, hfRecOut), "StorageEngine::WriteRecord()");
		}

	private:
		std::string genDataFilePath(uint32_t fileId)
		{
			std::stringstream stream;
#ifdef WIN32
			stream << bucketDir << "\\" << fileId << DataFile::FileNameSuffix;
			return stream.str();
#else
#endif
		}

	public:
		static Status CreateHintFile(const std::string& bucketDir, const HashFile::HashTree &hashTree)
		{
			HintFileEngine engine(HintFileEngine::OpenMode::Write, genHintFilePath(bucketDir));
			RET_IFNOT_OK(engine.Open(), "StorageEngine::CreateHintFile()");

			for (auto& item : hashTree)
			{
				HintFile::Record hfRec(item.first);
				hfRec.Header.TimeStamp = item.second.TimeStamp;
				hfRec.Header.SizeOfValue = item.second.SizeOfValue;
				hfRec.Header.OffsetOfValue = item.second.OffsetOfValue;
				hfRec.Header.DataFileId = item.second.DataFileId;
				RET_IFNOT_OK(engine.WriteRecord(hfRec), "StorageEngine::CreateHintFile()");
			}

			RET_BY_SENDER(engine.Close(), "StorageEngine::CreateHintFile()");
		}

		static std::string genHintFilePath(const std::string &bucketDir)
		{
			std::stringstream stream;
#ifdef WIN32
			// TODO: create multi hint files
			stream << bucketDir << "\\_bc" << HintFile::FileNameSuffix;
			return stream.str();
#else
#endif
		}

	private:
		std::string bucketDir;
		HashFile::HashTree& hashTree;

		DataFileEngineMap dfEngineMap;
		std::pair<uint32_t, DataFileEnginePtr> dfActiveEngine;
		uint32_t lastFileId;
	};
} // namespace FreshCask
#endif // __CORE_STORAGEENGINE_HPP__