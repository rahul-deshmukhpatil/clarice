
#include "base/MarketDataApplication.h"
#include "base/FeedHandler.h"
#include "feeds/omxnordic/OMXNordicFeedHandler.h"
#include "feeds/omxnordic/OMXNordicLineGroup.h"

using namespace omxnordic;
using namespace base;

//MarketDataApplication::feedHandlerConstructors[static_cast<int>(FeedID::OMXNORDIC)]=OMXNordicFeedHandler::createOMXNordicFeedHandler;
bool OMXNordicFeedHandler::_loaded = MarketDataApplication::registerFeedConstructors(FeedID::OMXNORDIC, _OMXNordicFeedHandler); 

FeedHandler* OMXNordicFeedHandler:: _OMXNordicFeedHandler(MarketDataApplication *app, const pugi::xml_node& feedHandlerNode, FeedAPI *feedAPI)
{
	return new OMXNordicFeedHandler(app, feedHandlerNode, feedAPI);
}

OMXNordicFeedHandler::OMXNordicFeedHandler(const MarketDataApplication *app, const pugi::xml_node & feedHandlerNode, FeedAPI *feedAPI)
	: FeedHandler(app, feedHandlerNode, FeedID::OMXNORDIC, OMXNordicLineGroup::_OMXNordicLineGroup, feedAPI)
{

}

