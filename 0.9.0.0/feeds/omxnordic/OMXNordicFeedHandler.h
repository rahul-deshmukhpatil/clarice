#ifndef __OMXNORDIC_FEEDMANAGER_H__
#define __OMXNORDIC_FEEDMANAGER_H__

#include "base/BaseCommon.h"
#include "base/FeedHandler.h"

using namespace base;

namespace omxnordic
{
	class OMXNordicFeedHandler: public FeedHandler
	{
		public:
			static bool _loaded;
			static FeedHandler* _OMXNordicFeedHandler(MarketDataApplication *app, const pugi::xml_node& feedHandlerNode, FeedAPI *feedAPI);

			OMXNordicFeedHandler(const MarketDataApplication *app, const pugi::xml_node & feedHandlerNode, FeedAPI *feedAPI);
	};
}
#endif //__OMXNORDIC_FEEDMANAGER_H__
