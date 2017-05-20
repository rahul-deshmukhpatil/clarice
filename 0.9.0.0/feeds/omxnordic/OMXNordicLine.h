#ifndef __OMXNORDICLINEHANDLER_H__
#define __OMXNORDICLINEHANDLER_H__

#include "base/Line.h"
#include "base/Subscription.h"
#include "feeds/omxnordic/OMXNordicCommon.h"
#include "feeds/omxnordic/OMXNordicMessages.h"

#include "base/Base.h"

using namespace base;

namespace omxnordic
{
	class OMXNordicLine : public Line
	{
		public:
			//@TODO: This should be static and should be called when lib gets loaded;
			void initializeParseFunctions();

			static Line* _OMXNordicLine(const MarketDataApplication *, FeedHandler *, LineGroup *, Thread *, const pugi::xml_node &, LineAPI *);
			OMXNordicLine(const MarketDataApplication *, OMXNordicFeedHandler *, OMXNordicLineGroup *, Thread *, const pugi::xml_node &, LineAPI *);

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

			// Application messages
			void exchangeTime();
			void exchangeMSeconds();
			void systemEvent();
			void stateMessage();
			void refData();
			void tradingState();

			void addOrder();
			void addOrderMPID();
			void orderExec();
			void orderExecPx();
			void orderCancel();
			void orderDelete();
			void trade();
			void crossTrade();
			void brokenTrade();
			void imbalance();
	};
}

#endif //__OMXNORDICLINEHANDLER_H__
