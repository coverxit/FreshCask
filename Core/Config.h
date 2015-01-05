#ifndef __CORE_CONFIG_H__
#define __CORE_CONFIG_H__

#include <string>

namespace FreshCask 
{
	const uint32_t CurrentMajorVersion = 1;
	const uint32_t CurrentMinorVersion = 0;

	const bool EnableStatusTrackback = true;
	const uint32_t DefaultLRUCacheSize = 3;

	namespace DataFile 
	{
		const uint32_t DefaultMagicNumber = 0x46444346; // FCDF (FreshCask Data File)
		const uint32_t MaxFileSize = (uint32_t)(1024 << 20); // 1 GB

		const std::string FileNameSuffix = ".fcdf";
	} // namespace DataFile

	namespace HashFile
	{
		const uint32_t HashSeed = 0x53484346; // FCHS (FreshCask Hash File)
	} // namespace HashFile

	namespace HintFile
	{
		const uint32_t DefaultMagicNumber = 0x54484346; // FCHT (FreshCask Hint File)
		const std::string FileNameSuffix = ".fcht";
	}

} // namespace FreshCask

#endif // __CORE_CONFIG_H__