#ifndef __UTIL_STATUS_HPP__
#define __UTIL_STATUS_HPP__

#include <sstream>
#include <vector>
#include <tuple>
#include <system_error>

#ifdef WIN32
#include <Windows.h>
#else
#endif

namespace FreshCask
{
	class Status
	{
	public:
		Status() { code = cOK; }
		explicit Status(int code) : code(code) {}
		explicit Status(bool cond) : code(cUserDefined), message1("The operation was cancelled by user.") {}
		Status(int code, std::string message1, std::string message2)
			: code(code), message1(message1), message2(message2) {}

		static Status OK() { return Status(); }
		static Status NotFound(const std::string& message1, const std::string& message2 = "")
		{
			return Status(cNotFound, message1, message2);
		}
		static Status InvalidArgument(const std::string& message1, const std::string& message2 = "")
		{
			return Status(cInvalidArgument, message1, message2);
		}
		static Status IOError(const std::string& message1, const std::string& message2 = "")
		{
			return Status(cIOError, message1, message2);
		}
		static Status NotSupported(const std::string& message1, const std::string& message2 = "")
		{
			return Status(cNotSupported, message1, message2);
		}
		static Status NoFreeSpace(const std::string& message1, const std::string& message2 = "")
		{
			return Status(cNoFreeSpace, message1, message2);
		}
		static Status Corrupted(const std::string& message1, const std::string& message2 = "")
		{
			return Status(cCorrupted, message1, message2);
		}
		static Status EndOfFile(const std::string& message1, const std::string& message2 = "")
		{
			return Status(cEndOfFile, message1, message2);
		}
		static Status UserDefined(const std::string& message1, const std::string& message2 = "")
		{
			return Status(cUserDefined, message1, message2);
		}

		Status PushSender(const std::string& caller, const char* file, const int line) 
		{ 
			traceback.push_back(make_tuple(caller, file, line));
			return *this;
		}

		bool IsOK() const { return code == cOK; }
		bool IsNotFound() const { return code == cNotFound; }
		bool IsInvaildArgument() const { return code == cInvalidArgument; }
		bool IsIOError() const { return code == cIOError; }
		bool IsNotSupported() const { return code == cNotSupported; }
		bool IsNoFreeSpace() const { return code == cNoFreeSpace; }
		bool IsCorrupted() const { return code == cCorrupted; }
		bool IsEndOfFile() const { return code == cEndOfFile; }
		bool IsUserDefined() const { return code == cUserDefined; }

		std::string ToString()
		{
			std::stringstream result;

			switch (code)
			{
			case cOK:
				return "OK";

			case cNotFound:
				result << "Not Found: ";
				break;

			case cInvalidArgument:
				result << "Invalid Argument: ";
				break;

			case cIOError:
				result << "IO Error: ";
				break;

			case cNotSupported:
				result << "Not Supported: ";
				break;

			case cNoFreeSpace:
				result << "No Free Space: ";
				break;

			case cCorrupted:
				result << "Corrupted: ";
				break;

			case cEndOfFile:
				result << "EOF: ";
				break;

			case cUserDefined:
				result << "User Defined: ";
				break;

			default:
				result << "Unkown code (" << code << "): ";
				break;
			}

			result << message1;
			if (message2.length() > 0) result << " - " << message2;
			result << std::endl;

			if (traceback.size() > 0)
			{
				result << std::endl << "Traceback:" << std::endl;
				for (auto sender : traceback)
				{
					std::string& file = std::get<1>(sender);
					result << "- " << std::get<0>(sender) << " in " << file.substr(file.find_last_of('\\') + 1);
					result << " at line " << std::get<2>(sender) << std::endl;
				}
			}

			return result.str();
		}

	private:
		int code;
		std::string message1, message2;
		std::vector<std::tuple<std::string, std::string, int>> traceback;

		enum Code
		{
			cOK = 0,
			cNotFound = 1,
			cInvalidArgument = 2,
			cIOError = 3,
			cNotSupported = 4,
			cNoFreeSpace = 5,
			cCorrupted = 6,
			cEndOfFile = 7,
			cUserDefined = 8,
		};
	};

	class ErrnoTranslator
	{
	public:
		ErrnoTranslator(int code) 
		{
#ifdef WIN32
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);
#else
			ec.assign(code, std::generic_category()); 
#endif
		}

		~ErrnoTranslator()
		{
#ifdef WIN32
			LocalFree(lpMsgBuf);
#endif
		}

		operator std::string() 
		{ 
#ifdef WIN32
			return std::string((LPSTR)lpMsgBuf);
#else
			return ec.message(); 
#endif
		}

	private:
#ifdef WIN32
		LPVOID lpMsgBuf;
#else
		std::error_condition ec;
#endif
	};

} // namespace FreshCask

#define RET_BY_SENDER(exec, sender) return exec.PushSender(sender, __FILE__, __LINE__)

#define RET_IFNOT_OK(exec, sender)	{					\
			Status s = exec;							\
			if (!s.IsOK()) RET_BY_SENDER(s, sender);	\
		}

#endif // __UTIL_STATUS_HPP__