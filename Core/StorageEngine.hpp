#ifndef __CORE_STORAGEENGINEPOOL_HPP__
#define __CORE_STORAGEENGINEPOOL_HPP__

#include <map>
#include <memory>

#include <Core/DataFileEngine.hpp>

namespace FreshCask
{
	class StorageEngine
	{
	private:
		typedef DataFileEngine* DataFileEnginePtr;
		typedef std::map<uint32_t, std::shared_ptr<DataFileEngine>> DataFileEngineMap;

	public:
		StorageEngine(std::string bucketDir) : bucketDir(bucketDir), lastFileId(0), dfActiveEngine(std::pair<uint32_t, DataFileEnginePtr>(-1, nullptr)) {}
		~StorageEngine() { Close(); }

		Status Open()
		{
			if (!IsDirExist(bucketDir))
				return Status::NotFound("StorageEngine::Open()", "Directory doesn't exist.");

			RET_IFNOT_OK(ListDir(bucketDir, [&](std::string filePath) -> Status {
				if (EndWith(filePath, DataFile::FileNameSuffix))
				{
					std::shared_ptr<DataFileEngine> engine = std::shared_ptr<DataFileEngine>(new DataFileEngine(filePath));
					RET_IFNOT_OK(engine->Open(), "StorageEngine::ProcessFile()");
					dfEngineMap[engine->GetFileId()] = engine;

					uint32_t curFileId = engine->GetFileId();
					if (engine->GetFileFlag() & DataFile::Flag::ActiveFile)
						dfActiveEngine = std::pair<uint32_t, DataFileEnginePtr>(curFileId, engine.get());

					if (curFileId > lastFileId) lastFileId = curFileId;
				}
				else if (EndWith(filePath, HintFile::FileNameSuffix))
				{
					// TODO: load hint file
					
				}
				else return Status::OK();
			}), "StorageEngine::Open()");

			
			return Status::OK();
		}

		Status Close()
		{
			for (auto &engine : dfEngineMap)
				engine.second->Close();

			return Status::OK();
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
			dfEngineMap[lastFileId] = std::shared_ptr<DataFileEngine>(new DataFileEngine(genFilePath(++lastFileId)));
			RET_IFNOT_OK(dfEngineMap[lastFileId]->Create(lastFileId), "StorageEngine::WriteRecord()");

			dfActiveEngine = std::pair<uint32_t, DataFileEnginePtr>(lastFileId, dfEngineMap[lastFileId].get());
			RET_BY_SENDER(dfActiveEngine.second->WriteRecord(dfRec, hfRecOut), "StorageEngine::WriteRecord()");
		}

		Status Merge()
		{
			return Status::OK();
		}

	private:
		std::string genFilePath(uint32_t fileId)
		{
			std::stringstream stream;
#ifdef WIN32
			stream << bucketDir << "\\" << fileId << DataFile::FileNameSuffix;
			return stream.str();
#else
#endif
		}

	private:
		std::string bucketDir;

		DataFileEngineMap dfEngineMap;
		std::pair<uint32_t, DataFileEnginePtr> dfActiveEngine;
		uint32_t lastFileId;
	};
} // namespace FreshCask
#endif // __CORE_STORAGEENGINEPOOL_HPP__