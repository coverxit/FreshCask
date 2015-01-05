#ifndef __UTIL_LRUCACHE_HPP__
#define __UTIL_LRUCACHE_HPP__

#include <unordered_map>
#include <vector>

#include <Util/LockGuard.hpp>

#include <Algorithm/MurmurHash3.hpp>

namespace FreshCask
{
	class LRUCache
	{
	private:
		struct Node {
			SmartByteArray key, value;
			Node *prev, *next;
		};

	public:
		LRUCache(size_t Size = DefaultLRUCacheSize)
		{
			entries = new Node[Size];

			for (size_t i = 0; i < Size; i++) // store avaliable nodes
				freeEntires.push_back(entries + i);

			head = new Node; tail = new Node; 
			head->prev = NULL; head->next = tail;
			tail->prev = head; tail->next = NULL;
		}

		~LRUCache()
		{
			delete head;
			delete tail;
			delete[] entries;
		}

		Status Put(const SmartByteArray& key, const SmartByteArray& value)
		{
			HashType hash;
			RET_IFNOT_OK(HashFunction(key, hash), "LRUCache::Put()");

			LockGuard lock(syncMutex);

			Node *node = hashMap[hash];
			if (node) // node exists 
			{
				Deatch(node);
				node->value = value;
				Attach(node);
			}
			else
			{
				if (freeEntires.empty()) // no avaliable nodes
				{
					node = tail->prev;
					Deatch(node);
					hashMap.erase(hash);
				}
				else
				{
					node = freeEntires.back();
					freeEntires.pop_back();
				}
				node->key = key;
				node->value = value;
				hashMap[hash] = node;
				Attach(node);
			}

			RET_BY_SENDER(Status::OK(), "LRUCache::Put()");
		}

		Status Get(const SmartByteArray& key, SmartByteArray& out)
		{
			HashType hash;
			RET_IFNOT_OK(HashFunction(key, hash), "LRUCache::Gut()");

			LockGuard lock(syncMutex);

			Node *node = hashMap[hash];
			if (node)
			{
				Deatch(node);
				Attach(node);
				out = node->value;

				RET_BY_SENDER(Status::OK(), "LRUCache::Get()");
			}
			else
				RET_BY_SENDER(Status::NotFound("Key doesn't exist"), "LRUCache::Get()");
		}

		Status Delete(const SmartByteArray& key)
		{
			HashType hash;
			RET_IFNOT_OK(HashFunction(key, hash), "LRUCache::Delete()");

			LockGuard lock(syncMutex);

			Node *node = hashMap[hash];
			if (node)
			{
				Deatch(node);
				hashMap.erase(hash);
				freeEntires.push_back(node);

				RET_BY_SENDER(Status::OK(), "LRUCache::Delete()");
			}
			else
				RET_BY_SENDER(Status::NotFound("Key doesn't exist"), "LRUCache::Delete()");
		}

	private:
		void Deatch(Node *node)
		{
			node->prev->next = node->next;
			node->next->prev = node->prev;
		}

		void Attach(Node *node)
		{
			node->next = head->next;
			node->prev = head;
			head->next = node;
			node->next->prev = node;
		}

	private:
		Mutex syncMutex;

		Node *entries, *head, *tail;
		std::vector<Node*> freeEntires;
		std::unordered_map<HashType, Node*> hashMap;
	};
} // namespace FreshCask

#endif // __UTIL_LRUCACHE_HPP__