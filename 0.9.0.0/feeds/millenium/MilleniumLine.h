#ifndef __MILLENIUMLINEHANDLER_H__
#define __MILLENIUMLINEHANDLER_H__

#include "base/Line.h"
#include "base/Subscription.h"
#include "feeds/millenium/MilleniumCommon.h"
#include "feeds/millenium/MilleniumMessages.h"

#include "base/Base.h"

using namespace base;

namespace millenium
{
	class MilleniumLine : public Line
	{
		public:
			//@TODO: This should be static and should be called when lib gets loaded;
			void initializeParseFunctions();

			static Line* _MilleniumLine(const MarketDataApplication *, FeedHandler *, LineGroup *, Thread *, const pugi::xml_node &, LineAPI *);
			MilleniumLine(const MarketDataApplication *, MilleniumFeedHandler *, MilleniumLineGroup *, Thread *, const pugi::xml_node &, LineAPI *);

			static void _start(Line *pLine);
			void start();

			static void _getPacketStats(Line *pLine);
			void getPacketStats();

			static void _getPacketStatsSnap(Line *pLine);
			void getPacketStatsSnap();

			static bool _reRequestMissedPackets(Line *pLine, uint64_t first, uint64_t last);
			bool reRequestMissedPackets(uint64_t first, uint64_t last);

			void processPacket();
			void processMsg_rev();

			// System Messages
			void loginRequest();
			void loginResponse();
			void logoutRequest();
			void replayRequest();
			void replayResponse();
			void snapshotRequest();
			void snapshotResponse();
			void snapshotComplete();

			// Application messages
			void exchangeTime();
			void systemEvent();
			void symDir();
			void symStatus();
			void addOrder();
			void orderDeleted();
			void orderModified();
			void orderBookClear();
			void orderExec();
			void orderExecPxSize();
			void trade();
			void offBookTrade();
			void tradeBreak();
			void auctionInfo();
			void statistics();
			void enhancedTrade();
			void recoveryTrade();
	};
}

#endif //__MILLENIUMLINEHANDLER_H__
