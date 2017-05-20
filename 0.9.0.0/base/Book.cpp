#include "base/Line.h"
#include "base/Order.h"
#include "base/PriceLevel.h"
#include "base/Book.h"
#include "base/Quote.h"

using namespace base;
using namespace infra;

Book::Book(Subscription *sub)
	: Base(sub, BOOK_UPDATE)
	, _buyQuantity(0)
	, _sellQuantity(0)
	, _buys()
	, _sells()
{
	//new(&_priceLevels) std::map<double, PriceLevel *, std::less<double>, BookAlloc<std::pair<const double, PriceLevel *> > >();
}

/**
 * See if level exists in the book
 * if exists, 
 *		add order to that level
 * else
 *		create the new level with order
 *		insert new level in the book
 */
void Book::addOrder(Order *order, bool modifyOrder)
{
	if(order->_side == BUY)
	{
		if(!modifyOrder)
		{
			_sub->_quote.cacheBuySideQuote();
		}
		
		std::pair<PriceLevelMap::iterator, PriceLevelMap::iterator> itr = _buys.equal_range(order->_px);

		//level already exists, just append this order to level
		if(itr.first != _buys.end() && itr.first->second->_px == order->_px)
		{
			itr.first->second->addOrder(order);
		}
		else
		{
			//This is new level, create and insert into map
			PriceLevel *pl = _line->_priceLevelPool->malloc();	
			new(pl) PriceLevel(order);
			_buys.insert(itr.second, std::make_pair(order->_px, pl));
		}

		// increase the cumulative quantity of the order book side
		_buyQuantity += order->_size;
		_sub->_quote.checkBuySideQuote(this);
	}
	else
	{
		if(!modifyOrder)
		{
			_sub->_quote.cacheSellSideQuote();
		}
		std::pair<PriceLevelMap::iterator, PriceLevelMap::iterator> itr = _sells.equal_range(order->_px);

		//level already exists, just append this order to level
		if(itr.first != _sells.end() && itr.first->second->_px == order->_px)
		{
			itr.first->second->addOrder(order);
		}
		else
		{
			//This is new level, create and insert into map
			PriceLevel *pl = _line->_priceLevelPool->malloc();	
			new(pl) PriceLevel(order);
			_sells.insert(itr.second, std::make_pair(order->_px, pl));
		}

		// increase the cumulative quantity of the order book side
		_sellQuantity += order->_size;
		_sub->_quote.checkSellSideQuote(this);
	}
}

void Book::addOrder(Side side, double px, uint64_t size, bool modifyOrder)
{
	if(side == BUY)
	{
		if(!modifyOrder)
		{
			_sub->_quote.cacheBuySideQuote();
		}
		std::pair<PriceLevelMap::iterator, PriceLevelMap::iterator> itr = _buys.equal_range(px);

		//level already exists, just append this order to level
		if(itr.first != _buys.end() && itr.first->second->_px == px)
		{
			itr.first->second->addOrder(size);
		}
		else
		{
			//This is new level, create and insert into map
			PriceLevel *pl = _line->_priceLevelPool->malloc();	
			new(pl) PriceLevel(px, size);
			_buys.insert(itr.second, std::make_pair(px, pl));
		}

		// increase the cumulative quantity of the order book side
		_buyQuantity += size;
		_sub->_quote.checkBuySideQuote(this);
	}
	else
	{
		if(!modifyOrder)
		{
			_sub->_quote.cacheSellSideQuote();
		}
		std::pair<PriceLevelMap::iterator, PriceLevelMap::iterator> itr = _sells.equal_range(px);

		//level already exists, just append this order to level
		if(itr.first != _sells.end() && itr.first->second->_px == px)
		{
			itr.first->second->addOrder(size);
		}
		else
		{
			//This is new level, create and insert into map
			PriceLevel *pl = _line->_priceLevelPool->malloc();	
			new(pl) PriceLevel(px, size);
			_sells.insert(itr.second, std::make_pair(px, pl));
		}

		// increase the cumulative quantity of the order book side
		_sellQuantity += size;
		_sub->_quote.checkSellSideQuote(this);
	}
}

