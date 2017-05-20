#include "infra/logger/Logger.h"
#include "infra/utils/StringUtils.h"
#include "base/MarketDataApplication.h"
#include "base/BaseCommon.h"
#include "base/NetworkReader.h"
#include "base/FeedHandler.h"
#include "base/LineGroup.h"
#include "base/Subscription.h"
#include "base/ProductInfo.h"
#include "base/API.h"

using namespace base;
using namespace infra;

FeedHandler::FeedHandler(const MarketDataApplication *app, const pugi::xml_node &feedHandlerNode, FeedID feedID, LineGroupConstructor func, FeedAPI *feedAPI)
	: _feedAPI(feedAPI)
	, _feedID(feedID)
	, _name()
	, _mode(playbackMode(feedHandlerNode.getAttributeAsString("playback-mode", "", false, "file,live")))
	, _playbackFilesVector(_mode == PlaybackMode::FILE_PLAYBACK ? tokenize(feedHandlerNode.getAttributeAsString("playback-files", "", false, ""), ","): std::vector<std::string> ()) 
	, _lineGroupConstructor(func)
	, _appInstance(app)
	, _lineGroupMap()
	, _logger(nullptr)
	, _subMapMutex()	
	//@TODO: search what is this constant 255 means
	, _symbolSubMap(255)	
	, _isinSubMap(255)	
	, _prodInfoMap(255)	
	, _prodInfoMapISIN(255)	
	, _reRecord(feedHandlerNode.getAttributeAsBool("re-record", false))
	, _maintainOrders(feedHandlerNode.getAttributeAsBool("maintain-orders", false))
	, _recording(nullptr)
{
	strncpy(const_cast<char *>(_name), feedHandlerNode.getAttributeAsCString("name", "", false, ""), sizeof(_name));
	
	bool record = feedHandlerNode.getAttributeAsBool("record", false);
	if(record)
	{
		std::string location = feedHandlerNode.getAttributeAsCString("record-location", "");
		std::string fileName;

		if(!location.empty())
		{
			fileName = location + "/" + std::string(_name) + ".cl.gz"; 
		}
		else
		{
			fileName = std::string(_name) + ".cl.gz"; 
		}

		_recording = new Recording(this, fileName, true);
	}

	///< create the LineGroups under this feed
	createLineGroups(feedHandlerNode);

	//register callbacks to the line
	registerCallbacks(feedHandlerNode.getAttributeAsCString("callbacks", ""));
}
			
void FeedHandler::registerCallbacks(CallBacks callbacks)
{
	NameToLineGroupMap::iterator itr = _lineGroupMap.begin();

	for( ; itr != _lineGroupMap.end(); itr++)
	{
		itr->second->registerCallbacks(callbacks);		
	}
}

void FeedHandler::registerCallbacks(std::string updateTypes)
{
	std::vector<std::string> result = tokenize(updateTypes, ",");

	CallBacks callbacks;
	CLEAR_CALLBACKS(callbacks);
	
	for(int i = 0; i < result.size(); i++)
	{
		std::string updateType = result[i];
		SET_CALLBACK(callbacks, getCallback(updateType));
	}

	registerCallbacks(callbacks);
}

FeedHandler::~FeedHandler()
{
	//_feedID = FeedID::UNKNOWN_FEED;
	//_name[0] = '\0';
	//_mode;
	//_playbackFilesVector
	_reRecord = false;
	// _recordingMutex
	delete _recording;
	//_lineGroupConstructor
	_appInstance = nullptr;
	//@TODO: delete _lineGroupMap
	_logger = nullptr;
	_recordPlayback = false;
	//_pbLocation
	//_ipToLineGroupMap
}

/**
 *	\brief: createLineGroups
 *			Creates all Ling Groups. 
 */
