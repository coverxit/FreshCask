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
	std::cout << "(f)qltest - Test FQL." << std::endl;
	std::cout << "(a)utotests - Automated Tests." << std::endl;
}

void FQLTest()
{
	FreshCask::FQL::Parser parser;
	parser.Bind("list bucket", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "list bucket :";
	});
	parser.Bind("select bucket", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "select bucket " << param[0] << " :";
	});
	parser.Bind("create bucket", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "create bucket " << param[0] << " :";
	});
	parser.Bind("remove bucket", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "remove bucket " << param[0] << " :";
	});
	parser.Bind("get", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "get " << param[0] << " :";
	});
	parser.Bind("put", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "put " << param[0] << " " << param[1] << " :";
	});
	parser.Bind("delete", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "delete " << param[0] << " :";
	});
	parser.Bind("enumerate", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "enumerate :";
	});
	parser.Bind("compact", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "compact :";
	});
	parser.Bind("proc begin", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "proc begin :";
	});
	parser.Bind("proc end", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "proc end :";
	});

	auto testParse = [&](const std::string& str) {
		auto ret = parser.Parse(str);
		std::cout << FreshCask::FQL::Parser::ToString(ret) << std::endl;
	};

	testParse("no"); testParse("list"); testParse("list no"); testParse("list bucket"); testParse("list bucket more");
	testParse("select"); testParse("select no"); testParse("select bucket"); testParse("select bucket test_db");
	testParse("select bucket invalid?name"); testParse("select bucket invalid/name"); testParse("select bucket invalid\"name");
	testParse("select bucket test_db more"); testParse("select bucket invalid?name more"); testParse("select bucket invalid/name more");
	testParse("create"); testParse("create no"); testParse("create bucket"); testParse("create bucket test_db");
	testParse("create bucket invalid?name"); testParse("create bucket invalid/name"); testParse("create bucket invalid\"name");
	testParse("create bucket test_db more"); testParse("create bucket invalid?name more"); testParse("create bucket invalid/name more");
	testParse("remove"); testParse("remove no"); testParse("remove bucket"); testParse("remove bucket test_db");
	testParse("remove bucket invalid?name"); testParse("remove bucket invalid/name"); testParse("remove bucket invalid\"name");
	testParse("remove bucket test_db more"); testParse("remove bucket invalid?name more"); testParse("remove bucket invalid/name more");
	testParse("get"); testParse("get key"); testParse("get \"key"); testParse("get \"key\""); testParse("get \"key\" more");
	testParse("get 'key"); testParse("get 'key'"); testParse("get 'key' more"); testParse("put"); testParse("put key");
	testParse("put \"key"); testParse("put 'key"); testParse("put key value"); testParse("put \"key value"); testParse("put \"key\" value");
	testParse("put 'key value"); testParse("put 'key' value"); testParse("put key \"value"); testParse("put key \"value\""); testParse("put key 'value");
	testParse("put key 'value'"); testParse("put \"key\" \"value"); testParse("put \"key\" \"value\""); testParse("put 'key' 'value"); 
	testParse("put 'key' 'value'"); testParse("put \"key\" 'value"); testParse("put \"key\" 'value'"); testParse("put 'key' \"value");
	testParse("put 'key' \"value\""); testParse("put key value more"); testParse("put \"key value more"); testParse("put \"key\" value more");
	testParse("put 'key value more"); testParse("put 'key' value more"); testParse("put key \"value more"); testParse("put key \"value\" more");
	testParse("put key 'value more"); testParse("put key 'value' more"); testParse("put \"key\" \"value more"); testParse("put \"key\" \"value\" more");
	testParse("put 'key' 'value more"); testParse("put 'key' 'value' more"); testParse("put \"key\" 'value more"); testParse("put \"key\" 'value' more");
	testParse("put 'key' \"value more"); testParse("put 'key' \"value\" more"); testParse("delete"); testParse("delete key"); testParse("delete \"key");
	testParse("delete \"key\""); testParse("delete \"key\" more"); testParse("delete 'key"); testParse("delete 'key'"); testParse("delete 'key' more");
	testParse("enumerate"); testParse("enumerate more"); testParse("compact"); testParse("compact more"); testParse("proc"); testParse("proc no");
	testParse("proc begin"); testParse("proc begin more"); testParse("proc end"); testParse("proc end more");
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
			std::vector<std::string> keys;
			doTest(bc.Enumerate(keys));

			for (auto& key : keys)
			{
				FreshCask::SmartByteArray value;
				doTest(bc.Get(key, value));
				std::cout << "Key: " << key << ", Value: " << value.ToString() << std::endl;
			}
		}
		else if (input == "autotests" || input == "a") 
		{
			FQLTest();
		}
		else if (input == "fqltest" || input == "f")
		{
			FreshCask::FQL::Parser parser;
			FreshCask::FQL::Parser syntaxCheck;

			std::vector<std::string> commandVec;
			bool procBegin = false, nestFlag = false;

			parser.Bind("list bucket", [](FreshCask::FQL::Parser::ParamArray param){
				std::cout << "list bucket" << std::endl;
			});
			parser.Bind("select bucket", [](FreshCask::FQL::Parser::ParamArray param){
				std::cout << "select bucket " << param[0] << std::endl;
			});
			parser.Bind("create bucket", [](FreshCask::FQL::Parser::ParamArray param){
				std::cout << "create bucket " << param[0] << std::endl;
			});
			parser.Bind("remove bucket", [](FreshCask::FQL::Parser::ParamArray param){
				std::cout << "remove bucket " << param[0] << std::endl;
			});
			parser.Bind("get", [&](FreshCask::FQL::Parser::ParamArray param){
				FreshCask::SmartByteArray out;
				FreshCask::Status s = bc.Get(param[0], out);
				doTest(s);

				if (s.IsOK()) std::cout << "Value: " << out.ToString() << std::endl;
			});
			parser.Bind("put", [&](FreshCask::FQL::Parser::ParamArray param){
				doTest(bc.Put(param[0], param[1]));
			});
			parser.Bind("delete", [&](FreshCask::FQL::Parser::ParamArray param){
				doTest(bc.Delete(param[0]));
			});
			parser.Bind("enumerate", [&](FreshCask::FQL::Parser::ParamArray param){
				std::vector<std::string> keys;
				doTest(bc.Enumerate(keys));

				for (auto& key : keys)
				{
					FreshCask::SmartByteArray value;
					doTest(bc.Get(key, value));
					std::cout << "Key: " << key << ", Value: " << value.ToString() << std::endl;
				}
			});
			parser.Bind("compact", [&](FreshCask::FQL::Parser::ParamArray param){
				doTest(bc.Compact());
			});

			auto procBeginStatement = [&](FreshCask::FQL::Parser::ParamArray param){
				if (procBegin) std::cout << "No nest of 'proc begin'" << std::endl, procBegin = false, nestFlag = true;
				else procBegin = true;
			};
			auto procEndStatement = [&](FreshCask::FQL::Parser::ParamArray param){
				if (!procBegin) std::cout << "No 'proc begin' before." << std::endl;
				procBegin = false;
			};

			parser.Bind("proc begin", procBeginStatement);
			parser.Bind("proc end", procEndStatement);

			syntaxCheck.Bind("proc begin", procBeginStatement);
			syntaxCheck.Bind("proc end", procEndStatement);

			std::string q;
			std::cout << "FQL> ";
			do {
				std::cin.sync();
				std::getline(std::cin, q);

				if (q == "quit" || q == "q") break;

				if (procBegin)
				{
					FreshCask::FQL::Parser::RetType ret = syntaxCheck.Parse(q);
					if (!FreshCask::FQL::Parser::IsOK(ret))
					{
						std::cout << FreshCask::FQL::Parser::ToString(ret) << std::endl;
						procBegin = false;
						commandVec.clear();
					}
					else if (nestFlag)
						commandVec.clear(), nestFlag = false;
					else
						commandVec.push_back(q);
				}
				else
				{
					FreshCask::FQL::Parser::RetType ret = parser.Parse(q);

					if (!FreshCask::FQL::Parser::IsOK(ret))
						std::cout << FreshCask::FQL::Parser::ToString(ret) << std::endl;
				}

				if (procBegin) std::cout << "     ..  ";
				else 
				{
					if (!commandVec.empty())
					{
						for (size_t i = 0; i < commandVec.size() - 1; i++)
							parser.Parse(commandVec[i]);
						commandVec.clear();
					}

					std::cout << "FQL> ";
				}
			} while (true);
		}
		else if (input == "compact" || input == "t") doTest( bc.Compact() );
		else std::cout << "[Console] Unknown command." << std::endl;
		std::cout << "> ";
	}
	return 0;
}