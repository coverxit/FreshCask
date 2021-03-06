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
			typedef std::vector<std::string>												ParamArray;
			typedef std::vector<std::string>												VerbArray;
			typedef std::function<void(ParamArray)>											BindStatement;
			typedef std::pair<bool, std::string>											RetType;

		private:
			typedef std::function<RetType(const VerbArray&, BindStatement, ParamArray*)>	TokenParser;
			typedef std::map<std::string, std::pair<TokenParser, bool>>						TokenParserMap;
			typedef std::function<RetType(const VerbArray&, ParamArray*)>					TokenStatement;
			typedef std::map<std::string, TokenStatement>									TokenStatementMap;

		public:
			Parser() 
			{
#define MAKE_TOKEN_PARSER(name) std::bind(&Parser::name, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
				tokenParsers = TokenParserMap
				{
					// token				parser										bind-able
					{ "list",				{ MAKE_TOKEN_PARSER(ListParser),			false	} },
					{ "list bucket",		{ MAKE_TOKEN_PARSER(ListBucketParser),		true	} },
					{ "select",				{ MAKE_TOKEN_PARSER(SelectParser),			false	} },
					{ "select bucket",		{ MAKE_TOKEN_PARSER(SelectBucketParser),	true	} },
					{ "create",				{ MAKE_TOKEN_PARSER(CreateParser),			false	} },
					{ "create bucket",		{ MAKE_TOKEN_PARSER(CreateBucketParser),	true	} },
					{ "remove",				{ MAKE_TOKEN_PARSER(RemoveParser),			false	} },
					{ "remove bucket",		{ MAKE_TOKEN_PARSER(RemoveBucketParser),	true	} },
					{ "get",				{ MAKE_TOKEN_PARSER(GetParser),				true	} },
					{ "put",				{ MAKE_TOKEN_PARSER(PutParser),				true	} },
					{ "delete",				{ MAKE_TOKEN_PARSER(DeleteParser),			true	} },
					{ "enumerate",			{ MAKE_TOKEN_PARSER(EnumerateParser),		true	} },
					{ "compact",			{ MAKE_TOKEN_PARSER(CompactParser),			true	} },
					{ "proc",				{ MAKE_TOKEN_PARSER(ProcParser),			false	} },
					{ "proc begin",			{ MAKE_TOKEN_PARSER(ProcBeginParser),		true	} },
					{ "proc end",			{ MAKE_TOKEN_PARSER(ProcEndParser),			true	} },
				};
#undef MAKE_TOKEN_PARSER

#ifndef _M_CEE // fuck C++/CLI!!!
#define MAKE_TOKEN_STATEMENT(name) std::bind(&Parser::name, this, std::placeholders::_1, nullptr, std::placeholders::_2)
#else
#define MAKE_TOKEN_STATEMENT(name) std::bind(&Parser::name, this, std::placeholders::_1, __nullptr, std::placeholders::_2)
#endif
				tokenStatements = TokenStatementMap
				{
					{ "list",				MAKE_TOKEN_STATEMENT(ListParser)		  },
					{ "list bucket",		MAKE_TOKEN_STATEMENT(ListBucketParser)    },
					{ "select",				MAKE_TOKEN_STATEMENT(SelectParser)		  },
					{ "select bucket",		MAKE_TOKEN_STATEMENT(SelectBucketParser)  },
					{ "create",				MAKE_TOKEN_STATEMENT(CreateParser)		  },
					{ "create bucket",		MAKE_TOKEN_STATEMENT(CreateBucketParser)  },
					{ "remove",				MAKE_TOKEN_STATEMENT(RemoveParser)		  },
					{ "remove bucket",		MAKE_TOKEN_STATEMENT(RemoveBucketParser)  },
					{ "get",				MAKE_TOKEN_STATEMENT(GetParser)			  },
					{ "put",				MAKE_TOKEN_STATEMENT(PutParser)			  },
					{ "delete",				MAKE_TOKEN_STATEMENT(DeleteParser)		  },
					{ "enumerate",			MAKE_TOKEN_STATEMENT(EnumerateParser)	  },
					{ "compact",			MAKE_TOKEN_STATEMENT(CompactParser)		  },
					{ "proc",				MAKE_TOKEN_STATEMENT(ProcParser)		  },
					{ "proc begin",			MAKE_TOKEN_STATEMENT(ProcBeginParser)	  },
					{ "proc end",			MAKE_TOKEN_STATEMENT(ProcEndParser)		  },
				};
#undef MAKE_TOKEN_STATEMENT
			}
			~Parser() {}

			RetType Parse(const std::string& q, ParamArray* out = nullptr)
			{
				if (q.empty()) return Fail("Empty statement");

				std::vector<std::string> verbs = split(q);
				auto its = tokenStatements.find(toLower(verbs[0]));
				if (its == tokenStatements.end())
					return Fail(std::string("Undeclared identifier: `") + verbs[0] + '`');
			
				return its->second(VerbArray(verbs.begin() + 1, verbs.end()), out);
			}

			RetType Bind(const std::string& token, BindStatement statement)
			{
				auto it = tokenParsers.find(token);
				if (it == tokenParsers.end())
					return Fail("Token doesn't exist.");

				if (!it->second.second) // not bind-able
					return Fail("Token cannot be binded.");

				tokenStatements[token] = std::bind(it->second.first, std::placeholders::_1, statement, std::placeholders::_2);
				return OK();
			}

			static bool IsOK(RetType& ret) { return ret.first; }
			static std::string ToString(RetType& ret) { return ret.second; }

		private:
			RetType ListParser(const VerbArray& q, BindStatement, ParamArray* out)
			{
				if (q.empty())
					return Fail("Expected `bucket` after `list`");

				if (toLower(q[0]) != "bucket")
					return Fail(std::string("Expected `bucket` rather than `") + q[0] + "` after `list`");

				return tokenStatements["list bucket"](VerbArray(q.begin() + 1, q.end()), out);
			}

			RetType ListBucketParser(const VerbArray& q, BindStatement s, ParamArray* out)
			{
				if (!q.empty()) // other chars after databse
					return Fail(std::string("Unexpected verb `") + q[0] + "` after `bucket`");

				if (s != nullptr) s(ParamArray());
				if (out != nullptr) *out = ParamArray();
				return OK("list bucket");
			}

			RetType SelectParser(const VerbArray& q, BindStatement, ParamArray* out)
			{
				if (q.empty())
					return Fail("Expected `bucket` after `select`");

				if (toLower(q[0]) != "bucket")
					return Fail(std::string("Expected `bucket` rather than `") + q[0] + "` after `select`");

				return tokenStatements["select bucket"](VerbArray(q.begin() + 1, q.end()), out);
			}

			RetType SelectBucketParser(const VerbArray& q, BindStatement s, ParamArray* out)
			{
				if (q.empty()) // we need bucket name
					return Fail("Expected `<bucket name>` after `bucket`");

				if (q.size() > 1) // other chars after <bucket name>
					return Fail(std::string("Unexpected verb `") + q[1] + "` after `" + q[0] + "`");

				static std::vector<char> invalidChar = {
					0x5C, 0x2F, 0x3A, 0x2A, 0x3F, 0x22, 0x3C, 0x3E, 0x7C
				};

				for (auto& c : invalidChar)
					if (q[0].find(c) != std::string::npos) // invalid char
						return Fail(std::string("Invalid character: `") + c + "` in `" + q[0] + "`");

				if (s != nullptr) s(q);
				if (out != nullptr) *out = q;
				return OK("select bucket");
			}

			RetType CreateParser(const VerbArray& q, BindStatement, ParamArray* out)
			{
				if (q.empty())
					return Fail("Expected `bucket` after `create`");

				if (toLower(q[0]) != "bucket")
					return Fail(std::string("Expected `bucket` rather than `") + q[0] + "` after `create`");

				return tokenStatements["create bucket"](VerbArray(q.begin() + 1, q.end()), out);
			}

			RetType CreateBucketParser(const VerbArray& q, BindStatement s, ParamArray* out)
			{
				if (q.empty()) // we need bucket name
					return Fail("Expected `<bucket name>` after `bucket`");

				if (q.size() > 1) // other chars after <bucket name>
					return Fail(std::string("Unexpected verb `") + q[1] + "` after `" + q[0] + "`");

				static std::vector<char> invalidChar = {
					0x5C, 0x2F, 0x3A, 0x2A, 0x3F, 0x22, 0x3C, 0x3E, 0x7C
				};

				for (auto& c : invalidChar)
					if (q[0].find(c) != std::string::npos) // invalid char
						return Fail(std::string("Invalid character: `") + c + "` in `" + q[0] + "`");

				if (s != nullptr) s(q);
				if (out != nullptr) *out = q;
				return OK("create bucket");
			}

			RetType RemoveParser(const VerbArray& q, BindStatement, ParamArray* out)
			{
				if (q.empty())
					return Fail("Expected `bucket` after `remove`");

				if (toLower(q[0]) != "bucket")
					return Fail(std::string("Expected `bucket` rather than `") + q[0] + "` after `remove`");

				return tokenStatements["remove bucket"](VerbArray(q.begin() + 1, q.end()), out);
			}

			RetType RemoveBucketParser(const VerbArray& q, BindStatement s, ParamArray* out)
			{
				if (q.empty()) // we need bucket name
					return Fail("Expected `<bucket name>` after `bucket`");

				if (q.size() > 1) // other chars after <bucket name>
					return Fail(std::string("Unexpected verb `") + q[1] + "` after `" + q[0] + "`");

				static std::vector<char> invalidChar = {
					0x5C, 0x2F, 0x3A, 0x2A, 0x3F, 0x22, 0x3C, 0x3E, 0x7C
				};

				for (auto& c : invalidChar)
					if (q[0].find(c) != std::string::npos) // invalid char
						return Fail(std::string("Invalid character: `") + c + "` in `" + q[0] + "`");

				if (s != nullptr) s(q);
				if (out != nullptr) *out = q;
				return OK("remove bucket");
			}

			RetType GetParser(const VerbArray& q, BindStatement s, ParamArray* out)
			{
				if (q.empty())
					return Fail("Expected `<key>` after `get`");

				if (q.size() > 1) // other chars after <key>
					return Fail(std::string("Unexpected verb `") + q[1] + "` after `" + q[0] + "`");

				std::string key;
				RetType ret = dealQuotation(q[0], "Expected NOT NULL `<key>` after `get`", key);
				if (!IsOK(ret)) return ret;

				ParamArray param;
				param.push_back(key);
				
				if (s != nullptr) s(param);
				if (out != nullptr) *out = param;
				return OK("get");
			}

			RetType PutParser(const VerbArray& q, BindStatement s, ParamArray* out)
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

				ParamArray param;
				param.push_back(key);
				param.push_back(value);

				if (s != nullptr) s(param);
				if (out != nullptr) *out = param;
				return OK("put");
			}

			RetType DeleteParser(const VerbArray& q, BindStatement s, ParamArray* out)
			{
				if (q.empty())
					return Fail("Expected `<key>` after `delete`");

				if (q.size() > 1) // other chars after <key>
					return Fail(std::string("Unexpected verb `") + q[1] + "` after `" + q[0] + "`");

				std::string key;
				RetType ret = dealQuotation(q[0], "Expected NOT NULL `<key>` after `get`", key);
				if (!IsOK(ret)) return ret;

				ParamArray param;
				param.push_back(key);
				
				if (s != nullptr) s(param);
				if (out != nullptr) *out = param;
				return OK("delete");
			}

			RetType EnumerateParser(const VerbArray& q, BindStatement s, ParamArray* out)
			{
				if (!q.empty()) // other chars after enumerate
					return Fail(std::string("Unexpected verb `") + q[0] + "` after `enumerate`");

				if (s != nullptr) s(ParamArray());
				if (out != nullptr) *out = ParamArray();
				return OK("enumerate");
			}

			RetType CompactParser(const VerbArray& q, BindStatement s, ParamArray* out)
			{
				if (!q.empty()) // other chars after compact
					return Fail(std::string("Unexpected verb `") + q[0] + "` after `compact`");

				if (s != nullptr) s(ParamArray());
				if (out != nullptr) *out = ParamArray();
				return OK("compact");
			}

			RetType ProcParser(const VerbArray& q, BindStatement, ParamArray* out)
			{
				if (q.empty())
					return Fail("Expected `begin` or `end` after `proc`");

				if (toLower(q[0]) != "begin" && toLower(q[0]) != "end")
					return Fail(std::string("Expected `begin` or `end` rather than `") + q[0] + "` after `proc`");

				if (toLower(q[0]) == "begin")
					return tokenStatements["proc begin"](VerbArray(q.begin() + 1, q.end()), out);
				else // end
					return tokenStatements["proc end"](VerbArray(q.begin() + 1, q.end()), out);
			}

			RetType ProcBeginParser(const VerbArray& q, BindStatement s, ParamArray* out)
			{
				if (!q.empty()) // other chars after `begin`
					return Fail(std::string("Unexpected verb `") + q[0] + "` after `begin`");

				if (s != nullptr) s(ParamArray());
				if (out != nullptr) *out = ParamArray();
				return OK("proc begin");
			}

			RetType ProcEndParser(const VerbArray& q, BindStatement s, ParamArray* out)
			{
				if (!q.empty()) // other chars after `end`
					return Fail(std::string("Unexpected verb `") + q[0] + "` after `end`");
				
				if (s != nullptr) s(ParamArray());
				if (out != nullptr) *out = ParamArray();
				return OK("proc end");
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