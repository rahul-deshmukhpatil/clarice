#ifndef __ORDER_H__
#define __ORDER_H__

#include <stdint.h>
#include <string.h>
#include <string>

#include "base/BaseCommon.h"
#include "base/Base.h"

namespace base
{
	enum OrderType
	{
		ORDTYPE_UNKNOWN = 0,
		NORMAL_ORDER = 1,
		MARKET_ORDER = 2
	};

	enum OrderAction
	{
		ADDED = 'A',
		MODIFIED = 'M',
		DELETED = 'D',
		EXECUTED = 'E',
		ORDACT_UNKNOWN = 'U'
	};

    /*
     *  Order Class:
     *  Each order ID is uniq across exchange.
     *  So OrderPool contains uniq orders.
     *  Order Updates for existing order come on same line as of the line of order origin.
     *  So order pools are defined per thread.
     *
     */
    class Order : public Base
    {
        public:
            /*
             * Order Constructor
             * Input:   Price, size , order ID and IDType i.e numeric or alphanumeric
             * Output:  Order is constructed
             */
			 Order();
			static void Add(Line *line, Subscription *sub, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, double px, uint32_t size, Side side, uint64_t orderID);

			void init(Line *, Subscription *, OrderPool *orderPool, uint32_t, uint64_t , uint32_t , uint32_t , double , uint32_t , Side , uint64_t);
			static void ModifyPxSize(Line *, uint32_t, uint64_t , uint32_t , uint32_t , uint64_t, double , uint32_t);
			static void ModifyPx(Line *,  uint32_t, uint64_t , uint32_t , uint32_t , uint64_t, double );
			static void ModifySize(Line *,  uint32_t, uint64_t , uint32_t , uint32_t , uint64_t, uint32_t);
			static void CancelSize(Line *, uint32_t, uint64_t , uint32_t , uint32_t , uint64_t, uint32_t);

			static void Delete(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID);

			void remove(Line *, uint32_t, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds);

			// must be when order modify crosses the apposite side top of book;
			// Both order executed quantity and remaining display quantity;
			// are sent along with trade/execution price ;
			static void Execute(Line *, uint32_t, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID);
			static void ExecuteSize(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, uint64_t execSize, uint64_t remaining);
			static void ExecuteSize(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, uint64_t execSize);
			static void ExecutePxSize(Line *, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, double execPx, uint64_t execSize, uint64_t remaining);
			static void ExecutePxSize(Line *, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, double execPx, uint64_t execSize);
			void _execute(Line *, uint32_t, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, double execPx, uint64_t execSize, uint64_t remaining);
			~Order();

            double _px;
            uint32_t _size;
            double _oldPx;
            uint32_t _oldSize;
			Side _side;
			OrderType _orderType;
			OrderAction	_action;
            ID _orderID;
			OrderPool *_orderPool; // to which order belongs
			Order	*_next;	// Next order on the same level
			PriceLevel *_level; // level to which order belongs

		private:
			void _modify(Line *line, uint32_t, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, double px, uint32_t size);
    };
}

#include "base/MarketDataApplication.h"
#include "base/LineGroup.h"
#include "base/Line.h"
#include "base/Subscription.h"
#include "base/API.h"

using namespace base;

inline Order::Order()
	:Base(nullptr, ORDER_UPDATE)
	, _px(0.0)
	, _size(0)
	, _oldPx(0)
	, _oldSize(0)
	, _side(SIDE_UNKNOWN)
	, _action(OrderAction::ORDACT_UNKNOWN)
	, _orderID()
	, _orderType(ORDTYPE_UNKNOWN)
	, _orderPool(nullptr)
	, _next(nullptr)
	, _level(nullptr)
{
}

/*
 * Order Constructor
 * Input:   Price, size , order ID and IDType i.e numeric or alphanumeric
 * Output:  Order is constructed
 */
inline void Order::Add(Line *line, Subscription *sub, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, double px, uint32_t size, Side side, uint64_t orderID)
{
	OrderPool *orderPool = line->_orderPool;
#ifdef __ORDERPOOL_LATENCY__
	timespec mt = globalClock();
	Order *order = orderPool->malloc();
	getLatency(line->_lineGroup->_op, mt);
#else
	Order *order = orderPool->malloc();
#endif
	
	order->init(line
			, sub
			, orderPool 
			, msgType 
			, seqNo 
			, exSeconds 
			, exUSeconds 
			, px
			, size 
			, side 
			, orderID);
}

