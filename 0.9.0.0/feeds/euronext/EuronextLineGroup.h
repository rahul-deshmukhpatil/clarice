#ifndef __EURONEXT_CONNECTION_H__
#define __EURONEXT_CONNECTION_H__

#include "base/BaseCommon.h"
#include "base/LineGroup.h"

#include "feeds/euronext/EuronextCommon.h"

using namespace base;

namespace euronext
{
class EuronextLineGroup : public LineGroup
{
	public:
		static LineGroup* _EuronextLineGroup(const MarketDataApplication *app, FeedHandler *feedHandler, const pugi::xml_node& lineGroupNode, LineGroupAPI *lineGroupAPI);
		EuronextLineGroup(const MarketDataApplication *app, EuronextFeedHandler *pEuronextFeedHandler, const pugi::xml_node& lineGroupNode, LineGroupAPI *lineGroupAPI);
};
}
 #endif //__EURONEXT_CONNECTION_H__
