#ifndef __CORE_BUCKETMANAGER_HPP__
#define __CORE_BUCKETMANAGER_HPP__

#include <Core/Config.h>

#include <Util/Status.hpp>
#include <Util/SmartByteArray.hpp>

#include <Core/StorageEnginePool.hpp>

namespace FreshCask
{
	class BucketManager
	{
	public:
		BucketManager() {}
		~BucketManager() { Close(); }

		Status Open(std::string bucketPath)
		{
			enginePool = std::shared_ptr<StorageEnginePool>(new StorageEnginePool(bucketPath));
			RET_BY_SENDER(enginePool->Open(), "BucketManager::Open()");
//			RET_BY_SENDER(storeEngine.Open(filePath, DataFile::ActiveFile), "Bucket::Open()");
//			return Status::OK();
		}

		Status Close() 
		{ 
			RET_BY_SENDER(enginePool->Close(), "BucketManager::Close()");
//			RET_BY_SENDER(storeEngine.Close(), "Bucket::Close()"); 
//			return Status::OK();
		}

		Status Get(SmartByteArray key, SmartByteArray &out)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Get()");

			HashFile::HashMap::iterator it = hashMap.find(hash);
			if (it == hashMap.end())
				RET_BY_SENDER(Status::NotFound("Key doesn't exist"), "BucketManager::Get()");

			DataFile::Record dataRec;
			RET_IFNOT_OK(enginePool->ReadRecord(it->second, dataRec), "BucketManager::Get()");
//			RET_IFNOT_OK(storeEngine.ReadRecord(it->second.OffsetOfValue, dataRec), "Bucket::Get()");
			out = dataRec.Value;
			return Status::OK();
		}

		Status Put(SmartByteArray key, SmartByteArray &value)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Put()");

			HashFile::Record hashRec;
			RET_IFNOT_OK(enginePool->WriteRecord(DataFile::Record(key, value), hashRec), "BucketManager::Put()");
//			RET_IFNOT_OK(storeEngine.WriteRecord(DataFile::Record(key, value), hashRec), "Bucket::Put()");
			
			hashMap[hash] = hashRec;
			return Status::OK();
		}

	private:
		HashFile::HashMap hashMap;
		std::shared_ptr<StorageEnginePool> enginePool;
	};

} // namespace FreshCask
#endif // __CORE_BUCKETMANAGER_HPP__