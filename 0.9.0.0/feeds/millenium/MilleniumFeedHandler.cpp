
#include "base/MarketDataApplication.h"
#include "base/FeedHandler.h"
#include "feeds/millenium/MilleniumFeedHandler.h"
#include "feeds/millenium/MilleniumLineGroup.h"

using namespace millenium;
using namespace base;

//MarketDataApplication::feedHandlerConstructors[static_cast<int>(FeedID::MILLENIUM)]=MilleniumFeedHandler::createMilleniumFeedHandler;
bool MilleniumFeedHandler::_loaded = MarketDataApplication::registerFeedConstructors(FeedID::MILLENIUM, _MilleniumFeedHandler); 

FeedHandler* MilleniumFeedHandler:: _MilleniumFeedHandler(MarketDataApplication *app, const pugi::xml_node& feedHandlerNode, FeedAPI *feedAPI)
{
	return new MilleniumFeedHandler(app, feedHandlerNode, feedAPI);
}

MilleniumFeedHandler::MilleniumFeedHandler(const MarketDataApplication *app, const pugi::xml_node & feedHandlerNode, FeedAPI *feedAPI)
	: FeedHandler(app, feedHandlerNode, FeedID::MILLENIUM, MilleniumLineGroup::_MilleniumLineGroup, feedAPI)
{

}

