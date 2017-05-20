#include "base/PriceLevel.h"
#include "base/Order.h"

using namespace base;

/**
 * PriceLevel: constructor
 * 		construct the level with Order
 */
PriceLevel::PriceLevel(Order *order)
	: _px(order->_px)
	, _size(order->_size)
	, _orders(order)
	, _numOrders(1)
{
	// if modified px order added at new level 
	// still ->_next should be nullptr
	ASSERT(order->_next == nullptr, "order->_next : " << order->_next);
	order->_level = this;
}

PriceLevel::PriceLevel(double px, uint64_t size)
	: _px(px)
	, _size(size)
	, _orders(nullptr)
	, _numOrders(1)
{
}



/*
 *  Adding another order to existing level "
 */
void PriceLevel::addOrder(Order *order)
{
	ASSERT(_px == order->_px, "_px : " << _px << ", order->_px : " << order->_px);
	ASSERT(order->_size, "order->_size :" << order->_size);

	_size += order->_size; 
	order->_level = this;

	order->_next = _orders;
	_orders = order;

	++_numOrders;
}

void PriceLevel::addOrder(uint64_t size)
{
	_size += size; 
	++_numOrders;
}

// only modify size from order
void PriceLevel::modifyOrder(Order *order)
{
	//update(order->_msgType, order->_seqNum, order->_xt.tv_sec, order->_xt.tv_nsec);

	ASSERT(_px == order->_px, "_px : " << _px << ", order->_px" << order->_px);
	ASSERT(order->_size, "order->_size :" << order->_size);

	_size += order->_size - order->_oldSize; 
}

// only modify size from order
void PriceLevel::modifyOrder(uint64_t size, uint64_t oldSize)
{
	//update(order->_msgType, order->_seqNum, order->_xt.tv_sec, order->_xt.tv_nsec);

	_size += size - oldSize; 
}

/**
 * Delete the order from price Level
 * If no order remaining on level,  delete the level
 */
void PriceLevel::deleteOrder(Order *order)
{
	//update(order->_msgType, order->_seqNum, order->_xt.tv_sec, order->_xt.tv_nsec);

	_size -= order->_oldSize; 

	//@TODO: save order in the reverse order of addition
	//		ie. old order must be first one
	if(_orders == order)
	{
		_orders = order->_next;
	}
	else
	{
		Order *temp = _orders;
		while(temp->_next != order)
		{
			temp = temp->_next;
		}
		temp->_next = order->_next;
	}
	order->_next = nullptr;
	--_numOrders;
}

/**
 * Delete the order from price Level
 * If no order remaining on level,  delete the level
 */
void PriceLevel::deleteOrder(uint64_t oldSize)
{
	//update(order->_msgType, order->_seqNum, order->_xt.tv_sec, order->_xt.tv_nsec);

	_size -= oldSize; 
	--_numOrders;
}
