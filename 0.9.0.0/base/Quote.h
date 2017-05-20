#ifndef __QUOTE_H__
#define __QUOTE_H__

namespace base
{
	enum QuoteType 
	{
		QUOTE_UNKNOWN = 'U',
		NORMAL = 'N',
		WIDE = 'W',
		CROSSED = 'C'
	};

	enum QuoteFill
	{
		FILL_UNKNOWN = 0,
		BOTH_SIDE	= 1,
		EMPTY_ASK	= 2,
		EMPTY_BID	= 3
	};

	enum QuoteChange
	{
		CHANGE_UNKNOWN = 0,
		PRICE_CHANGE = 1,
		SIZE_CHANGE = 2
	};

	class Quote : public Base
	{
		public:
		Quote(Subscription *);
		void cacheBuySideQuote();
		void checkBuySideQuote(Book *book);
		void cacheSellSideQuote();
		void checkSellSideQuote(Book *book);

//		QuoteType _quoteType; // crossed, normal
//		QuoteFill _fill;
//		Side	  _side; // which side has changed
		QuoteChange _change; // Price/Size 
		PriceLevel *_buy;
		PriceLevel *_sell;
		double _lastBuyPx;
		uint64_t _lastBuySize;
		double _lastSellPx;
		uint64_t _lastSellSize;
	};
}

#include "base/Book.h"
#include "base/PriceLevel.h"
#include "base/Line.h"
#include "base/API.h"
using namespace base;

inline Quote::Quote(Subscription *sub)
	: Base(sub, QUOTE_UPDATE)
//	, _quoteType(QUOTE_UNKNOWN)
//	, _fill(FILL_UNKNOWN)
//	, _side(SIDE_UNKNOWN)
	, _change(CHANGE_UNKNOWN)
	, _buy(nullptr)
	, _sell(nullptr)
	, _lastBuyPx(0)
	, _lastBuySize(0)
	, _lastSellPx(0)
	, _lastSellSize(0)
{
}

inline void Quote::cacheBuySideQuote()
{
	if(_buy)
	{
		_lastBuyPx = _buy->_px;
		_lastBuySize = _buy->_size;
	}
}

inline void Quote::checkBuySideQuote(Book *book)
{
	PriceLevelMap::const_iterator itr = book->_buys.begin();
	if(itr == book->_buys.end())
	{
		update(book->_line, book->_msgType, book->_seqNum, book->_xt.tv_sec, book->_xt.tv_nsec);
		_buy = nullptr;

		LineAPI *api = _line->_lineAPI;
		(api->*(api->_onQuote))(this);
		return; 
	}

	PriceLevel *topBuyLevel = itr->second; 
	if(_lastBuyPx != topBuyLevel->_px)
	{
		update(book->_line, book->_msgType, book->_seqNum, book->_xt.tv_sec, book->_xt.tv_nsec);
		_buy = topBuyLevel;

		LineAPI *api = _line->_lineAPI;
		(api->*(api->_onQuote))(this);
	}
	else if (_lastBuySize != topBuyLevel->_size)
	{
		//raise the quote
		update(book->_line, book->_msgType, book->_seqNum, book->_xt.tv_sec, book->_xt.tv_nsec);
		LineAPI *api = _line->_lineAPI;
		(api->*(api->_onQuote))(this);
	}
}

inline void Quote::cacheSellSideQuote()
{
	if(_sell)
	{
		_lastSellPx = _sell->_px;
		_lastSellSize = _sell->_size;
	}
}

inline void Quote::checkSellSideQuote(Book *book)
{
	PriceLevelMap::const_iterator itr = book->_sells.begin();
	if(itr == book->_sells.end())
	{
		update(book->_line, book->_msgType, book->_seqNum, book->_xt.tv_sec, book->_xt.tv_nsec);
		_sell = nullptr;

		LineAPI *api = _line->_lineAPI;
		(api->*(api->_onQuote))(this);
		return; 
	}

	PriceLevel *topSellLevel = itr->second; 
	if(_lastSellPx != topSellLevel->_px)
	{
		update(book->_line, book->_msgType, book->_seqNum, book->_xt.tv_sec, book->_xt.tv_nsec);
		_sell = topSellLevel;

		LineAPI *api = _line->_lineAPI;
		(api->*(api->_onQuote))(this);
	}
	else if (_lastSellSize != topSellLevel->_size)
	{
		//raise the quote
		update(book->_line, book->_msgType, book->_seqNum, book->_xt.tv_sec, book->_xt.tv_nsec);
		LineAPI *api = _line->_lineAPI;
		(api->*(api->_onQuote))(this);
	}
}
#endif //__QUOTE_H__
