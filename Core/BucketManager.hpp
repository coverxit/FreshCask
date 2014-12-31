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

		Status Open(const std::string &_bucketDir)
		{
			bucketDir = _bucketDir;
			engine = std::shared_ptr<StorageEngine>(new StorageEngine(bucketDir, hashMap));
			RET_BY_SENDER(engine->Open(), "BucketManager::Open()");
		}

		Status Close() 
		{
			RET_BY_SENDER(engine->Close(), "BucketManager::Close()");
		}

		Status Get(const SmartByteArray& key, SmartByteArray &out)
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

		Status Put(const SmartByteArray& key, const SmartByteArray &value)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Put()");

			HashFile::Record hashRec;
			RET_IFNOT_OK(engine->WriteRecord(DataFile::Record(key, value), hashRec), "BucketManager::Put()");
			
			hashMap[hash] = std::pair<SmartByteArray, HashFile::Record>(key, hashRec);
			RET_BY_SENDER(Status::OK(), "BucketManager::Put()");
		}

		Status Delete(const SmartByteArray& key)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Delete()");

			HashFile::HashMap::iterator it = hashMap.find(hash);
			if (it == hashMap.end())
				RET_BY_SENDER(Status::NotFound("Key doesn't exist"), "BucketManager::Delete()");
			hashMap.erase(it);

			RET_BY_SENDER(Put(key, SmartByteArray::Null()), "BucketManager::Delete()");
		}

		Status Enumerate(const std::function<bool(const SmartByteArray&, const SmartByteArray&)>& func)
		{
			return listPairs([=](const SmartByteArray& Key, const SmartByteArray& Value) -> Status {
				return Status(func(Key, Value));
			});
		}

		Status Compact()
		{
			std::string tmpBucketDir = bucketDir + "__tmp__";

			BucketManager tmpBucket;
			RET_IFNOT_OK(tmpBucket.Open(tmpBucketDir), "BucketManager::Compact()");
			RET_IFNOT_OK(tmpBucket.listPairs([&](const SmartByteArray& Key, const SmartByteArray& Value) -> Status {
				RET_BY_SENDER(tmpBucket.Put(Key, Value), "BucketManager::CompactEnumerator()");
			}), "BucketManager::Compact()");

			RET_IFNOT_OK(StorageEngine::CreateHintFile(tmpBucketDir, hashMap), "BucketManager::Compact()");
			RET_IFNOT_OK(this->Close(), "BucketManager::Compact()");
			RET_IFNOT_OK(tmpBucket.Close(), "BucketManger::Compact()");

			RET_IFNOT_OK(RemoveFile(bucketDir), "BucketManager::Compact()");
			RET_IFNOT_OK(RenameFile(tmpBucketDir, bucketDir), "BucketManager::Compact()");
			RET_IFNOT_OK(this->Open(bucketDir), "BucketManager::Compact()");
		}

	private:
		Status listPairs(const std::function<Status(const SmartByteArray&, const SmartByteArray&)>& func)
		{
			for (auto& item : hashMap)
			{
				SmartByteArray value;
				RET_IFNOT_OK(Get(item.second.first, value), "BucketManager::listPairs()");
				RET_IFNOT_OK(func(item.second.first, value), "BucketManager::listPairs()");
			}
			RET_BY_SENDER(Status::OK(), "BucketManager::listPairs()");
		}

	private:
		std::string bucketDir;
		HashFile::HashMap hashMap;
		std::shared_ptr<StorageEngine> engine;
	};

} // namespace FreshCask
#endif // __CORE_BUCKETMANAGER_HPP__