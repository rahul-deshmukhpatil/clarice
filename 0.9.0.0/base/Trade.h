#ifndef __TRADE_H__
#define __TRADE_H__

#include "base/BaseCommon.h"
#include "base/Base.h"

namespace base
{
	enum TradeBook 
	{
		TRADE_ONBOOK = 'N',
		TRADE_OFFBOOK = 'F', 
		TRADE_UNKNOWNBOOK = 'U'
	};

	enum TradeExchange
	{
		TRADE_ONEXCHANGE = 'N',
		TRADE_OFFEXCHANGE = 'F',
		TRADE_UNKNOWNEX	= 'U'
	};

    class Trade : public Base
    {
        public:
			Trade(Subscription *sub);
			Trade(Line *, Subscription *, uint32_t, uint64_t, uint32_t, uint32_t, double, uint32_t, Side, uint64_t);

			void update(Line *, uint32_t, uint64_t, uint32_t, uint32_t, double, uint32_t, Side, uint64_t);
			~Trade();
		
            double          _px;
            uint32_t		_size;
            Side			_side;
            ID				_tradeID;
			TradeBook		_tradeBook;
			TradeExchange	_tradeExchange;
    };
}

#include "base/BaseCommon.h"
#include "base/Subscription.h"
#include "base/Line.h"
#include "base/API.h"

using namespace base;

// when _sub->_trade is initialized
inline Trade::Trade(Subscription *sub)
	: Base(sub, TRADE_UPDATE)
	, _px(0)
	, _size(0)
	, _side(SIDE_UNKNOWN)
	, _tradeID()
	, _tradeBook(TRADE_UNKNOWNBOOK)
	, _tradeExchange(TRADE_UNKNOWNEX)
{
}

//Used for reporting the offbook trades 
inline Trade::Trade(Line *line, Subscription *sub, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, double px, uint32_t size, Side side, uint64_t tradeID)
	: Base(line, sub, msgType, seqNo, exSeconds, exUSeconds, TRADE_UPDATE)
	, _px(px)
	, _size(size)
	, _side(side)
	, _tradeID(tradeID)
	, _tradeBook(TRADE_ONBOOK)
	, _tradeExchange(TRADE_ONEXCHANGE)

{
}

// For updating regular onbook, onexchange trades _sub->_trade
inline void Trade::update(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, double px, uint32_t size, Side side, uint64_t tradeID)
{
	ASSERT( _line == line || _line == nullptr, "_line : " << _line << ", line : " << line);
	Base::update(line, msgType, seqNo, exSeconds, exUSeconds);
	_px = px;
	_size = size;
	_side = side;
	_tradeID = tradeID;
	_tradeBook = TRADE_ONBOOK;
	_tradeExchange = TRADE_ONEXCHANGE;
	LineAPI *api = _line->_lineAPI;
	(api->*(api->_onTrade))(this, nullptr);
}

inline Trade::~Trade()
{
	_px   = 0.0;
	_size    = 0;
	_side	= SIDE_UNKNOWN;
	_tradeBook = TRADE_UNKNOWNBOOK;
	_tradeExchange = TRADE_UNKNOWNEX;
	// ~Base::Base
}

#endif // __TRADE_H__ 