void FeedHandler::createLineGroups(const pugi::xml_node &configNode)
{
	pugi::xml_node lineGroups = configNode.child("LineGroups");
	if(lineGroups)
	{
		int i = 0;
		for (pugi::xml_node_iterator it = lineGroups.begin(); it != lineGroups.end(); ++it)
		{
			pugi::xml_node lineGroupNode = *it;
			std::string nameOfChildNode = lineGroupNode.name();
			if(nameOfChildNode == "LineGroup")
			{
				++i;
				char buff[10];
				snprintf(buff, 10, "%d", i);
				std::string lineGroupName = lineGroupNode.getAttributeAsString("name", "", false , "");
			
				const NameToLineGroupMap::iterator itr = _lineGroupMap.find(lineGroupName);
				if(itr == _lineGroupMap.end())
				{
					LineGroup *lineGroup = _lineGroupConstructor(_appInstance, this, lineGroupNode, _feedAPI);
					_lineGroupMap.insert(std::make_pair(lineGroupName, lineGroup));
					logConsole(DEBUG, "Inserted lineGroup [ %s : %p ] into map", lineGroupName.c_str(), lineGroup);
				}
				else
				{
					///< Throw an error exception duplicate lineGroup name 
					char errorBuff[512];
					
					snprintf(errorBuff, sizeof(errorBuff), "Duplicate LineGroup Name found in config. LineGroup is already present %s : %p into map", lineGroupName.c_str(), itr->second);
					std::string error(errorBuff);

					throw std::invalid_argument(error);
				}
			}
			else
			{
				///< Unintended node specified in the config file, raise exception log 
				logConsole(WARN, "<LineGroups> config node has unexpected child node <%s>", nameOfChildNode.c_str());
			}
		}
	}
}

/**
 *	\brief: start
 *		starts all the underlying line group threads
 */
void FeedHandler::start(const Thread* _thread) const
{
	logMessage(INFO, "Feed Handler [ %s : %p ] is starting LineGroups ", _name, this);
	NameToLineGroupMap::const_iterator itr = _lineGroupMap.begin();
	for( ; itr != _lineGroupMap.end(); itr++)
	{
		itr->second->start();
	}
	(_feedAPI->*(_feedAPI->_onFeedStarted))(this);
	logMessage(INFO, "Feed Handler [ %s : %p ] is started", _name, this);
}

/**
 *	\brief: stop
 *		stops all the underlying line group threads
 */
void FeedHandler::stop(const Thread* _thread) const
{
	logMessage(DEBUG, "Feed Handler [ %s : %p ] is stoping", _name, this);
	NameToLineGroupMap::const_iterator itr = _lineGroupMap.begin();
	for( ; itr != _lineGroupMap.end(); itr++)
	{
		itr->second->stop();
	}
	(_feedAPI->*(_feedAPI->_onFeedStopped))(this);
	logMessage(INFO, "Feed Handler [ %s : %p ] is stopped", _name, this);
}

/**
 *	\brief: printStats 
 *		print stats of all the underlying line group threads
 */
void FeedHandler::printStats(const Thread *_thread) const
{
	logMessage(INFO, "====================================================================================");
	logMessage(INFO, "Feed Handler [ %s : %p ] stats", _name, this);
	logMessage(INFO, "====================================================================================");
	NameToLineGroupMap::const_iterator itr = _lineGroupMap.begin();
	for( ; itr != _lineGroupMap.end(); itr++)
	{
		itr->second->printStats(_thread);
	}
	logMessage(INFO, "====================================================================================");
}

/*
 * subscribe using symbol id 
 */
int32_t FeedHandler::subscribe(const uint64_t symbolID, Subscription **retSub)
{
	WLock wlock(_subMapMutex);

	auto itr = _idSubMap.find(symbolID);		

	if(itr != _idSubMap.end())
	{
		// Activate the subscription
		//"TODO: assert if subscription is already activated
		Subscription *sub = itr->second;
		sub->_isActive.store(true, std::memory_order_release);
		*retSub = sub; 
		return SUCESS;
	}

	// create the new active subscrition
	*retSub = new Subscription(symbolID, true);
	_idSubMap.insert(make_pair(symbolID, *retSub));	
	return SUCESS;
}

bool ifStringIsRegEx(const char *str)
{
	while(*str)
	{
		if(!isalnum(*str))
		{
			return true;
		}
		str++;
	}
	return false;
}

bool ifStringIsNum(const char *str)
{
	while(*str)
	{
		if(!isdigit(*str))
		{
			return false;
		}
		str++;
	}
	return true;
}

