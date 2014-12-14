#ifndef __CORE_STORAGEENGINEPOOL_HPP__
#define __CORE_STORAGEENGINEPOOL_HPP__

#include <vector>
#include <memory>

#include <Core/StorageEngine.hpp>

namespace FreshCask
{
	class StorageEnginePool
	{
	public:
		StorageEnginePool(std::string bucketDir) : bucketDir(bucketDir) {}
		~StorageEnginePool() { Close(); }

		Status Open()
		{
			if (!IsDirExist(bucketDir))
				return Status::NotFound("StorageEnginePool::Open()", "Directory doesn't exist.");
		}

		Status Close()
		{
			for (auto &engine : engines)
				engine->Close();
		}

		Status ReadRecord(uint32_t fileId, uint32_t offset, DataFile::Record &out)
		{

		}

		Status WriteRecord(DataFile::Record dfRec, HashFile::Record &hfRecOut)
		{

		}

	private:
		std::string bucketDir;
		std::vector<std::shared_ptr<StorageEngine>> engines;
	};
} // namespace FreshCask
#endif // __CORE_STORAGEENGINEPOOL_HPP__