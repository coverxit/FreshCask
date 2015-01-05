#ifndef __UTIL_SMARTBYTEARRAY_HPP__
#define __UTIL_SMARTBYTEARRAY_HPP__

#include <memory>

namespace FreshCask {

	class SmartByteArray
	{
	public:
		SmartByteArray() : data(nullptr), size(0) {}
		SmartByteArray(const uint32_t& size) : data(new Byte[size]), size(size) {}
		SmartByteArray(const std::string& str) : data(new Byte[str.length()]), size(str.length())
		{
			memcpy(Data(), &str[0], str.length());
		}
		SmartByteArray(const char* str) { new (this) SmartByteArray(std::string(str)); }
		SmartByteArray(const BytePtr ptr, const uint32_t& size) : data(ptr, senderAllocDeleter()), size(size) {}

		std::string ToString() const
		{
			if (Data() != nullptr)
				return std::string(reinterpret_cast<char*>(Data()), Size());
			else
				return std::string();
		}

		BytePtr Data() const { return data.get(); }
		uint32_t Size() const { return size; }

		bool IsNull() { return size == 0 || data == nullptr; }
		static SmartByteArray Null() { return SmartByteArray();  }

		friend bool operator<(const SmartByteArray& lhs, const SmartByteArray& rhs)
		{
			if (lhs.Size() < rhs.Size()) return true;
			else if (lhs.Size() > rhs.Size()) return false;
			else return memcmp(lhs.Data(), rhs.Data(), lhs.Size()) < 0;
		}

	private:
		struct senderAllocDeleter { // tricky, avoid being deleted by std::shared_ptr
			void operator()(BytePtr) { /* do nothing */ }
		};

	private:
		std::shared_ptr<Byte> data;
		uint32_t size;
	};

}	// namespace FreshCask

#endif // __UTIL_SMARTBYTEARRAY_HPP__