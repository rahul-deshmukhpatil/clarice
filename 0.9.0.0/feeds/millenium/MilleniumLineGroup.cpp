
#include "feeds/millenium/MilleniumFeedHandler.h"
#include "feeds/millenium/MilleniumLineGroup.h"
#include "feeds/millenium/MilleniumLine.h"

using namespace millenium;
using namespace base;

LineGroup* MilleniumLineGroup::_MilleniumLineGroup(const MarketDataApplication *app, FeedHandler *feedHandler, const pugi::xml_node& lineGroupNode, LineGroupAPI *lineGroupAPI)
{
	return new millenium::MilleniumLineGroup(app, reinterpret_cast<millenium::MilleniumFeedHandler *>(feedHandler), lineGroupNode, lineGroupAPI);
}

MilleniumLineGroup:: MilleniumLineGroup(const MarketDataApplication *app, MilleniumFeedHandler *pMilleniumFeedHandler, const pugi::xml_node & lineGroupNode, LineGroupAPI *lineGroupAPI)
	: LineGroup(app, pMilleniumFeedHandler, lineGroupNode, MilleniumLine::_MilleniumLine, lineGroupAPI)
{
}

