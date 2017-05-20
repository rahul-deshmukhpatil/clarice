#include "base/API.h"
#include "base/Line.h"
#include "base/LineGroup.h"
#include "base/FeedHandler.h"

using namespace base;

extern "C"
CallBacks getCallback(const std::string updateType)
{
	if(updateType == "ORDER")
	{
		return ORDER;
	}
	else if(updateType == "BOOK")
	{
		return BOOK;
	}
	else if(updateType == "STATUS")
	{
		return STATUS;
	}
	else if(updateType == "QUOTE")
	{
		return QUOTE;
	}
	else if(updateType == "CUSTOM")
	{
		return CUSTOM;
	}
	else if(updateType == "ALL")
	{
		return ALL_CALLBACKS;
	}
	else
	{
		return NO_CALLBACK;
	}
}

void FeedAPI::onFeedStarted(const FeedHandler *feedHandler)
{
}

void FeedAPI::onFeedStopped(const FeedHandler *feedHandler)
{
}

FeedAPI::FeedAPI()
	: LineGroupAPI()
	, _onFeedStarted(&FeedAPI::onFeedStarted)
	, _onFeedStopped(&FeedAPI::onFeedStopped)
{
}

FeedAPI::FeedAPI(OnFeedStarted onFeedStarted, OnFeedStopped onFeedStopped, OnLineGroupStarted onLineGroupStarted, OnLineGroupStopped onLineGroupStopped, OnLineStarted onLineStarted, OnLineStopped onLineStopped, OnPacketStart onPacketStart, OnPacketEnd onPacketEnd, OnOrder onOrder, OnBook onBook, OnQuote onQuote, OnTrade onTrade, OnStatus onStatus, OnCustom onCustom, OnProductInfo onProductInfo)
	: LineGroupAPI(onLineGroupStarted, onLineGroupStopped, onLineStarted, onLineStopped, onPacketStart, onPacketEnd, onOrder , onBook , onQuote , onTrade , onStatus , onCustom , onProductInfo)
	, _onFeedStarted(onFeedStarted)
	, _onFeedStopped(onFeedStopped)
{
}

void LineGroupAPI::onLineGroupStarted(const LineGroup *lineGroup)
{
}

void LineGroupAPI::onLineGroupStopped(const LineGroup *lineGroup)
{
}

LineGroupAPI::LineGroupAPI()
	: LineAPI()
	, _onLineGroupStarted(&LineGroupAPI::onLineGroupStarted)
	, _onLineGroupStopped(&LineGroupAPI::onLineGroupStopped)
{
}

LineGroupAPI::LineGroupAPI(OnLineGroupStarted onLineGroupStarted, OnLineGroupStopped onLineGroupStopped, OnLineStarted onLineStarted, OnLineStopped onLineStopped, OnPacketStart onPacketStart, OnPacketEnd onPacketEnd, OnOrder onOrder, OnBook onBook, OnQuote onQuote, OnTrade onTrade, OnStatus onStatus, OnCustom onCustom, OnProductInfo onProductInfo)
	: LineAPI(onLineStarted, onLineStopped, onPacketStart, onPacketEnd, onOrder , onBook , onQuote , onTrade , onStatus , onCustom , onProductInfo)
	, _onLineGroupStarted(onLineGroupStarted)
	, _onLineGroupStopped(onLineGroupStopped)
{
}


void LineAPI::onLineStarted(const Line *pLine)
{
}

void LineAPI::onLineStopped(const Line *pLine)
{
}

void LineAPI::onPacketStart(const Line *pLine)
{
}

void LineAPI::onPacketEnd(const Line *pLine)
{
}

void LineAPI::onProductInfo(const ProductInfo *product)
{
}

void LineAPI::onOrder(const Order *order)
{
}

void LineAPI::onBook(const Book *book)
{
	
}

void LineAPI::onQuote(const Quote *quote)
{
	
}

void LineAPI::onTrade(const Trade *trade, const Order *order)
{
}

void LineAPI::onStatus(const Status *status)
{
}

void LineAPI::onCustom(const Custom *custom)
{

}

LineAPI::LineAPI()
	: _onLineStarted(&LineAPI::onLineStarted)
	, _onLineStopped(&LineAPI::onLineStopped)
	, _onPacketStart(&LineAPI::onPacketStart)
	, _onPacketEnd(&LineAPI::onPacketEnd)
	, _onOrder(&LineAPI::onOrder)
	, _onBook(&LineAPI::onBook)
	, _onQuote(&LineAPI::onQuote)
	, _onTrade(&LineAPI::onTrade)
	, _onStatus(&LineAPI::onStatus)
	, _onCustom(&LineAPI::onCustom)
	, _onProductInfo(&LineAPI::onProductInfo)
{
}

LineAPI::LineAPI(OnLineStarted onLineStarted, OnLineStopped onLineStopped, OnPacketStart onPacketStart, OnPacketEnd onPacketEnd, OnOrder onOrder, OnBook onBook, OnQuote onQuote, OnTrade onTrade, OnStatus onStatus, OnCustom onCustom, OnProductInfo onProductInfo)
	: _onLineStarted(onLineStarted)
	, _onLineStopped(onLineStopped)
	, _onPacketStart(onPacketStart)
	, _onPacketEnd(onPacketEnd)
	, _onOrder(onOrder)
	, _onBook(onBook)
	, _onQuote(onQuote)
	, _onTrade(onTrade)
	, _onStatus(onStatus)
	, _onCustom(onCustom)
	, _onProductInfo(onProductInfo)
{
}
