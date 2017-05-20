
#include "base/MarketDataApplication.h"
#include "base/FeedHandler.h"
#include "feeds/euronext/EuronextFeedHandler.h"
#include "feeds/euronext/EuronextLineGroup.h"

using namespace euronext;
using namespace base;

//MarketDataApplication::feedHandlerConstructors[static_cast<int>(FeedID::EURONEXT)]=EuronextFeedHandler::createEuronextFeedHandler;
bool EuronextFeedHandler::_loaded = MarketDataApplication::registerFeedConstructors(FeedID::EURONEXT, _EuronextFeedHandler); 

FeedHandler* EuronextFeedHandler:: _EuronextFeedHandler(MarketDataApplication *app, const pugi::xml_node& feedHandlerNode, FeedAPI *feedAPI)
{
	return new EuronextFeedHandler(app, feedHandlerNode, feedAPI);
}

EuronextFeedHandler::EuronextFeedHandler(const MarketDataApplication *app, const pugi::xml_node & feedHandlerNode, FeedAPI *feedAPI)
	: FeedHandler(app, feedHandlerNode, FeedID::EURONEXT, EuronextLineGroup::_EuronextLineGroup, feedAPI)
{

}

