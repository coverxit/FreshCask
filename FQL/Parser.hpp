#ifndef __FQL_PARSER_HPP__
#define __FQL_PARSER_HPP__

#include <map>
#include <vector>
#include <tuple>
#include <functional>
#include <string>

#include <FQL/Misc.hpp>

namespace FreshCask
{
	namespace FQL
	{
		class Parser
		{
		public:
			typedef std::vector<std::string>									ParamArray;
			typedef std::vector<std::string>									VerbArray;
			typedef std::function<void(ParamArray)>								BindStatement;
			typedef std::pair<bool, std::string>								RetType;

		private:
			typedef std::function<RetType(const VerbArray&, BindStatement)>		TokenParser;
			typedef std::map<std::string, TokenParser>							TokenParserMap;
			typedef std::function<RetType(const VerbArray&)>					TokenStatement;
			typedef std::map<std::string, TokenStatement>						TokenStatementMap;

		public:
			Parser() 
			{
#define MAKE_TOKEN_PARSER(name) std::bind(&Parser::name, this, std::placeholders::_1, std::placeholders::_2)
				tokenParsers = TokenParserMap
				{
					{ "list",				MAKE_TOKEN_PARSER(ListDatabaseParser)	},
					{ "select",				MAKE_TOKEN_PARSER(SelectDatbaseParser)	},
					{ "create",				MAKE_TOKEN_PARSER(CreateDatbaseParser)	},
					{ "remove",				MAKE_TOKEN_PARSER(RemoveDatbaseParser)	},
					{ "get",				MAKE_TOKEN_PARSER(GetParser)			},
					{ "put",				MAKE_TOKEN_PARSER(PutParser)			},
					{ "delete",				MAKE_TOKEN_PARSER(DeleteParser)			},
					{ "enumerate",			MAKE_TOKEN_PARSER(EnumerateParser)		},
					{ "compact",			MAKE_TOKEN_PARSER(CompactParser)		},
					{ "proc",				MAKE_TOKEN_PARSER(ProcParser)			},
				};
#undef MAKE_TOKEN_PARSER
			}
			~Parser() {}

			RetType Parse(const std::string& q)
			{
				if (q.empty()) return Fail("Empty statement");

				std::vector<std::string> verbs = split(q);
				auto itp = tokenParsers.find(toLower(verbs[0]));
				if (itp == tokenParsers.end())
					return Fail(std::string("Undeclared identifier: `") + verbs[0] + '`');

				auto its = tokenStatements.find(toLower(verbs[0]));
				if (its != tokenStatements.end())
					return its->second(VerbArray(verbs.begin() + 1, verbs.end()));
				else // no bind statement
					return itp->second(VerbArray(verbs.begin() + 1, verbs.end()), nullptr);
			}

			RetType Bind(const std::string& token, BindStatement statement)
			{
				auto it = tokenParsers.find(token);
				if (it == tokenParsers.end())
					return Fail("Token doesn't exist.");

				tokenStatements[token] = std::bind(it->second, std::placeholders::_1, statement);
				return OK();
			}

			static bool IsOK(RetType& ret) { return ret.first; }
			static std::string ToString(RetType& ret) { return ret.second; }

		private:
			RetType ListDatabaseParser(const VerbArray& q, BindStatement s)
			{
				if (q.empty())
					return Fail("Expected `database` after `list`");

				if (toLower(q[0]) != "database")
					return Fail(std::string("Expected `database` rather than `") + q[0] + "` after `list`");

				if (q.size() > 1) // other chars after databse
					return Fail(std::string("Unexpected verb `") + q[1] + "` after `database`");

				if (s == nullptr)
					return OK("No binded statement to parser `list database`.");
				else
					s(ParamArray());
				
				return OK();
			}

			RetType SelectDatbaseParser(const VerbArray& q, BindStatement s)
			{
				if (q.empty())
					return Fail("Expected `database` after `select`");

				if (toLower(q[0]) != "database")
					return Fail(std::string("Expected `database` rather than `") + q[0] + "` after `select`");

				if (q.size() == 1) // we need database name
					return Fail("Expected `<database name>` after `database`");

				if (q.size() > 2) // other chars after <database name>
					return Fail(std::string("Unexpected verb `") + q[2] + "` after `" + q[1] + "`");

				static std::vector<char> invalidChar = {
					0x5C, 0x2F, 0x3A, 0x2A, 0x3F, 0x22, 0x3C, 0x3E, 0x7C
				};

				for (auto& c : invalidChar)
					if (q[1].find(c) != std::string::npos) // invalid char
						return Fail(std::string("Invalid character: `") + c + "` in `" + q[1] + "`");
				
				if (s == nullptr)
					return OK("No binded statement to parser `select database`.");
				else
					s(ParamArray(q.begin() + 1, q.end()));

				return OK();
			}

			RetType CreateDatbaseParser(const VerbArray& q, BindStatement s)
			{
				if (q.empty())
					return Fail("Expected `database` after `create`");

				if (toLower(q[0]) != "database")
					return Fail(std::string("Expected `database` rather than `") + q[0] + "` after `create`");

				if (q.size() == 1) // we need database name
					return Fail("Expected `<database name>` after `database`");

				if (q.size() > 2) // other chars after <database name>
					return Fail(std::string("Unexpected verb `") + q[2] + "` after `" + q[1] + "`");

				static std::vector<char> invalidChar = {
					0x5C, 0x2F, 0x3A, 0x2A, 0x3F, 0x22, 0x3C, 0x3E, 0x7C
				};

				for (auto& c : invalidChar)
					if (q[1].find(c) != std::string::npos) // invalid char
						return Fail(std::string("Invalid character: `") + c + "` in `" + q[1] + "`");

				if (s == nullptr)
					return OK("No binded statement to parser `create database`.");
				else
					s(ParamArray(q.begin() + 1, q.end()));

				return OK();
			}