inline void Order::init(Line *line, Subscription *sub, OrderPool *orderPool, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, double px, uint32_t size, Side side, uint64_t orderID)
{
	Base::update(line, sub, msgType, seqNo, exSeconds, exUSeconds, ORDER_UPDATE);
	_px = px;
	_size = size;
	_oldPx = 0;
	_oldSize = 0;
	_side = side;
	_action = OrderAction::ADDED;
	_orderID = orderID;
	_orderType = px == 0.0 ? MARKET_ORDER : NORMAL_ORDER;
	_orderPool = orderPool;
	_next = nullptr;
	_level = nullptr;

	line->insert(orderID, this);

	LineAPI *api = line->_lineAPI;
	if(line->hasStrictInterest(BOOK))
	{
		sub->_book.update(line, msgType, seqNo, exSeconds, exUSeconds);
		if(_line->_maintainOrders)
		{
			sub->_book.addOrder(this);
		}
		else
		{
			sub->_book.addOrder(_side, _px, _size);
		}
	
		if(!line->hasStrictInterest(QUOTE))
		{
			(api->*(api->_onBook))(&_sub->_book);
		}
	}
	else
	{
		(api->*(api->_onOrder))(this);
	}
}

inline void Order::ModifyPxSize(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, double px, uint32_t size)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	Order *order = line->find(orderID);
	getLatency(line->_lineGroup->_om, mt);
#else
	Order *order = line->find(orderID);
#endif

	if (order)
	{
		order->_modify(line, msgType, seqNo, exSeconds, exUSeconds, px, size);
	}
}

inline void Order::ModifyPx(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, double px)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	Order *order = line->find(orderID);
	getLatency(line->_lineGroup->_om, mt);
#else
	Order *order = line->find(orderID);
#endif

	if (order)
	{
		order->_modify(line, msgType, seqNo, exSeconds, exUSeconds, px, order->_size);
	}
}

inline void Order::ModifySize(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, uint32_t newSize)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	Order *order = line->find(orderID);
	getLatency(line->_lineGroup->_om, mt);
#else
	Order *order = line->find(orderID);
#endif

	if (order)
	{
		order->_modify(line, msgType, seqNo, exSeconds, exUSeconds, order->_px, newSize);
	}
}

inline void Order::CancelSize(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, uint32_t cancelSize)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	Order *order = line->find(orderID);
	getLatency(line->_lineGroup->_om, mt);
#else
	Order *order = line->find(orderID);
#endif

	if (order)
	{
		order->_modify(line, msgType, seqNo, exSeconds, exUSeconds, order->_px, order->_size - cancelSize);
	}
}

inline void Order::_modify(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, double px, uint32_t size)
{
	Base::update(msgType, seqNo, exSeconds, exUSeconds);
	_oldPx = _px;
	_oldSize = _size;
	_px = px;
	_size = size;
	_orderType = (px == 0.0)? MARKET_ORDER : NORMAL_ORDER;
	_action = OrderAction::MODIFIED;
	
	LineAPI *api = line->_lineAPI;
	if(line->hasStrictInterest(BOOK))
	{
		_sub->_book.update(line, msgType, seqNo, exSeconds, exUSeconds);
		if(_line->_maintainOrders)
		{
			_sub->_book.modifyOrder(this);
		}
		else
		{
			_sub->_book.modifyOrder(_side, _px, _oldPx, _size, _oldSize);
		}
		
		if(!line->hasStrictInterest(QUOTE))
		{
			(api->*(api->_onBook))(&_sub->_book);
		}
	}
	else
	{
		(api->*(api->_onOrder))(this);
	}
}

inline void Order::Delete(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	Order *order = line->find(orderID);
	getLatency(line->_lineGroup->_om, mt);
#else
	Order *order = line->find(orderID);
#endif
	if (order)
	{
		order->remove(line, msgType, seqNo, exSeconds, exUSeconds);
		//delete order;
	}
}

inline void Order::remove(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds)
{
	Base::update(msgType, seqNo, exSeconds, exUSeconds);
	_oldPx = _px;
	_oldSize = _size;
	_px = 0.0;
	_size = 0;
	_action = OrderAction::DELETED;
	
	LineAPI *api = line->_lineAPI;
	if(line->hasStrictInterest(BOOK))
	{
		_sub->_book.update(line, msgType, seqNo, exSeconds, exUSeconds);
		if(_line->_maintainOrders)
		{
			_sub->_book.deleteOrder(this);
		}
		else
		{
			_sub->_book.deleteOrder(_side, _oldPx, _oldSize);
		}

		if(!line->hasStrictInterest(QUOTE))
		{
			(api->*(api->_onBook))(&_sub->_book);
		}
	}
	else
	{
		(api->*(api->_onOrder))(this);
	}
	
	line->erase(_orderID.numeric());
	// we do not delete the implicitly constructed order objects
	// we just free them from order pool
	_orderPool->free(this);
	_orderPool = nullptr;
}

