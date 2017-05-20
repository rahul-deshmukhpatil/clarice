#include <string.h>
#include "base/Subscription.h"

using namespace base;

Subscription::Subscription(const uint64_t symbolID, bool activate)
	: _symbolID(symbolID)
	, _isActive(activate)
	, _isin()
	, _prodInfo(nullptr)
	, _status(this)
	, _trade(this)
	, _book(this)
	, _quote(this)
{
	strncpy(_symbol, "", sizeof(_symbol));
}

Subscription::Subscription(const char *symbol, bool activate)
	: _symbolID(0)
	, _isActive(activate)
	, _isin()
	, _prodInfo(nullptr)
	, _status(this)
	, _trade(this)
	, _book(this)
	, _quote(this)
{
	strncpy(const_cast<char *>(_symbol), symbol, sizeof(_symbol));
}

const char * InstrStateToString(InstrState state)
{
	switch(state)
	{
		case InstrState::UNKNOWN:
			return "UNKNOWN";

		case InstrState::PREOPEN :
			return "PREOPEN";

		case InstrState::AUCTION :
			return "AUCTION";

		case InstrState::AUCTION_OPEN :
			return "AUCTION_OPEN";

		case InstrState::AUCTION_INTRA :
			return "AUCTION_INTRA";

		case InstrState::AUCTION_VOLA :
			return "AUCTION_VOLA";

		case InstrState::AUCTION_CLOSE :
			return "AUCTION_CLOSE";

		case InstrState::OPEN :
			return "OPEN";

		case InstrState::CLOSED :
			return "CLOSED";

		case InstrState::SUSPENDED :
			return "SUSPENDED";

		case InstrState::HALTED :
			return "HALTED";

		default:
			return "INVALID";
	}
}
