#ifndef __CORE_BUCKETMANAGER_HPP__
#define __CORE_BUCKETMANAGER_HPP__

#include <Core/Config.h>

#include <Util/Status.hpp>
#include <Util/SmartByteArray.hpp>
#include <Util/Misc.hpp>

#include <Core/StorageEnginePool.hpp>

namespace FreshCask
{
	class BucketManager
	{
	public:
		BucketManager() {}
		~BucketManager() { Close(); }

		Status Open(std::string filePath)
		{
//			RET_BY_SENDER(storeEngine.Open(filePath, DataFile::ActiveFile), "Bucket::Open()");
			return Status::OK();
		}

		Status Close() 
		{ 
//			RET_BY_SENDER(storeEngine.Close(), "Bucket::Close()"); 
			return Status::OK();
		}

		Status Get(SmartByteArray key, SmartByteArray &out)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "Bucket::Get()");

			HashFile::HashMap::iterator it = hashMap.find(hash);
			if (it == hashMap.end())
				return Status::NotFound("Bucket::Get()", "Key doesn't exist");

			DataFile::Record dataRec;
//			RET_IFNOT_OK(storeEngine.ReadRecord(it->second.OffsetOfValue, dataRec), "Bucket::Get()");
			out = dataRec.Value;
			return Status::OK();
		}

		Status Put(SmartByteArray key, SmartByteArray value)
		{
			HashFile::HashType hash;
			RET_IFNOT_OK(HashFile::HashFunction(key, hash), "Bucket::Put()");

			HashFile::Record hashRec;
//			RET_IFNOT_OK(storeEngine.WriteRecord(DataFile::Record(key, value), hashRec), "Bucket::Put()");
			
			hashMap[hash] = hashRec;
			return Status::OK();
		}

	private:
		HashFile::HashMap hashMap;
		//StorageEngine storeEngine;
	};

} // namespace FreshCask
#endif // __CORE_BUCKETMANAGER_HPP__