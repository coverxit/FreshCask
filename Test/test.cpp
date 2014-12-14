#include <iostream>
#include <chrono>
#include <random>

#include <Include/FreshCask.h>

int main()
{
	std::cout << sizeof(FreshCask::HashFile::Record) << std::endl;

	FreshCask::BucketManager bc;
	FreshCask::Status s = bc.Open("test.fc");

	if (!s.IsOK()) std::cout << s.ToString() << std::endl;

	FreshCask::SmartByteArray Key(new FreshCask::Byte[sizeof(uint32_t)], sizeof(uint32_t));
	FreshCask::SmartByteArray Value(new FreshCask::Byte[1024], 1024);
	memset(Value.Data(), 0xFF, 1024);

	std::chrono::high_resolution_clock::time_point start, end;
	uint64_t duration;

	start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 100000; i++)
	{
		std::stringstream stream;
		stream << i;
		memcpy(Key.Data(), stream.str().c_str(), sizeof(uint32_t));
		
		bc.Put(Key, Value);
	}
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "100,000 Times: Put 1KB data done in " << duration << " ms" << std::endl;

	std::mt19937 gen(FreshCask::HashFile::HashSeed);
	start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 1000000; i++)
	{
		memcpy(Key.Data(), &i, sizeof(uint32_t));
	//	bc.Get(Key, Value);

		//std::cout << "K: " << std::hex << Key.ToString() << ", V: " << std::hex << Value.ToString().substr(0, 10) << std::endl;
	}
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "1,000,000 Times: Random Get in " << duration << " ms" << std::endl;
}