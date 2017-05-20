#ifndef __SPSCQUEUE_H__
#define __SPSCQUEUE_H__

#include <memory>
#include <atomic>

#include <stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>

namespace infra 
{

// stacktrace.h (c) 2008, Timo Bingmann from http://idlebox.net/
// published under the WTFPL v2.0


/** Print a demangled stack backtrace of the caller function to FILE* out. */
static inline void print_stacktrace(FILE *out = stderr, unsigned int max_frames =15)
{
    fprintf(out, "stack trace:\n");

    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0) {
	fprintf(out, "  <empty, possibly corrupt>\n");
	return;
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char* funcname = (char*)malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (int i = 1; i < addrlen; i++)
    {
	char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

	// find parentheses and +address offset surrounding the mangled name:
	// ./module(function+0x15c) [0x8048a6d]
	for (char *p = symbollist[i]; *p; ++p)
	{
	    if (*p == '(')
		begin_name = p;
	    else if (*p == '+')
		begin_offset = p;
	    else if (*p == ')' && begin_offset) {
		end_offset = p;
		break;
	    }
	}

	if (begin_name && begin_offset && end_offset
	    && begin_name < begin_offset)
	{
	    *begin_name++ = '\0';
	    *begin_offset++ = '\0';
	    *end_offset = '\0';

	    // mangled name is now in [begin_name, begin_offset) and caller
	    // offset in [begin_offset, end_offset). now apply
	    // __cxa_demangle():

	    int status;
	    char* ret = abi::__cxa_demangle(begin_name,
					    funcname, &funcnamesize, &status);
	    if (status == 0) {
		funcname = ret; // use possibly realloc()-ed string
		fprintf(out, "  %s : %s+%s\n",
			symbollist[i], funcname, begin_offset);
	    }
	    else {
		// demangling failed. Output function name as a C function with
		// no arguments.
		fprintf(out, "  %s : %s()+%s\n",
			symbollist[i], begin_name, begin_offset);
	    }
	}
	else
	{
	    // couldn't parse the line? print the whole line.
	    fprintf(out, "  %s\n", symbollist[i]);
	}
    }

    free(funcname);
    free(symbollist);
}

enum BoostElementState 
{
	EMPTY	= 7,
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

		uint8_t _elementState;
		
		void writtenElement()
		{
			//_elementState.store(WRITTEN, std::memory_order_release);
			_elementState = WRITTEN;
		}

		void pushDeleter()
		{
			//_elementState.store(WRITTEN, std::memory_order_release);
			_elementState = WRITTEN;
		}

		void popDeleter()
		{
			//_elementState.store(EMPTY, std::memory_order_release);
			_elementState = EMPTY;
		}

		~BoostSPSCQueueElement()
		{
		 	//_elementState.store(EMPTY, std::memory_order_release);
			_elementState = EMPTY;
		}
};

template<typename T, uint64_t Size=2064>
class BoostSPSCQueue 
{
	public:
		BoostSPSCQueue() : head_(0), tail_(0) {}


		// Below functions should be only called by writer thread
		std::shared_ptr<T> pushPtr()
		{
			if (ring_[head_]._elementState != EMPTY)
				return nullptr;

			uint64_t prev = head_;
			head_ = ((head_ + 1) % Size);
			//@TODO: See  how you could replace lambda with the sophicated function
			//return std::shared_ptr<T>(&ring_[head], std::mem_fun_ref(&T::pushDeleter));
			//return std::shared_ptr<T>(&ring_[prev], [](T *t){t->_elementState.store(WRITTEN, std::memory_order_release); /*fprintf(stderr, "Wrote something\n");*/});
			return std::shared_ptr<T>(&ring_[prev], [](T *t){t->_elementState = WRITTEN; /*fprintf(stderr, "Wrote something\n");*/});
		}

		std::shared_ptr<T> pushPtrSpin()
		{
			///@TODO: this is code duplication: Remove the code and call pushPtr in while loop
			while(ring_[head_]._elementState != EMPTY);

			uint64_t prev = head_;
			head_ = ((head_ + 1) % Size);
			return std::shared_ptr<T>(&ring_[prev], [](T *t){t->_elementState = WRITTEN; /*fprintf(stderr, "Wrote something\n");*/});
		}

		// Below functions should be only called by reader thread
		std::shared_ptr<T> popPtr()
		{
			if (ring_[tail_]._elementState != WRITTEN)
				return nullptr;

			uint64_t prev = tail_;
			tail_ = ((tail_ + 1) % Size);
			//@TODO: See how you could replace lamda with popDeleter
			//return std::shared_ptr<T>(&ring_[tail]);
			return std::shared_ptr<T>(&ring_[prev], [](T *t){/*fprintf(stderr, "1] %ld deleting : %p \n", syscall(SYS_gettid), t); print_stacktrace();*/ t->~T();});
		}
		
		std::shared_ptr<T> popPtrSpin()
		{
			///@TODO: this is code duplication: Remove the code and call popPtr in while loop
			while(ring_[tail_]._elementState != WRITTEN);

			uint64_t prev = tail_;
			tail_ = ((tail_ + 1) % Size);
			return std::shared_ptr<T>(&ring_[prev], [](T *t){/*fprintf(stderr, "2] %ld deleting : %p \n", syscall(SYS_gettid), t); print_stacktrace();*/ t->~T();});
		}

	private:
		uint64_t next(uint64_t current)
		{
			return (current + 1) % Size;
		}

		T ring_[Size];
		uint64_t head_, tail_;
};


}

#endif //__SPSCQUEUE_H__
