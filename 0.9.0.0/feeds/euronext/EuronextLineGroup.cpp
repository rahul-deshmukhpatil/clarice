
#include "feeds/euronext/EuronextFeedHandler.h"
#include "feeds/euronext/EuronextLineGroup.h"
#include "feeds/euronext/EuronextLine.h"

using namespace euronext;
using namespace base;

LineGroup* EuronextLineGroup::_EuronextLineGroup(const MarketDataApplication *app, FeedHandler *feedHandler, const pugi::xml_node& lineGroupNode, LineGroupAPI *lineGroupAPI)
{
	return new euronext::EuronextLineGroup(app, reinterpret_cast<euronext::EuronextFeedHandler *>(feedHandler), lineGroupNode, lineGroupAPI);
}

EuronextLineGroup:: EuronextLineGroup(const MarketDataApplication *app, EuronextFeedHandler *pEuronextFeedHandler, const pugi::xml_node & lineGroupNode, LineGroupAPI *lineGroupAPI)
	: LineGroup(app, pEuronextFeedHandler, lineGroupNode, EuronextLine::_EuronextLine, lineGroupAPI)
{
}

