#ifndef __LOCK_H
#define __LOCK_H

#include <mutex>

namespace infra
{
	using namespace std;

	/*
	 * Normal lock with the mutex
	 */
	class Lock
	{
		public:
			Lock(std::mutex &mutex);
			~Lock();

		private:
			std::mutex &_mutex;
	};
	
	typedef Lock RLock;
	typedef Lock WLock;

	/*
	 * Atomic lock
	 */
	template<typename T=uint64_t>
	class AtomicLock
	{
		public:
			AtomicLock(T &counter);
			~AtomicLock();

		private:
			T &_counter;
	};
}
#endif
