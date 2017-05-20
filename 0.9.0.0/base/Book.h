#ifndef __BOOK_H__
#define __BOOK_H__

#include <limits>
#include <iostream>
#include <map>
#include "pool/object_pool.hpp"

#include "base/Base.h"
#include "base/BaseCommon.h"

namespace base
{
	// http://www.josuttis.com/cppcode/myalloc.hpp
	template <class T>
	class BookAlloc 
	{
		public:
			// type definitions
			boost::object_pool<T> _pool;
			typedef T        value_type;
			typedef T*       pointer;
			typedef const T* const_pointer;
			typedef T&       reference;
			typedef const T& const_reference;
			typedef std::size_t    size_type;
			typedef std::ptrdiff_t difference_type;

			// rebind allocator to type U
			template <class U>
				struct rebind {
					typedef BookAlloc<U> other;
				};

			// return address of values
			pointer address (reference value) const {
				return &value;
			}
			const_pointer address (const_reference value) const {
				return &value;
			}

			/* constructors and destructor
			 * - nothing to do because the allocator has no state
			 */
			BookAlloc() throw()
				: _pool(256)
			{
				_pool.set_next_size(512);
			}

			BookAlloc(const BookAlloc&) throw() {
			}

			template <class U>
				BookAlloc (const BookAlloc<U>&) throw() {
				}
			~BookAlloc() throw() {
			}

			// return maximum number of elements that can be allocated
			size_type max_size () const throw() {
				return std::numeric_limits<std::size_t>::max() / sizeof(T);
			}

			// allocate but don't initialize num elements of type T
			pointer allocate (size_type num, const void* = 0) {
				// print message and allocate memory with global new
				//std::cerr << "allocate " << num << " element(s)"
				//	<< " of size " << sizeof(T) << std::endl;
				pointer ret = (pointer)(::operator new(num*sizeof(T)));
				//std::cerr << " allocated at: " << (void*)ret << std::endl;
				//return ret;
				ASSERT(num == 1, "num : " << num);
				return _pool.malloc();
			}

			// initialize elements of allocated storage p with value value
			void construct (pointer p, const T& value) {
				// initialize memory with placement new
				new((void*)p)T(value);
			}

			// destroy elements of initialized storage p
			void destroy (pointer p) {
				// destroy objects by calling their destructor
				p->~T();
			}

			// deallocate storage p of deleted elements
			void deallocate (pointer p, size_type num) {
				// print message and deallocate memory with global delete
				//std::cerr << "deallocate " << num << " element(s)"
				//	<< " of size " << sizeof(T)
				//		<< " at: " << (void*)p << std::endl;
				//::operator delete((void*)p);
				ASSERT(num == 1, "num : " << num);
				_pool.free(p);
			}
	};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
	bool operator== (const BookAlloc<T1>&,
			const BookAlloc<T2>&) throw() {
		return true;
	}
template <class T1, class T2>
	bool operator!= (const BookAlloc<T1>&,
			const BookAlloc<T2>&) throw() {
		return false;
	}


	typedef std::map<double, PriceLevel *> PriceLevelMap;
	class PriceLevels
	{
		public:
			PriceLevels(Side side);
			uint64_t _quantity;
			PriceLevelMap *_priceLevels;	
	};

	class Book : public Base
	{
		public:
			Book(Subscription *sub);
			~Book();
			void addOrder(Order *order, bool modifyOrder = false);
			void addOrder(Side side, double px, uint64_t size, bool modifyOrder = false);
			void modifyOrder(Order *order);
			void modifyOrder(Side side, double px, double oldPx, uint64_t size, uint64_t oldSize);
			void deleteOrder(Order *order, bool modifyOrder = false);
			void deleteOrder(Side side, double oldPx, uint64_t oldSize, bool modifyOrder = false);

			uint64_t _buyQuantity;
			uint64_t _sellQuantity;

			std::map<double, PriceLevel *, std::greater<double>, BookAlloc<std::pair<const double, PriceLevel *> > > _buys;
			std::map<double, PriceLevel *, std::less<double>, BookAlloc<std::pair<const double, PriceLevel *> > > _sells;
	};
}

#endif //__BOOK_H__