bool ifStringIsISIN(const char *str)
{
	if(strlen(str) != 12)
	{
		return false;
	}

	if(isupper(str[0]) && isupper(str[1]))
	{
		for(int i = 2; i < 11; i++)
		{
			if(!isdigit(str[i]))
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}

int32_t FeedHandler::subscribe(const char *symbol, Subscription **retSub)
{
	WLock wlock(_subMapMutex);

	if(ifStringIsRegEx(symbol))
	{
		logConsole(INFO, "Subscribed regex subscription %s to FeedHandler [%s : %p] ", symbol, _name, this);
		std::regex expr = std::regex(symbol, std::regex_constants::grep);
		_regexMap.push_back(expr);
		
		judySArray<Subscription *>::pair symbolSub = _symbolSubMap.begin();
		while(symbolSub.value)
		{
			// if inactive subscription matches newly added regex 
			// activate that subscription with callbacks and subAPI
			Subscription *sub = symbolSub.value;
			if(std::regex_match(sub->_symbol, expr))
			{
				if(!sub->_isActive)
				{
					sub->_isActive.store(true, std::memory_order_release);
				}
			}
			symbolSub = _symbolSubMap.next();
		}

		auto itrIdSubMap = _idSubMap.begin();
		if(itrIdSubMap != _idSubMap.end())
		{
			// if inactive subscription matches newly added regex 
			// activate that subscription with callbacks and subAPI
			Subscription *sub = itrIdSubMap->second;
			if(std::regex_match(std::to_string(sub->_symbolID), expr))
			{
				if(!sub->_isActive)
				{
					sub->_isActive.store(true, std::memory_order_release);
				}
			}
		}

		// We just added regex 
		return REGEX_SUBSCRIPTION;
	}
	else
	{
		// Check if its a number then its a symbolID subscription
		// Check if its a isin subscription
		// then its a  normal symbol subscription

		Subscription *sub;

		if(ifStringIsNum(symbol))
		{
			uint64_t symbolID = atoll(symbol); 
			auto itr = _idSubMap.find(symbolID);
			if(itr != _idSubMap.end())
			{
				// Activate the subscription
				//"TODO: assert if subscription is already activated
				sub = itr->second;
				sub->_isActive.store(true, std::memory_order_release);
				*retSub = sub; 
				return SUCESS;
			}
		}
		else
		{
			if(ifStringIsISIN(symbol))
			{
				sub = _isinSubMap.find(symbol);
			}
			else
			{
				sub = _symbolSubMap.find(symbol);
			}
			
			if(sub)
			{
				// Activate the subscription or overide the subAPIs and callbacks
				//"TODO: assert if subscription is already activated
				sub->_isActive.store(true, std::memory_order_release);
				*retSub = sub; 
				return SUCESS;
			}
		}

		// create the new active subscrition
		*retSub = new Subscription(symbol, true);
		_symbolSubMap.insert(symbol, *retSub);	
		return SUCESS;
	}
}

/*
 *	Get the symbol subscription which must be active
 */
Subscription* FeedHandler::getIdSub(uint64_t symbolID) 
{
	// _idSubMap might get modified
	WLock wlock(_subMapMutex);
	return _getIdSub(symbolID);
}

/*
 *	Get the symbol subscription which must be active
 */
Subscription* FeedHandler::_getIdSub(uint64_t symbolID) 
{
	// _idSubMap might get modified
	auto itr = _idSubMap.find(symbolID);

	//Got the subscription and it is activ one
	if(itr != _idSubMap.end())
	{
		return itr->second; 
	}
	else
	{
		// create the new active subscrition
		ProductInfo* pInfo = _getProductInfo(nullptr, 0, symbolID, "", "", 0, 0, 0);
		Subscription *sub = pInfo->_sub;

		for(auto itr : _regexMap)
		{
			if(std::regex_match(std::to_string(symbolID), itr))
			{
				sub->_isActive.store(true, std::memory_order_release);
			}
		}

		_idSubMap.insert(make_pair(symbolID, sub));	
		return sub;
	}
}

/*
 *	Get the symbol subscription which must be active
 */
Subscription* FeedHandler::getSymbolSub(const char *symbol) 
{
	// _idSubMap might get modified
	WLock wlock(_subMapMutex);
}

Subscription* FeedHandler::_getSymbolSub(const char *symbol) 
{
	Subscription *sub = _symbolSubMap.find(symbol);

	//Got the subscription and it is activ one
	if(sub)
	{
		return sub; 
	}
	else
	{
/*
		// create the new active subscrition
		sub = new Subscription(symbol, NO_CALLBACK, nullptr, false);

		for(auto itr : _regexMap)
		{
			if(std::regex_match(symbol, itr))
			{
				sub->_isActive.store(true, std::memory_order_release);
			}
		}
		_symbolSubMap.insert(symbol, sub);	
		return sub;
*/
		return nullptr;
	}
}

FeedID FeedHandler::getFeedID(const std::string& feedHandlerType)
{
	if(feedHandlerType == "millenium")
	{
		return FeedID::MILLENIUM;	
	}
	if(feedHandlerType == "omxnordic")
	{
		return FeedID::OMXNORDIC;	
	}
	else
	{
			///@TODO: raise an exception	
	}
}

const char* FeedHandler::getFeedType(FeedID id)
{
	switch(id)
	{
		case FeedID::MILLENIUM:
			return "Millenium";

		case FeedID::OMXNORDIC:
			return "OMXNordic";

		default:
			///@TODO: raise an exception	
			{
			}
	}
}

/*
 * Gives back the pointer to the productInfo of the symbol or symbolID
 */
ProductInfo* FeedHandler::getProductInfo(Line *line, uint32_t msgType, const char* symbol, uint64_t symbolID, const char *isin, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds)
{
	// Lock for _prodInfoMap, _symbolSubMap
	WLock wlock(_subMapMutex);
	return _getProductInfo(line, msgType, symbol, symbolID, isin, seqNo, exSeconds, exUSeconds);
}

ProductInfo* FeedHandler::_getProductInfo(Line *line, uint32_t msgType, const char* symbol, uint64_t symbolID, const char *isin, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds)
{
	ProductInfo *product = _prodInfoMap.find(symbol);
	if(product)
	{
		return product;
	}
	else
	{
		Subscription *sub = getValidIdIsinSymbolSub(symbolID, symbol, isin);
		product = new ProductInfo(line, sub, msgType, seqNo, exSeconds, exUSeconds, symbol);
		sub->_prodInfo = product;
		product->_sub = sub;
		
		// insert the _prodInfo in both the maps	
		_prodInfoMap.insert(symbol, product);
		if(symbolID)
		{
			_prodInfoMapID.insert(make_pair(symbolID, product));
		}
		return product;
	}
}

/*
 * Gives back the pointer to the productInfo of the symbol or symbolID
 * @precond: should not be called from function which takes WLock to _subMapMutex
 *			 as this function takes Wlock
 	@Algo:
		See if product info exists
			return productInfo
		else
			Get symbol subscription if exists
			Get isin subscription if exists

			if symbolSub && it is active
				replace _isinSub with symbol sub
			else if isinSub && it is active
				replace _symbolSub with isinSub
			else  if symbolSub
				
 */
ProductInfo* FeedHandler::getProductInfo(Line *line, uint32_t msgType, uint64_t symbolID, const char* symbol, const char *isin, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds)
{
	// Lock for _prodInfoMap, _symbolSubMap
	WLock wlock(_subMapMutex);

	return _getProductInfo(line, msgType, symbolID, symbol, isin, seqNo, exSeconds, exUSeconds);
}

ProductInfo* FeedHandler::_getProductInfo(Line *line, uint32_t msgType, uint64_t symbolID, const char* symbol, const char *isin, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds)
{
	auto itr = _prodInfoMapID.find(symbolID);
	if(itr != _prodInfoMapID.end())
	{
		return itr->second;
	}
	else
	{
		Subscription *sub = getValidIdIsinSymbolSub(symbolID, symbol, isin);

		ProductInfo *product = new ProductInfo(line, sub, msgType, seqNo, exSeconds, exUSeconds, symbolID, symbol , isin);

		// sub->_productInfo and productInfo->_sub settting
		sub->_prodInfo = product;
		product->_sub = sub;
		
		// insert the _prodInfo in both the maps	
		_prodInfoMapID.insert(make_pair(symbolID, product));
		if(strcmp(symbol, ""))
		{
			_prodInfoMap.insert(symbol, product);
		}

		if(strcmp(isin, ""))
		{
			_prodInfoMapISIN.insert(isin, product);
		}
		return product;
	}
}

/*
 * Check all 3 synscription maps symbolId, symbol, isin for the 
 * subscription. Priority is given to symbolId, then symbol and then Isin.
 *	
 *	@precond:  This requires Wlock to _subMapMutex. So should be called only
 *				from function getProductInfo(which takes has it)
 *
 * @Algo :
 *          1> remove the subscriptions belonging from all three maps
 *  		2> If there are different subscription in every map for Product-Symbol-ISIN
 * which represent same instrument, Keep only one subscription for all three
 *				Priority is to SymbolID sub, then symbol sub and then isin sub
 *			3> If we choose idSub as primary sub and it is not active
 *				but corrosponding _symbolSub or _isinSub are then replace
 *				make _productSub active. Again _symbolSub attributes override _isinSub
 *			4> Reinsert the primary sub in all 3 maps
 *			
 */ 
Subscription* FeedHandler::getValidIdIsinSymbolSub(uint64_t symbolID, const char* symbol, const char *isin)
{
		Subscription *sub = nullptr;
		Subscription *symbolSub = nullptr;
		Subscription *isinSub = nullptr;
		Subscription *productSub = nullptr;
		if(strcmp(symbol, ""))
		{
			symbolSub = _symbolSubMap.find(symbol);
			//remove the subscription for now from map 
			// and later insert the modified sub
			_symbolSubMap.removeEntry(symbol);
		}

		if(strcmp(isin, ""))
		{
			isinSub = _isinSubMap.find(isin);
			//remove the subscription for now from map 
			// and later insert the modified sub
			_isinSubMap.removeEntry(isin);
		}

		auto itr = _idSubMap.find(symbolID);
		if(itr != _idSubMap.end())
		{
			productSub = itr->second;
			//remove the subscription for now from map 
			// and later insert the modified sub
			_idSubMap.erase(itr);
		}

		if(productSub) // we just have raw symbolSub, which is not activated 
		{
			sub = productSub; 
		}
		if(symbolSub && symbolSub->_isActive)
		{
			sub = symbolSub;
		}
		else if(isinSub && isinSub->_isActive)
		{
			sub = isinSub;
		}
		else if(symbolSub) // we just have raw symbolSub, which is not activated 
		{
			sub = symbolSub; 
		}
		else if(isinSub) // we just have raw isinSub, which is not activated 
		{
			sub = isinSub; 
		}
		else // none of above is true, create a raw subscription if needed
		{
			sub = new Subscription(symbolID, false);
			for(auto itr : _regexMap)
			{
				if(std::regex_match(std::to_string(symbolID), itr))
				{
					sub->_isActive.store(true, std::memory_order_release);
				}
			}
		}

		//this must be productSub to be inactive 
		if(!sub->_isActive)
		{
			if(isinSub && isinSub->_isActive)
			{
				sub->_isActive.store(true, std::memory_order_release);
			}

			if(symbolSub && symbolSub->_isActive)
			{
				sub->_isActive.store(true, std::memory_order_release);
			}
		}

		// Unifying symbols, replace if needed
		sub->_symbolID = symbolID;
		_idSubMap.insert(make_pair(symbolID, sub));	
		if(productSub && productSub != sub)
		{
			// raise the subscription replace;
		}

		if(strcmp(symbol, ""))
		{
			strncpy(sub->_symbol, symbol, sizeof(sub->_symbol));
			_symbolSubMap.insert(symbol, sub);	
			if(symbolSub && symbolSub != sub)
			{
				// raise the subscription replace;
			}
		}

		if(strcmp(isin, ""))
		{
			strncpy(sub->_isin, isin, sizeof(sub->_isin));
			_isinSubMap.insert(isin, sub);	
			if(isinSub && isinSub != sub)
			{
				// raise the subscription replace;
			}
		}

		return sub;
}
