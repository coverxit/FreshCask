#include <iostream>
#include <chrono>
#include <random>

#include <FreshCask.h>

void doTest(FreshCask::Status s)
{ 
	if (!s.IsOK()) 
		std::cout << "[Result] " << s.ToString(); 
	/*else 
		std::cout << "[Result] The operation completed successfully." << std::endl; */
}

void printHelp()
{
	std::cout << "FreshCask Test Console" << std::endl;
	std::cout << "(h)elp - Show this help." << std::endl;
	std::cout << "(q)uit - Quit this console." << std::endl;
	std::cout << "(o)pen - Open bucket at D:\\BucketTest." << std::endl;
	std::cout << "(c)lose - Close bucket." << std::endl;
	std::cout << "(g)et <key> - Get a <key, value> pair by key." << std::endl;
	std::cout << "(p)ut <key> <value> - Put a <key, value> pair into bucket." << std::endl;
	std::cout << "(d)elete <key> - Delete a <key, value> pair by key." << std::endl;
	std::cout << "(e)numerate - Enumerate all <key, value> pairs." << std::endl;
	std::cout << "compac(t) - Compact bucket to increase performance." << std::endl;
}

int main()
{
	FreshCask::BucketManager bc;
	std::string input;

	printHelp(); std::cout << "> ";
	while (std::cin >> input)
	{
		if (input == "help" || input == "h") printHelp();
		else if (input == "quit" || input == "q") { if (bc.IsOpen()) doTest( bc.Close() ); break; }
		else if (input == "open" || input == "o") doTest( bc.Open("D:\\BucketTest") );
		else if (input == "close" || input == "c") doTest( bc.Close() );
		else if (input == "get" || input == "g")
		{
			std::string key;
			std::cin >> key;

			FreshCask::SmartByteArray out;
			FreshCask::Status s = bc.Get(key, out);
			doTest( s );

			if (s.IsOK()) std::cout << "Value: " << out.ToString() << std::endl;
		}
		else if (input == "put" || input == "p")
		{
			std::string key, value;
			std::cin >> key >> value;

			doTest( bc.Put(key, value) );
		}
		else if (input == "delete" || input == "d")
		{
			std::string key;
			std::cin >> key;

			doTest( bc.Delete(key) );
		}
		else if (input == "enumerate" || input == "e")
		{
			doTest( bc.Enumerate([](const FreshCask::SmartByteArray& key, const FreshCask::SmartByteArray& value) -> bool {
				std::cout << "Key: " << key.ToString() << ", Value: " << value.ToString() << std::endl;
				return true;
			}) );
		}
		else if (input == "compact" || input == "t") doTest( bc.Compact() );
		else std::cout << "[Console] Unknown command." << std::endl;
		std::cout << "> ";
	}
	return 0;
}