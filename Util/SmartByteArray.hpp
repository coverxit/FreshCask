#ifndef __UTIL_SMARTBYTEARRAY_HPP__
#define __UTIL_SMARTBYTEARRAY_HPP__

#include <memory>
namespace FreshCask {

	class SmartByteArray  // TODO: Convert to std::shared_ptr<Byte>
	{
	public:
		SmartByteArray() : data(nullptr), size(0) {}
		SmartByteArray(BytePtr data, uint32_t size) : data(data), size(size), senderAlloc(true) {}
		SmartByteArray(uint32_t size) : data(new Byte[size], selfAllocDeleter()), size(size), senderAlloc(false) {}
		SmartByteArray(std::string str) : data(new Byte[str.length()], selfAllocDeleter()), size(str.length()), senderAlloc(false)
		{
			memcpy(Data(), &str[0], str.length());
		}

		std::string ToString()
		{
			if (Data() != nullptr)
				return std::string(reinterpret_cast<char*>(Data()), Size());
			else
				return std::string();
		}

		BytePtr Data() const { return data.get(); };
		uint32_t Size() const { return size; }

		static SmartByteArray Null() { return SmartByteArray();  }

	private:
		struct selfAllocDeleter
		{
			void operator()(BytePtr ptr) { delete[] ptr; }
		};

	private:
		std::shared_ptr<Byte> data;
		uint32_t size;
		bool senderAlloc;
	};

}	// namespace FreshCask

#endif // __UTIL_SMARTBYTEARRAY_HPP__