/*
 * Whole Order executed at same price
 */
inline void Order::Execute(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	Order *order = line->find(orderID);
	getLatency(line->_lineGroup->_om, mt);
#else
	Order *order = line->find(orderID);
#endif

	if (order)
	{
		order->_execute(line
			, msgType 
			, seqNo
			, exSeconds 
			, exUSeconds
			, order->_px
			, order->_size
			, 0);
	}
}

/*
 * Order executed partly at same price
 */
inline void Order::ExecuteSize(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, uint64_t execSize, uint64_t remaining)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	Order *order = line->find(orderID);
	getLatency(line->_lineGroup->_om, mt);
#else
	Order *order = line->find(orderID);
#endif

	if (order)
	{
		order->_execute(line
			, msgType 
			, seqNo
			, exSeconds 
			, exUSeconds
			, order->_px
			, execSize 
			, remaining);
	}
}

inline void Order::ExecuteSize(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, uint64_t execSize)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	Order *order = line->find(orderID);
	getLatency(line->_lineGroup->_om, mt);
#else
	Order *order = line->find(orderID);
#endif

	if (order)
	{
		order->_execute(line
			, msgType 
			, seqNo
			, exSeconds 
			, exUSeconds
			, order->_px
			, execSize 
			, order->_size - execSize);

	}
}


/*
 * Order executed at different price
 */
inline void Order::ExecutePxSize(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, double execPx, uint64_t execSize, uint64_t remaining)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	Order *order = line->find(orderID);
	getLatency(line->_lineGroup->_om, mt);
#else
	Order *order = line->find(orderID);
#endif

	if (order)
	{
		order->_execute(line
			, msgType 
			, seqNo
			, exSeconds 
			, exUSeconds
			, execPx 
			, execSize 
			, remaining);
	}
}

inline void Order::ExecutePxSize(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t orderID, double execPx, uint64_t execSize)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	Order *order = line->find(orderID);
	getLatency(line->_lineGroup->_om, mt);
#else
	Order *order = line->find(orderID);
#endif

	if (order)
	{
		order->_execute(line
			, msgType 
			, seqNo
			, exSeconds 
			, exUSeconds
			, execPx 
			, execSize
			, order->_size - execSize);
	}
}

// must be when order modify crosses the apposite side top of book
// Both order executed quantity and remaining display quantity
// are sent along with trade/execution price 
inline void Order::_execute(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, double execPx, uint64_t execSize, uint64_t remaining)
{
	Base::update(msgType, seqNo, exSeconds, exUSeconds);

	// price of order has not changed, just Trade has happned at execPx
	_oldPx = _px;
	_oldSize = _size;
	_size = remaining;

	//Remaining qunaity is +ve nonzero and is equal to _oldSize - execSize
	ASSERT(execSize == (_oldSize - remaining), "execSize : " << execSize << ", _oldSize : " << _oldSize << ", remaining : " << remaining);

	_action = OrderAction::EXECUTED;
	
	LineAPI *api = line->_lineAPI;
	if(line->hasStrictInterest(BOOK))
	{
		if(remaining)
		{
			_sub->_book.update(line, msgType, seqNo, exSeconds, exUSeconds);
			if(_line->_maintainOrders)
			{
				_sub->_book.modifyOrder(this);
			}
			else
			{
				_sub->_book.modifyOrder(_side, _px, _oldPx, _size, _oldSize);
			}
			if(!line->hasStrictInterest(QUOTE))
			{
				(api->*(api->_onBook))(&_sub->_book);
			}
		}
		else
		{
			_sub->_book.update(line, msgType, seqNo, exSeconds, exUSeconds);
			if(_line->_maintainOrders)
			{
				_sub->_book.deleteOrder(this);
			}
			else
			{
				_sub->_book.deleteOrder(_side, _oldPx, _oldSize);
			}
			
			if(!line->hasStrictInterest(QUOTE))
			{
				(api->*(api->_onBook))(&_sub->_book);
			}
		}
	}
	else
	{
		(api->*(api->_onOrder))(this);
	}
	
	if(!remaining)
	{
		line->erase(_orderID.numeric());
		// free the order to order pool 
		_orderPool->free(this);
	}
}

inline Order::~Order()
{
	_px = 0.0;
	_size = 0;
	_oldPx = 0.0;
	_oldSize = 0;
	_side = SIDE_UNKNOWN;
	_action = ORDACT_UNKNOWN;
	_next = nullptr;
	_level = nullptr;
	// ~Base::Base
}

#endif// __ORDER_H__
