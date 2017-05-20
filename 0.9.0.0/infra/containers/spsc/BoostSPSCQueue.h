#ifndef __SPSCQUEUE_H__
#define __SPSCQUEUE_H__

#include <memory>
#include <atomic>

#include <stdio.h>

namespace infra 
{

enum BoostElementState 
{
	EMPTY	= 0,
	READ	= 1,
	WRITTEN = 2
};

class BoostSPSCQueueElement
{
	public:
		BoostSPSCQueueElement()
		: _elementState(EMPTY)
		{
		}

		BoostSPSCQueueElement(const BoostSPSCQueueElement &rhs)
		: _elementState(EMPTY)
		{
		}

		std::atomic<uint32_t> _elementState;
		
		void writtenElement()
		{
			_elementState.store(WRITTEN, std::memory_order_release);
		}

		void pushDeleter()
		{
			_elementState.store(WRITTEN, std::memory_order_release);
		}

		void popDeleter()
		{
			_elementState.store(EMPTY, std::memory_order_release);
		}

		~BoostSPSCQueueElement()
		{
		 	_elementState.store(EMPTY, std::memory_order_release);
		}
};

template<typename T, uint64_t Size=2064>
class BoostSPSCQueue 
{
	public:
		BoostSPSCQueue() : head_(0), tail_(0) {}

		bool empty() const
		{
			uint64_t tail = tail_.load(std::memory_order_relaxed);
			if (ring_[tail]._elementState.load(std::memory_order_acquire) != WRITTEN)
				return true;
			else
				return false;
		}

		bool push(const T & value)
		{
			uint64_t head = head_.load(std::memory_order_relaxed);
			
			if (ring_[head]._elementState.load(std::memory_order_acquire) != EMPTY)
				return false;

			ring_[head] = value;
			ring_[head]._elementState.store(WRITTEN, std::memory_order_release);
			head_.store(next(head), std::memory_order_release);

			return true;
		}

		T* push()
		{
			uint64_t head = head_.load(std::memory_order_relaxed);
			
			if (ring_[head]._elementState.load(std::memory_order_acquire) != EMPTY)
				return nullptr;

			head_.store(next(head), std::memory_order_release);
			//@TODO: See  how you could replace lambda with the sophicated function
			//return std::shared_ptr<T>(&ring_[head], std::mem_fun_ref(&T::pushDeleter));
			return &ring_[head];
		}

		std::shared_ptr<T> pushPtr()
		{
			uint64_t head = head_.load(std::memory_order_relaxed);
			
			if (ring_[head]._elementState.load(std::memory_order_acquire) != EMPTY)
				return nullptr;

			head_.store(next(head), std::memory_order_release);
			//@TODO: See  how you could replace lambda with the sophicated function
			//return std::shared_ptr<T>(&ring_[head], std::mem_fun_ref(&T::pushDeleter));
			return std::shared_ptr<T>(&ring_[head], [](T *t){t->_elementState.store(WRITTEN, std::memory_order_release); /*fprintf(stderr, "Wrote something\n");*/});
		}

		T* pushSpin()
		{
			///@TODO: this is code duplication: Remove the code and call pushPtr in while loop
			uint64_t head = head_.load(std::memory_order_relaxed);
			
			while(ring_[head]._elementState.load(std::memory_order_acquire) != EMPTY);

			head_.store(next(head), std::memory_order_release);
			return &ring_[head];
		}

		std::shared_ptr<T> pushPtrSpin()
		{
			///@TODO: this is code duplication: Remove the code and call pushPtr in while loop
			uint64_t head = head_.load(std::memory_order_relaxed);
			
			while(ring_[head]._elementState.load(std::memory_order_acquire) != EMPTY);

			head_.store(next(head), std::memory_order_release);
			return std::shared_ptr<T>(&ring_[head], [](T *t){t->_elementState.store(WRITTEN, std::memory_order_release); /*fprintf(stderr, "Wrote something\n");*/});
		}

		bool pop(T & value)
		{
			uint64_t tail = tail_.load(std::memory_order_relaxed);
			if (ring_[tail]._elementState.load(std::memory_order_acquire) != WRITTEN)
				return false;

			value = ring_[tail];
			
			ring_[tail]._elementState.store(EMPTY, std::memory_order_release);
			tail_.store(next(tail), std::memory_order_release);
			
			return true;
		}

		const T* front() const
		{
			uint64_t tail = tail_.load(std::memory_order_relaxed);
			if (ring_[tail]._elementState.load(std::memory_order_acquire) != WRITTEN)
				return nullptr;

			return &ring_[tail];
		}

		T* pop()
		{
			uint64_t tail = tail_.load(std::memory_order_relaxed);
			if (ring_[tail]._elementState.load(std::memory_order_acquire) != WRITTEN)
				return nullptr;

			tail_.store(next(tail), std::memory_order_release);
			//@TODO: See how you could replace lamda with popDeleter
			//return std::shared_ptr<T>(&ring_[tail]);
			return &ring_[tail];
		}

		std::shared_ptr<T> popPtr()
		{
			uint64_t tail = tail_.load(std::memory_order_relaxed);
			if (ring_[tail]._elementState.load(std::memory_order_acquire) != WRITTEN)
				return nullptr;

			tail_.store(next(tail), std::memory_order_release);
			//@TODO: See how you could replace lamda with popDeleter
			//return std::shared_ptr<T>(&ring_[tail]);
			return std::shared_ptr<T>(&ring_[tail], [](T *t){/*fprintf(stderr, "%ld deleting : %p \n", syscall(SYS_gettid), t);*/t->~T();});
		}
	
		std::shared_ptr<T> popSpin()
		{
			///@TODO: this is code duplication: Remove the code and call popPtr in while loop
			uint64_t tail = tail_.load(std::memory_order_relaxed);

			while(ring_[tail]._elementState.load(std::memory_order_acquire) != WRITTEN);

			tail_.store(next(tail), std::memory_order_release);
			return &ring_[tail];
		}
		
		std::shared_ptr<T> popPtrSpin()
		{
			///@TODO: this is code duplication: Remove the code and call popPtr in while loop
			uint64_t tail = tail_.load(std::memory_order_relaxed);

			while(ring_[tail]._elementState.load(std::memory_order_acquire) != WRITTEN);

			tail_.store(next(tail), std::memory_order_release);
			return std::shared_ptr<T>(&ring_[tail], [](T *t){/*fprintf(stderr, "%ld deleting : %p \n", syscall(SYS_gettid), t);*/t->~T();});
		}

	private:
		uint64_t next(uint64_t current)
		{
			return (current + 1) % Size;
		}

		T ring_[Size];
		std::atomic<uint64_t> head_, tail_;
};

}

#endif //__SPSCQUEUE_H__