			RetType RemoveDatbaseParser(const VerbArray& q, BindStatement s)
			{
				if (q.empty())
					return Fail("Expected `database` after `remove`");

				if (toLower(q[0]) != "database")
					return Fail(std::string("Expected `database` rather than `") + q[0] + "` after `remove`");

				if (q.size() == 1) // we need database name
					return Fail("Expected `<database name>` after `database`");

				if (q.size() > 2) // other chars after <database name>
					return Fail(std::string("Unexpected verb `") + q[2] + "` after `" + q[1] + "`");

				static std::vector<char> invalidChar = {
					0x5C, 0x2F, 0x3A, 0x2A, 0x3F, 0x22, 0x3C, 0x3E, 0x7C
				};

				for (auto& c : invalidChar)
					if (q[1].find(c) != std::string::npos) // invalid char
						return Fail(std::string("Invalid character: `") + c + "` in `" + q[1] + "`");

				if (s == nullptr)
					return OK("No binded statement to parser `remove database`.");
				else
					s(ParamArray(q.begin() + 1, q.end()));

				return OK();
			}

			RetType GetParser(const VerbArray& q, BindStatement s)
			{
				if (q.empty())
					return Fail("Expected `<key>` after `get`");

				if (q.size() > 1) // other chars after <key>
					return Fail(std::string("Unexpected verb `") + q[1] + "` after `" + q[0] + "`");

				std::string key;
				RetType ret = dealQuotation(q[0], "Expected NOT NULL `<key>` after `get`", key);
				if (!IsOK(ret)) return ret;

				if (s == nullptr)
					return OK("No binded statement to parser `get`.");
				else
				{
					ParamArray ret;
					ret.push_back(key);
					s(ret);
				}

				return OK();
			}

			RetType PutParser(const VerbArray& q, BindStatement s)
			{
				if (q.empty())
					return Fail("Expected `<key>` after `put`");

				if (q.size() == 1)
					return Fail(std::string("Expected `<value>` after `") + q[0] + "`");

				if (q.size() > 2) // other chars after <value>
					return Fail(std::string("Unexpected verb `") + q[2] + "` after `" + q[1] + "`");

				std::string key, value;
				RetType ret;

				ret = dealQuotation(q[0], "Expected NOT NULL `<key>` after `put`", key);
				if (!IsOK(ret)) return ret;

				ret = dealQuotation(q[1], std::string("Expected NOT NULL `<value>` after `") + q[0] + "`", value);
				if (!IsOK(ret)) return ret;

				if (s == nullptr)
					return OK("No binded statement to parser `put`.");
				else
				{
					ParamArray ret;
					ret.push_back(key);
					ret.push_back(value);
					s(ret);
				}

				return OK();
			}

			RetType DeleteParser(const VerbArray& q, BindStatement s)
			{
				if (q.empty())
					return Fail("Expected `<key>` after `delete`");

				if (q.size() > 1) // other chars after <key>
					return Fail(std::string("Unexpected verb `") + q[1] + "` after `" + q[0] + "`");

				std::string key;
				RetType ret = dealQuotation(q[0], "Expected NOT NULL `<key>` after `get`", key);
				if (!IsOK(ret)) return ret;

				if (s == nullptr)
					return OK("No binded statement to parser `delete`.");
				else
				{
					ParamArray ret;
					ret.push_back(key);
					s(ret);
				}

				return OK();
			}

			RetType EnumerateParser(const VerbArray& q, BindStatement s)
			{
				if (!q.empty()) // other chars after enumerate
					return Fail(std::string("Unexpected verb `") + q[0] + "` after `enumerate`");

				if (s == nullptr)
					return OK("No binded statement to parser `enumerate`.");
				else
					s(ParamArray());

				return OK();
			}

			RetType CompactParser(const VerbArray& q, BindStatement s)
			{
				if (!q.empty()) // other chars after compact
					return Fail(std::string("Unexpected verb `") + q[0] + "` after `compact`");

				if (s == nullptr)
					return OK("No binded statement to parser `compact`.");
				else
					s(ParamArray());

				return OK();
			}

			RetType ProcParser(const VerbArray& q, BindStatement s)
			{
				if (q.empty())
					return Fail("Expected `begin` or `end` after `proc`");

				if (toLower(q[0]) != "begin" && toLower(q[0]) != "end")
					return Fail(std::string("Expected `begin` or `end` rather than `") + q[0] + "` after `proc`");

				if (q.size() > 1) // other chars after `begin` or `end`
					return Fail(std::string("Unexpected verb `") + q[1] + "` after `" + q[0] + "`");

				if (s == nullptr)
					return OK("No binded statement to parser `proc`.");
				else
					s(q);

				return OK();
			}

		private:
			static RetType OK(const std::string& msg = "OK") { return std::make_pair(true, msg); }
			static RetType Fail(const std::string& msg) { return std::make_pair(false, msg); }

		private:
			TokenParserMap tokenParsers;
			TokenStatementMap tokenStatements;
		};
	} // namespace FQL
} // namespace FreshCask

#endif // __FQL_PARSER_HPP__