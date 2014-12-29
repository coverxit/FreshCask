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
		}

		Status Close() 
		{ 
			RET_BY_SENDER(enginePool->Close(), "BucketManager::Close()");
		}

		Status Get(SmartByteArray key, SmartByteArray &out)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Get()");

			HashFile::HashMap::iterator it = hashMap.find(hash);
			if (it == hashMap.end())
				RET_BY_SENDER(Status::NotFound("Key doesn't exist"), "BucketManager::Get()");

			DataFile::Record dataRec;
			RET_IFNOT_OK(enginePool->ReadRecord(it->second.second, dataRec), "BucketManager::Get()");
			out = dataRec.Value;
			return Status::OK();
		}
		
		Status Put(SmartByteArray key, SmartByteArray &value)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Put()");

			HashFile::Record hashRec;
			RET_IFNOT_OK(enginePool->WriteRecord(DataFile::Record(key, value), hashRec), "BucketManager::Put()");
			
			hashMap[hash] = std::pair<SmartByteArray, HashFile::Record>(key, hashRec);
			return Status::OK();
		}

		Status Delete(SmartByteArray key)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Delete()");

			HashFile::HashMap::iterator it = hashMap.find(hash);
			if (it == hashMap.end())
				RET_BY_SENDER(Status::NotFound("Key doesn't exist"), "BucketManager::Delete()");
			hashMap.erase(it);

			RET_BY_SENDER(Put(key, SmartByteArray::Null()), "BucketManager::Delete()");
		}

		Status Enumerate(std::function<Status(SmartByteArray, SmartByteArray&)> func)
		{
			for (auto& item : hashMap)
			{
				SmartByteArray value;
				RET_IFNOT_OK(Get(item.second.first, value), "BucketManager::Enumerate()");
				RET_IFNOT_OK(func(item.second.first, value), "BucketManager::Enumerate()");
			}
			return Status::OK();
		}

	private:
		HashFile::HashMap hashMap;
		std::shared_ptr<StorageEnginePool> enginePool;
	};

} // namespace FreshCask
#endif // __CORE_BUCKETMANAGER_HPP__