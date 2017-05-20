#include "infra/lock/Lock.h"

using namespace infra;

Lock::Lock(std::mutex &mutex)
:_mutex(mutex)
{
	_mutex.lock();
}

Lock::~Lock()
{
	_mutex.unlock();
}

template<typename T>
AtomicLock<T>::AtomicLock(T &counter)
: _counter(counter)
{
	while(!__sync_lock_test_and_set(&_counter, 1));
}

template<typename T>
AtomicLock<T>::~AtomicLock()
{
	assert(_counter == 1);
	__sync_lock_release(&_counter);
}