/**
 * Search level based on the _oldpx of the order
 * if Price of the order has changed
 *		remove order from the level and insert at new level
 *	else
 *		just change the size of level
 *
 */
void Book::modifyOrder(Order *order)
{
	if(order->_px == order->_oldPx)
	{
		ASSERT(order->_size == order->_oldSize, "Book::modifyOrder has same old and new, price size");
		if(order->_side == BUY)
		{
			_sub->_quote.cacheBuySideQuote();
			order->_level->modifyOrder(order);
			_buyQuantity += order->_size - order->_oldSize;
			_sub->_quote.checkBuySideQuote(this);
		}
		else
		{
			_sub->_quote.cacheSellSideQuote();
			order->_level->modifyOrder(order);
			_sellQuantity += order->_size - order->_oldSize;
			_sub->_quote.checkSellSideQuote(this);
		}
	}
	else
	{
		// remove order from the oldPx level
		// insert at new _px level
		deleteOrder(order, true);
		addOrder(order, true);
	}
}

void Book::modifyOrder(Side side, double px, double oldPx, uint64_t size, uint64_t oldSize)
{
	if(px == oldPx)
	{
		ASSERT(order->_size == order->_oldSize, "Book::modifyOrder has same old and new, price size");
		if(side == BUY)
		{
			_sub->_quote.cacheBuySideQuote();
			PriceLevel* level = _buys.find(px)->second;
			level->modifyOrder(size, oldSize);
			_buyQuantity += size - oldSize;
			_sub->_quote.checkBuySideQuote(this);
		}
		else
		{
			_sub->_quote.cacheSellSideQuote();
			PriceLevel* level = _sells.find(px)->second;
			level->modifyOrder(size, oldSize);
			_sellQuantity += size - oldSize;
			_sub->_quote.checkSellSideQuote(this);
		}
	}
	else
	{
		// remove order from the oldPx level
		// insert at new _px level
		deleteOrder(side, oldPx, oldSize);
		addOrder(side, px, size);
	}
}

/**
 * Delete the order from price Level
 * If no order remaining on level,  delete the level
 */
void Book::deleteOrder(Order *order, bool modifyOrder)
{
	order->_level->deleteOrder(order);

	// last order deleted, so remove complete level off the book
	// and free it back to the pool
	if(order->_side == BUY)
	{
		_sub->_quote.cacheBuySideQuote();
		if(!order->_level->_size)
		{
			_line->_priceLevelPool->free(order->_level);	
			_buys.erase(order->_oldPx);
		}
		order->_level = nullptr;
		_buyQuantity += order->_size - order->_oldSize;
		if(!modifyOrder)
		{
			_sub->_quote.checkBuySideQuote(this);
		}
	}
	else
	{
		_sub->_quote.cacheSellSideQuote();
		if(!order->_level->_size)
		{
			_line->_priceLevelPool->free(order->_level);	
			_sells.erase(order->_oldPx);
		}
		order->_level = nullptr;
		_sellQuantity += order->_size - order->_oldSize;
		if(!modifyOrder)
		{
			_sub->_quote.checkSellSideQuote(this);
		}
	}
}

void Book::deleteOrder(Side side, double oldPx, uint64_t oldSize, bool modifyOrder)
{
	if(side == BUY)
	{
		_sub->_quote.cacheBuySideQuote();
		PriceLevelMap::iterator itr = _buys.find(oldPx);
		itr->second->deleteOrder(oldSize);
		if(!itr->second->_size)
		{
			_line->_priceLevelPool->free(itr->second);	
			_buys.erase(itr);
		}
		_buyQuantity -= oldSize;
		if(!modifyOrder)
		{
			_sub->_quote.checkBuySideQuote(this);
		}
	}
	else
	{
		_sub->_quote.cacheSellSideQuote();
		PriceLevelMap::iterator itr = _sells.find(oldPx);
		itr->second->deleteOrder(oldSize);
		if(!itr->second->_size)
		{
			_line->_priceLevelPool->free(itr->second);	
			_sells.erase(itr);
		}
		_sellQuantity -= oldSize;
		if(!modifyOrder)
		{
			_sub->_quote.checkSellSideQuote(this);
		}
	}
}
