#include <iostream>
#include <chrono>
#include <random>

#include <Include/FreshCask.h>

int main()
{
	FreshCask::BucketManager bc;
	FreshCask::Status s = bc.Open("D:\\BucketTest");

	if (!s.IsOK()) std::cout << s.ToString() << std::endl;

	//FreshCask::SmartByteArray Key(new FreshCask::Byte[sizeof(uint32_t)], sizeof(uint32_t));
	FreshCask::SmartByteArray Value(std::string("this is a test string for test purpose."));

	std::chrono::high_resolution_clock::time_point start, end;
	uint64_t duration;

	start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 10000; i++)
	{
		std::stringstream stream;
		stream << i;

		auto Key = FreshCask::SmartByteArray(stream.str());
		bc.Put(Key, Value);
	}
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "10,000 Times: Put 1KB data done in " << duration << " ms" << std::endl;

	//std::mt19937 gen(FreshCask::HashFile::HashSeed);
	start = std::chrono::high_resolution_clock::now();
	std::cout << bc.Enumerate([](const FreshCask::SmartByteArray& key, const FreshCask::SmartByteArray& value) -> bool {
		//std::cout << "Key: " << key.ToString()  << ", Value = " << value.ToString() << std::endl;
		return true;
	}).ToString() << std::endl;
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "Enumerate 10,000 pairs in " << duration << " ms" << std::endl;

	bc.Close();

	std::cin.sync(); std::cin.get();
	return 0;
}