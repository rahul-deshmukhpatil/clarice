#ifndef __API_H__
#define __API_H__

#include <string>
#include "base/BaseCommon.h"

namespace base
{
	extern "C"
	CallBacks getCallback(const std::string updateType);

	class LineAPI
	{
		public:
			typedef void (LineAPI::*OnLineStarted)(const Line *pLine);
			typedef void (LineAPI::*OnLineStopped)(const Line *pLine);
			typedef void (LineAPI::*OnPacketStart)(const Line *pLine);
			typedef void (LineAPI::*OnPacketEnd)(const Line *pLine);
			typedef void (LineAPI::*OnOrder)(const Order *);
			typedef void (LineAPI::*OnBook)(const Book *);
			typedef void (LineAPI::*OnQuote)(const Quote *);
			typedef void (LineAPI::*OnTrade)(const Trade *, const Order *);
			typedef void (LineAPI::*OnStatus)(const Status *);
			typedef void (LineAPI::*OnCustom)(const Custom *);
			typedef void (LineAPI::*OnProductInfo)(const ProductInfo *);

			void onLineStarted(const Line *pLine);
			void onLineStopped(const Line *pLine);
			void onPacketStart(const Line *pLine);
			void onPacketEnd(const Line *pLine);

			void onOrder(const Order *);
			void onBook(const Book *);
			void onQuote(const Quote *);
			void onTrade(const Trade *, const Order *);
			void onStatus(const Status *);
			void onCustom(const Custom *);
			void onProductInfo(const ProductInfo *);

			LineAPI();
			LineAPI(OnLineStarted , OnLineStopped , OnPacketStart , OnPacketEnd, OnOrder, OnBook , OnQuote , OnTrade , OnStatus , OnCustom , OnProductInfo );
			
			OnLineStarted _onLineStarted;
			OnLineStopped _onLineStopped;
			OnPacketStart _onPacketStart;
			OnPacketEnd	  _onPacketEnd;

			OnOrder _onOrder;
			OnBook _onBook;
			OnQuote _onQuote;
			OnTrade	_onTrade;
			OnStatus _onStatus;
			OnCustom _onCustom;
			OnProductInfo _onProductInfo;
	};

	
	class LineGroupAPI: public LineAPI
	{
		public:
			typedef void (LineGroupAPI::*OnLineGroupStarted)(const LineGroup *lineGroup);
			typedef void (LineGroupAPI::*OnLineGroupStopped)(const LineGroup *lineGroup);

			void onLineGroupStarted(const LineGroup *lineGroup);
			void onLineGroupStopped(const LineGroup *lineGroup);

			LineGroupAPI();	
			LineGroupAPI(OnLineGroupStarted , OnLineGroupStopped , OnLineStarted , OnLineStopped , OnPacketStart , OnPacketEnd, OnOrder , OnBook , OnQuote , OnTrade , OnStatus , OnCustom , OnProductInfo);
			OnLineGroupStarted _onLineGroupStarted;
			OnLineGroupStopped _onLineGroupStopped;
	};
	
	class FeedAPI : public LineGroupAPI
	{
		public:
			typedef void (FeedAPI::*OnFeedStarted)(const FeedHandler *feedHandler);
			typedef void (FeedAPI::*OnFeedStopped)(const FeedHandler *feedHandler);
			
			void onFeedStarted(const FeedHandler *feedHandler);
			void onFeedStopped(const FeedHandler *feedHandler);

			FeedAPI();
			FeedAPI(OnFeedStarted , OnFeedStopped , OnLineGroupStarted , OnLineGroupStopped , OnLineStarted , OnLineStopped , OnPacketStart , OnPacketEnd, OnOrder , OnBook , OnQuote , OnTrade , OnStatus , OnCustom , OnProductInfo);
			OnFeedStarted _onFeedStarted;
			OnFeedStopped _onFeedStopped;
	};
}
#endif //__API_H__
