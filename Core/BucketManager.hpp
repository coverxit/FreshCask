#ifndef __CORE_BUCKETMANAGER_HPP__
#define __CORE_BUCKETMANAGER_HPP__

#include <Util/LRUCache.hpp>

#include <Core/StorageEngine.hpp>

namespace FreshCask
{
	class BucketManager
	{
	public:
		typedef std::function<bool(const SmartByteArray&, const SmartByteArray&)> ExternalEnumeratorType;

	private:
		typedef std::function<Status(const SmartByteArray&, const SmartByteArray&)> InternalEnumeratorType;

	public:
		BucketManager() : engine(nullptr) {}
		~BucketManager() { Close(); }

		//************************************
		// Method:    Open
		// FullName:  FreshCask::BucketManager::Open
		// Access:    public 
		// Returns:   Status
		// Qualifier: Open a bucket
		// Parameter: const std::string & _bucketDir
		//************************************
		Status Open(const std::string &_bucketDir)
		{
			bucketDir = _bucketDir;
			engine = std::shared_ptr<StorageEngine>(new StorageEngine(bucketDir, hashTree));
			RET_BY_SENDER(engine->Open(), "BucketManager::Open()");
		}

		//************************************
		// Method:    IsOpen
		// FullName:  FreshCask::BucketManager::IsOpen
		// Access:    public 
		// Returns:   bool
		// Qualifier: Test if bucket is opened
		//************************************
		bool IsOpen()
		{
			if (engine != nullptr) return true;
			else return false;
		}

		//************************************
		// Method:    Close
		// FullName:  FreshCask::BucketManager::Close
		// Access:    public 
		// Returns:   Status
		// Qualifier: Close bucket
		// Parameter: bool makeHintFile
		//************************************
		Status Close(bool makeHintFile = true) 
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("Bucket not open"), "BucketManager::Open()");
			
			RET_IFNOT_OK(engine->Close(makeHintFile), "BucketManager::Close()");
			
