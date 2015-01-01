#ifndef __FQL_MISC_HPP__
#define __FQL_MISC_HPP__

namespace FreshCask
{
	namespace FQL
	{
		std::string trim(std::string str)
		{
			if (str.length() == 0) return str;

			auto needTrim = [](const char c) -> bool {
				if (c == '\r' || c == '\n' || c == '\t' || c == ' ') return true;
				else return false;
			};

			// Trim left
			while (str.length() > 0)
				if (needTrim(str.front())) str.erase(str.begin());
				else break;

			// Trim right
			while (str.length() > 0)
				if (needTrim(str.back())) str.erase(str.end() - 1);
				else break;

			return str;
		}

		std::string toLower(std::string str)
		{
			for (auto& c : str)
				c = tolower(c);
			return str;
		}

		std::vector<std::string> split(std::string s, char delim = ' ')
		{
			s = trim(s);
			if (s.empty()) return std::vector<std::string>();

			std::vector<std::string> ret;
			size_t pos = 0;
			bool flag;

			while ((pos = s.find(delim)) != std::string::npos)
			{
				flag = true;
				if (s.front() == '"' || s.front() == '\'')
				{
					size_t tmp = 0;
					if ((tmp = s.find(s.front(), 1)) != std::string::npos)
					{
						flag = false;
						ret.push_back(s.substr(0, tmp + 1));
						s.erase(0, tmp + 1); s = trim(s);
					}
				}
				if (flag)
				{
					ret.push_back(trim(s.substr(0, pos)));
					s.erase(0, pos); s = trim(s);
				}
			}

			if (!s.empty()) ret.push_back(s);
			return ret;
		}

		std::pair<bool, std::string> dealQuotation(const std::string& p, const std::string& ep, std::string& out)
		{
			out = p;
			if (p.front() == '"' || p.front() == '\'') // expect an matched " or '
			{
				if (p.front() == '"')
				{
					if (p.back() != '"')
						return std::make_pair(false, (std::string("Missing '\"' after '") + p + "'"));
				}
				else
				{
					if (p.back() != '\'')
						return std::make_pair(false, std::string("Missing \"'\" after '") + p + "'");
				}

				out = p.substr(1, p.length() - 2);
				if (out.empty())
					return std::make_pair(false, ep);
			}
			return std::make_pair(true, "OK");
		}
	} // namespace FQL
} // namespace FreshCask

#endif // __FQL_MISC_HPP__