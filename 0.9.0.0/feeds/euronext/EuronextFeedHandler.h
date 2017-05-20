#ifndef __EURONEXT_FEEDMANAGER_H__
#define __EURONEXT_FEEDMANAGER_H__

#include "base/BaseCommon.h"
#include "base/FeedHandler.h"

using namespace base;

namespace euronext
{
	class EuronextFeedHandler: public FeedHandler
	{
		public:
			static bool _loaded;
			static FeedHandler* _EuronextFeedHandler(MarketDataApplication *app, const pugi::xml_node& feedHandlerNode, FeedAPI *feedAPI);

			EuronextFeedHandler(const MarketDataApplication *app, const pugi::xml_node & feedHandlerNode, FeedAPI *feedAPI);
	};
}
#endif //__EURONEXT_FEEDMANAGER_H__
