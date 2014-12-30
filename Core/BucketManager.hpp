#ifndef __CORE_BUCKETMANAGER_HPP__
#define __CORE_BUCKETMANAGER_HPP__

#include <Core/Config.h>

#include <Util/Status.hpp>
#include <Util/SmartByteArray.hpp>

#include <Core/StorageEngine.hpp>

namespace FreshCask
{
	class BucketManager
	{
	public:
		BucketManager() {}
		~BucketManager() { Close(); }

		Status Open(std::string bucketPath)
		{
			engine = std::shared_ptr<StorageEngine>(new StorageEngine(bucketPath));
			RET_BY_SENDER(engine->Open(), "BucketManager::Open()");
		}

		Status Close() 
		{ 
			RET_IFNOT_OK(Merge(), "BucketManager::Close()");
			RET_BY_SENDER(engine->Close(), "BucketManager::Close()");
		}

		Status Get(SmartByteArray key, SmartByteArray &out)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Get()");

			HashFile::HashMap::iterator it = hashMap.find(hash);
			if (it == hashMap.end())
				RET_BY_SENDER(Status::NotFound("Key doesn't exist"), "BucketManager::Get()");

			RET_BY_SENDER(engine->ReadValue(it->second.second, out), "BucketManager::Get()");
			//DataFile::Record dataRec;
			//RET_IFNOT_OK(engine->ReadRecord(it->second.second, dataRec), "BucketManager::Get()");
			//out = dataRec.Value;
			//return Status::OK();
		}

		Status Put(SmartByteArray key, SmartByteArray &value)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Put()");

			HashFile::Record hashRec;
			RET_IFNOT_OK(engine->WriteRecord(DataFile::Record(key, value), hashRec), "BucketManager::Put()");
			
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

		Status Enumerate(std::function<bool(SmartByteArray, SmartByteArray&)> func)
		{
			for (auto& item : hashMap)
			{
				SmartByteArray value;
				RET_IFNOT_OK(Get(item.second.first, value), "BucketManager::Enumerate()");
				RET_IFNOT_OK(Status(func(item.second.first, value)), "BucketManager::Enumerate()");
			}
			return Status::OK();
		}

		Status Merge()
		{
			RET_BY_SENDER(engine->Merge(), "BucketManager::Merge()");
		}

	private:
		HashFile::HashMap hashMap;
		std::shared_ptr<StorageEngine> engine;
	};

} // namespace FreshCask
#endif // __CORE_BUCKETMANAGER_HPP__