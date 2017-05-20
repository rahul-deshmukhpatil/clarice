
#include "feeds/omxnordic/OMXNordicFeedHandler.h"
#include "feeds/omxnordic/OMXNordicLineGroup.h"
#include "feeds/omxnordic/OMXNordicLine.h"

using namespace omxnordic;
using namespace base;

LineGroup* OMXNordicLineGroup::_OMXNordicLineGroup(const MarketDataApplication *app, FeedHandler *feedHandler, const pugi::xml_node& lineGroupNode, LineGroupAPI *lineGroupAPI)
{
	return new omxnordic::OMXNordicLineGroup(app, reinterpret_cast<omxnordic::OMXNordicFeedHandler *>(feedHandler), lineGroupNode, lineGroupAPI);
}

OMXNordicLineGroup:: OMXNordicLineGroup(const MarketDataApplication *app, OMXNordicFeedHandler *pOMXNordicFeedHandler, const pugi::xml_node & lineGroupNode, LineGroupAPI *lineGroupAPI)
	: LineGroup(app, pOMXNordicFeedHandler, lineGroupNode, OMXNordicLine::_OMXNordicLine, lineGroupAPI)
{
}

