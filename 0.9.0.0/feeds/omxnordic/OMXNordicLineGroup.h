#ifndef __OMXNORDIC_CONNECTION_H__
#define __OMXNORDIC_CONNECTION_H__

#include "base/BaseCommon.h"
#include "base/LineGroup.h"

#include "feeds/omxnordic/OMXNordicCommon.h"

using namespace base;

namespace omxnordic
{
class OMXNordicLineGroup : public LineGroup
{
	public:
		static LineGroup* _OMXNordicLineGroup(const MarketDataApplication *app, FeedHandler *feedHandler, const pugi::xml_node& lineGroupNode, LineGroupAPI *lineGroupAPI);
		OMXNordicLineGroup(const MarketDataApplication *app, OMXNordicFeedHandler *pOMXNordicFeedHandler, const pugi::xml_node& lineGroupNode, LineGroupAPI *lineGroupAPI);
};
}
 #endif //__OMXNORDIC_CONNECTION_H__
