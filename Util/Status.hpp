#ifndef __UTIL_STATUS_HPP__
#define __UTIL_STATUS_HPP__

#include <sstream>
#include <vector>
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
		Status(int code) : code(code) {}
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

		Status PushSender(const std::string& sender) { traceback.push_back(sender); return *this; }

		bool IsOK() const { return code == cOK; }
		bool IsNotFound() const { return code == cNotFound; }
		bool IsInvaildArgument() const { return code == cInvalidArgument; }
		bool IsIOError() const { return code == cIOError; }
		bool IsNotSupported() const { return code == cNotSupported; }

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

			default:
				result << "Unkown code (" << code << "): ";
				break;
			}

			result << message1;
			if (message2.length() > 0) result << " - " << message2 << std::endl;

			if (traceback.size() > 0)
			{
				result << "Traceback:" << std::endl;
				for (std::string& sender : traceback)
					result << "- " << sender << std::endl;
			}

			return result.str();
		}

	private:
		int code;
		std::string message1, message2;
		std::vector<std::string> traceback;

		enum Code
		{
			cOK = 0,
			cNotFound = 1,
			cInvalidArgument = 2,
			cIOError = 3,
			cNotSupported = 4
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

#define RET_IFNOT_OK(exec, sender)	{					\
			Status s = exec;							\
			if (!s.IsOK()) return s.PushSender(sender);	\
		}

#define RET_BY_SENDER(exec, sender) return exec.PushSender(sender);

#endif // __UTIL_STATUS_HPP__