			engine.reset(); hashTree.clear();
			RET_BY_SENDER(Status::OK(), "BucketManager::Close()");
		}

		//************************************
		// Method:    Get
		// FullName:  FreshCask::BucketManager::Get
		// Access:    public 
		// Returns:   Status
		// Qualifier: Get value by a specific key
		// Parameter: const SmartByteArray & key
		// Parameter: SmartByteArray & out
		//************************************
		Status Get(const SmartByteArray& key, SmartByteArray &out)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("Bucket not open"), "BucketManager::Get()");

			//HashFile::HashType hash;
			//RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Get()");

			HashFile::HashTree::iterator it = hashTree.find(key);
			if (it == hashTree.end())
				RET_BY_SENDER(Status::NotFound("Key doesn't exist"), "BucketManager::Get()");

			Status s = cache.Get(key, out);
			if (s.IsNotFound())
			{
				RET_IFNOT_OK(engine->ReadValue(it->second, out), "BucketManager::Get()");
				RET_BY_SENDER(cache.Put(key, out), "BucketManager::Get()");
			}
			else
				RET_BY_SENDER(s, "BucketManager::Get()");
		}

		//************************************
		// Method:    Put
		// FullName:  FreshCask::BucketManager::Put
		// Access:    public 
		// Returns:   Status
		// Qualifier: Put a <key, value> pair into bucket
		// Parameter: const SmartByteArray & key
		// Parameter: const SmartByteArray & value
		//************************************
		Status Put(const SmartByteArray& key, const SmartByteArray &value)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("Bucket not open"), "BucketManager::Put()");

			//HashFile::HashType hash;
			//RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Put()");

			HashFile::Record hashRec;
			RET_IFNOT_OK(engine->WriteRecord(DataFile::Record(key, value), hashRec), "BucketManager::Put()");

			//if (value.Size() > 0) hashTree[hash] = std::pair<SmartByteArray, HashFile::Record>(key, hashRec);
			if (value.Size() > 0) // value.Size() = 0 means delete.
			{
				hashTree[key] = hashRec;
				RET_IFNOT_OK(cache.Put(key, value), "BucketManager::Put()");
			}

			RET_BY_SENDER(Status::OK(), "BucketManager::Put()");
		}

		//************************************
		// Method:    Delete
		// FullName:  FreshCask::BucketManager::Delete
		// Access:    public 
		// Returns:   Status
		// Qualifier: Delete a <key, pair> by key
		// Parameter: const SmartByteArray & key
		//************************************
		Status Delete(const SmartByteArray& key)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("Bucket not open"), "BucketManager::Delete()");

			//HashFile::HashType hash;
			//RET_IFNOT_OK(HashFile::HashFunction(key, hash), "BucketManager::Delete()");

			HashFile::HashTree::iterator it = hashTree.find(key);
			if (it == hashTree.end())
				RET_BY_SENDER(Status::NotFound("Key doesn't exist"), "BucketManager::Delete()");
			hashTree.erase(it);

			RET_IFNOT_OK(cache.Delete(key), "BucketManager::Delete()");
			RET_BY_SENDER(Put(key, SmartByteArray::Null()), "BucketManager::Delete()");
		}

		//************************************
		// Method:    Enumerate
		// FullName:  FreshCask::BucketManager::Enumerate
		// Access:    public 
		// Returns:   Status
		// Qualifier: Enumerate all <key, value> pairs
		// Parameter: const ExternalEnumeratorType & func
		//************************************
		Status Enumerate(const ExternalEnumeratorType& func)
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("Bucket not open"), "BucketManager::Enumerate()");

			RET_BY_SENDER(listPairs([=](const SmartByteArray& key, const SmartByteArray& value) -> Status {
				return Status(func(key, value));
			}), "BucketManager::Enumerate()");
		}

		//************************************
		// Method:    Compact
		// FullName:  FreshCask::BucketManager::Compact
		// Access:    public 
		// Returns:   Status
		// Qualifier: Compact bucket
		//************************************
		Status Compact()
		{
			if (!IsOpen())
				RET_BY_SENDER(Status::IOError("Bucket not open"), "BucketManager::Compact()");

			std::stringstream tmpBucketDir;
			tmpBucketDir << bucketDir << "_tmp_" << GetTimeStamp();

			BucketManager tmpBucket;
			RET_IFNOT_OK(MakeDir(tmpBucketDir.str()), "BucketManager::Compact()");
			RET_IFNOT_OK(tmpBucket.Open(tmpBucketDir.str()), "BucketManager::Compact()");
			RET_IFNOT_OK(this->listPairs([&](const SmartByteArray& key, const SmartByteArray& value) -> Status {
				RET_BY_SENDER(tmpBucket.Put(key, value), "BucketManager::CompactEnumerator()");
			}), "BucketManager::Compact()");

			RET_IFNOT_OK(this->Close(), "BucketManager::Compact()");
			RET_IFNOT_OK(tmpBucket.Close(), "BucketManger::Compact()");

			RET_IFNOT_OK(RemoveDir(bucketDir), "BucketManager::Compact()");
#ifdef WIN32
			::Sleep(1);	// fuck windows, must wait 1 ms.
#endif
			RET_IFNOT_OK(RenameFile(tmpBucketDir.str(), bucketDir), "BucketManager::Compact()");
			RET_BY_SENDER(this->Open(bucketDir), "BucketManager::Compact()");
		}

	private:
		//************************************
		// Method:    listPairs
		// FullName:  FreshCask::BucketManager::listPairs
		// Access:    private 
		// Returns:   Status
		// Qualifier: Internal implement of Enumerate.
		// Parameter: const InternalEnumeratorType & func
		//************************************
		Status listPairs(const InternalEnumeratorType& func)
		{
			for (auto& item : hashTree)
			{
				SmartByteArray value;
				RET_IFNOT_OK(Get(item.first, value), "BucketManager::listPairs()");
				RET_IFNOT_OK(func(item.first, value), "BucketManager::listPairs()");
			}
			RET_BY_SENDER(Status::OK(), "BucketManager::listPairs()");
		}

	private:
		std::string bucketDir;
		HashFile::HashTree hashTree;
		LRUCache cache;
		std::shared_ptr<StorageEngine> engine;
	}; 
} // namespace FreshCask
#endif // __CORE_BUCKETMANAGER_HPP__