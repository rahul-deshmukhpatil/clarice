#ifndef __MILLENIUM_FEEDMANAGER_H__
#define __MILLENIUM_FEEDMANAGER_H__

#include "base/BaseCommon.h"
#include "base/FeedHandler.h"

using namespace base;

namespace millenium
{
	class MilleniumFeedHandler: public FeedHandler
	{
		public:
			static bool _loaded;
			static FeedHandler* _MilleniumFeedHandler(MarketDataApplication *app, const pugi::xml_node& feedHandlerNode, FeedAPI *feedAPI);

			MilleniumFeedHandler(const MarketDataApplication *app, const pugi::xml_node & feedHandlerNode, FeedAPI *feedAPI);
	};
}
#endif //__MILLENIUM_FEEDMANAGER_H__
