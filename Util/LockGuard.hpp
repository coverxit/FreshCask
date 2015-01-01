#ifndef __UTIL_LOCKGUARD_HPP__
#define __UTIL_LOCKGUARD_HPP__

#ifdef WIN32
#include <Windows.h>
#else
#include <mutex>
#endif

namespace FreshCask
{
	class Mutex
	{
	public:
		Mutex() 
		{
#ifdef WIN32
			InitializeCriticalSection(&cs);
#endif
		}
		~Mutex()
		{
#ifdef WIN32
			DeleteCriticalSection(&cs);
#endif
		}

		Mutex(const Mutex&) = delete;
		Mutex& operator=(const Mutex&) = delete;

		void Lock()
		{
#ifdef WIN32
			EnterCriticalSection(&cs);
#else
			mtx.lock();
#endif
		}

		void Unlock()
		{
#ifdef WIN32
			LeaveCriticalSection(&cs);
#else
			mtx.unlock();
#endif
		}

	private:
#ifdef WIN32
		CRITICAL_SECTION cs;
#else
		std::mutex mtx;
#endif
	};

	class LockGuard
	{
	public:
		explicit LockGuard(Mutex& _mtx) : mtx(_mtx) { mtx.Lock(); }
		~LockGuard() { mtx.Unlock(); }

		LockGuard(const LockGuard&) = delete;
		LockGuard& operator=(const LockGuard&) = delete;

	private:
		Mutex& mtx;
	};

} // namespace FreshCask

#endif // __UTIL_LOCKGUARD_HPP__