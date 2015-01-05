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
}

void FQLTest()
{
	FreshCask::FQL::Parser parser;
	parser.Bind("list database", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "list database :";
	});
	parser.Bind("select database", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "select database " << param[0] << " :";
	});
	parser.Bind("create database", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "create database " << param[0] << " :";
	});
	parser.Bind("remove database", [](FreshCask::FQL::Parser::ParamArray param){
		std::cout << "remove database " << param[0] << " :";
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
		std::cout << FreshCask::FQL::Parser::ToString(parser.Parse(str)) << std::endl;
	};

	testParse("no"); testParse("list"); testParse("list no"); testParse("list database"); testParse("list database more");
	testParse("select"); testParse("select no"); testParse("select database"); testParse("select database test_db");
	testParse("select database invalid?name"); testParse("select database invalid/name"); testParse("select database invalid\"name");
	testParse("select database test_db more"); testParse("select database invalid?name more"); testParse("select database invalid/name more");
	testParse("create"); testParse("create no"); testParse("create database"); testParse("create database test_db");
	testParse("create database invalid?name"); testParse("create database invalid/name"); testParse("create database invalid\"name");
	testParse("create database test_db more"); testParse("create database invalid?name more"); testParse("create database invalid/name more");
	testParse("remove"); testParse("remove no"); testParse("remove database"); testParse("remove database test_db");
	testParse("remove database invalid?name"); testParse("remove database invalid/name"); testParse("remove database invalid\"name");
	testParse("remove database test_db more"); testParse("remove database invalid?name more"); testParse("remove database invalid/name more");
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
			doTest( bc.Enumerate([](const FreshCask::SmartByteArray& key, const FreshCask::SmartByteArray& value) -> bool {
				std::cout << "Key: " << key.ToString() << ", Value: " << value.ToString() << std::endl;
				return true;
			}) );
		}
		else if (input == "fqltest" || input == "f")
		{
			FreshCask::FQL::Parser parser;
			FreshCask::FQL::Parser syntaxCheck;

			std::vector<std::string> commandVec;
			bool procBegin = false, nestFlag = false;

			parser.Bind("list database", [](FreshCask::FQL::Parser::ParamArray param){
				std::cout << "list database" << std::endl;
			});
			parser.Bind("select database", [](FreshCask::FQL::Parser::ParamArray param){
				std::cout << "select database " << param[0] << std::endl;
			});
			parser.Bind("create database", [](FreshCask::FQL::Parser::ParamArray param){
				std::cout << "create database " << param[0] << std::endl;
			});
			parser.Bind("remove database", [](FreshCask::FQL::Parser::ParamArray param){
				std::cout << "remove database " << param[0] << std::endl;
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
				doTest(bc.Enumerate([](const FreshCask::SmartByteArray& key, const FreshCask::SmartByteArray& value) -> bool {
					std::cout << "Key: " << key.ToString() << ", Value: " << value.ToString() << std::endl;
					return true;
				}));
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
						std::cout << FreshCask::FQL::Parser::ToString(parser.Parse(q)) << std::endl;
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
						std::cout << FreshCask::FQL::Parser::ToString(parser.Parse(q)) << std::endl;
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