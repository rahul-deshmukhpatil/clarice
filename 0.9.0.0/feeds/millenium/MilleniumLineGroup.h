#ifndef __MILLENIUM_CONNECTION_H__
#define __MILLENIUM_CONNECTION_H__

#include "base/BaseCommon.h"
#include "base/LineGroup.h"

#include "feeds/millenium/MilleniumCommon.h"

using namespace base;

namespace millenium
{
class MilleniumLineGroup : public LineGroup
{
	public:
		static LineGroup* _MilleniumLineGroup(const MarketDataApplication *app, FeedHandler *feedHandler, const pugi::xml_node& lineGroupNode, LineGroupAPI *lineGroupAPI);
		MilleniumLineGroup(const MarketDataApplication *app, MilleniumFeedHandler *pMilleniumFeedHandler, const pugi::xml_node& lineGroupNode, LineGroupAPI *lineGroupAPI);
};
}
 #endif //__MILLENIUM_CONNECTION_H__
