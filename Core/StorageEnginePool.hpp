#ifndef __CORE_STORAGEENGINEPOOL_HPP__
#define __CORE_STORAGEENGINEPOOL_HPP__

#include <map>
#include <memory>

#include <Core/StorageEngine.hpp>

namespace FreshCask
{
	class StorageEnginePool
	{
	private:
		typedef StorageEngine* StorageEnginePtr;
		typedef std::map<uint32_t, std::shared_ptr<StorageEngine>> EngineMap;

	public:
		StorageEnginePool(std::string bucketDir) : bucketDir(bucketDir), lastFileId(0), activeEngine(std::pair<uint32_t, StorageEnginePtr>(-1, nullptr)) {}
		~StorageEnginePool() { Close(); }

		Status Open()
		{
			if (!IsDirExist(bucketDir))
				return Status::NotFound("StorageEnginePool::Open()", "Directory doesn't exist.");

			RET_IFNOT_OK(ListDir(bucketDir, [&](std::string filePath) -> Status {
				std::shared_ptr<StorageEngine> engine = std::shared_ptr<StorageEngine>(new StorageEngine(filePath));
				RET_IFNOT_OK(engine->Open(), "StorageEnginePool::ProcessFile()");
				engineMap[engine->GetFileId()] = engine;

				uint32_t curFileId = engine->GetFileId();
				if (engine->GetFileFlag() & DataFile::Flag::ActiveFile)
					activeEngine = std::pair<uint32_t, StorageEnginePtr>(curFileId, engine.get());

				if (curFileId > lastFileId) lastFileId = curFileId;
				return Status::OK();
			}), "StorageEnginePool::Open()");

			// TODO: load hint file
			return Status::OK();
		}

		Status Close()
		{
			for (auto &engine : engineMap)
				engine.second->Close();

			return Status::OK();
		}

		Status ReadRecord(HashFile::Record hfRec, DataFile::Record &dfRecOut)
		{
			EngineMap::iterator it = engineMap.find(hfRec.DataFileId);
			if (it == engineMap.end())
				return Status::NotFound("StorageEnginePool::ReadRecord()", "Invalid File ID.");

			RET_BY_SENDER(it->second->ReadRecord(hfRec, dfRecOut), "StorageEnginePool::ReadRecord()");
		}

		Status WriteRecord(DataFile::Record dfRec, HashFile::Record &hfRecOut)
		{
			if (activeEngine.first != -1 && activeEngine.second != nullptr)
			{
				Status ret = activeEngine.second->WriteRecord(dfRec, hfRecOut);
				if (!ret.IsNoFreeSpace())
					RET_BY_SENDER(ret, "StorageEnginePool::WriteRecord()");
			}
	
			// create new data file
			engineMap[lastFileId] = std::shared_ptr<StorageEngine>(new StorageEngine(genFilePath(++lastFileId)));
			RET_IFNOT_OK(engineMap[lastFileId]->Create(lastFileId), "StorageEnginePool::WriteRecord()");

			activeEngine = std::pair<uint32_t, StorageEnginePtr>(lastFileId, engineMap[lastFileId].get());
			RET_BY_SENDER(activeEngine.second->WriteRecord(dfRec, hfRecOut), "StorageEnginePool::WriteRecord()");
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

		EngineMap engineMap;
		std::pair<uint32_t, StorageEnginePtr> activeEngine;
		uint32_t lastFileId;
	};
} // namespace FreshCask
#endif // __CORE_STORAGEENGINEPOOL_HPP__