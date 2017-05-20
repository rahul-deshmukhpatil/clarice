#ifndef __PRICELEVELS_H__
#define __PRICELEVELS_H__

#include "base/Base.h"
#include "base/BaseCommon.h"

namespace base 
{
	class PriceLevel// : public Base
	{
		public:
			// Init the price level to this order
			PriceLevel(Order *);
			PriceLevel(double px, uint64_t size);
			void addOrder(Order *order);
			void addOrder(uint64_t size);
			void modifyOrder(Order *order);
			void modifyOrder(uint64_t size, uint64_t oldSize);
			void deleteOrder(Order *order);
			void deleteOrder(uint64_t oldSize);
			double _px;
			uint64_t _size;
			Order *_orders;
			uint64_t _numOrders;
	};
}

#endif //__PRICELEVELS_H__
