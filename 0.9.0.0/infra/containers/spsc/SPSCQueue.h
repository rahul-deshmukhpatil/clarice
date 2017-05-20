#ifndef __SPSCQUEUE_H__
#define __SPSCQUEUE_H__

#include <memory>
#include <atomic>

namespace infra 
{

	enum ElementState 
	{
		EMPTY	= 0,
		READ	= 1,
		WRITTEN = 2
	};

	class SPSCQueueElement
	{
		public:
			SPSCQueueElement()
			: _elementState(EMPTY)
			{
			}

			ElementState _elementState;
			
			void writtenElement()
			{
				_elementState = WRITTEN;
			}
	};

	template<class T>
	class SPSCQueue
	{
		std::atomic<uint64_t> _front;
		std::atomic<uint64_t> _back;
		std::atomic<uint64_t> _size;
		
		T		 *_element;

		public:
			SPSCQueue(uint64_t size)
			: _front(0)
			, _back(0)
			, _size(size)
			{
				_element = new T[_size];
			}

			std::shared_ptr<T> getReadPtr()
			{
				if(_element[_front]._elementState == WRITTEN)
				{
					_element[_front]._elementState = READ;
					std::shared_ptr<T> result = std::shared_ptr<T>(&_element[_front]);
					_front = (_front + 1 ) % _size;
					return result;
				}
				else
				{
					/// No data available to read
					return nullptr;
				}
			}

			std::shared_ptr<T> getReadPtrBlockable()
			{
				std::shared_ptr<T> readPtr;
				while(! ( readPtr = getReadPtr()));
				return readPtr;
			}
			
			/**
			 *	It is responsibility of thread which calls below function to 
			 *  mark the element as written once its done
			 *
			 *	If data is not written then please make sure that you recycle
			 *	back the writePtr
			 */
			T* getWriterPtr()
			{
				///@TODO : this is asserstion. The last element is not written
				/// and last element is already read and freed so now _front and _back are 
				///	and you are requesting new element to write 
				if((_element[(_back - 1 + _size ) % _size]._elementState != WRITTEN) && (_front != _back))
				{
				}

				if(_element[_back]._elementState == EMPTY)
				{
					T* result = &_element[_back];
					_back = ( _back + 1) % _size;
					return result;
				}
				else
				{
					return nullptr;
				}
			}

			T* getWriterPtrBlockable()
			{
				T* writePtr;
				while(! (writePtr = getReadPtrBlockable()));
				return writePtr;
			}

			void recycleElement(T *element)
			{
				if(element._elementState == WRITTEN)
				{
					///@Raise an exception
					//@TODO: assert this
					return;
				}
				
				if(element._elementState == READ)
				{
					///@Raise an exception
					//@TODO: assert this
					return;
				}

				// element is still empty element._elementState == EMPTY;
				_back = (_back - 1 + _size ) % _size;
			}
	};
}
#endif // __SPSCQUEUE_H__

