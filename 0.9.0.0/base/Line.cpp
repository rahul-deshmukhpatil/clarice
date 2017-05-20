#include "infra/logger/Logger.h"
#include "infra/thread/Thread.h"

#include "base/MarketDataApplication.h"
#include "base/NetworkReader.h"
#include "base/Subscription.h"
#include "base/API.h"
#include "base/FeedHandler.h"
#include "base/LineGroup.h"
#include "base/Line.h"
#include "base/PriceLevel.h"
#include "base/Order.h"
//#include "base/Packet.h"

using namespace base;
using namespace infra;

const char *IP::IPString[24] = {
		"primary-mc-line",
		"secondary-mc-line",
		"dr-primary-mc-line",
		"dr-secondary-mc-line",

		"primary-tcp-line",
		"secondary-tcp-line",
		"dr-primary-tcp-line",
		"dr-secondary-tcp-line",

		"primary-mc-snap-line",
		"secondary-mc-snap-line",
		"dr-primary-mc-snap-line",
		"dr-secondary-mc-snap-line",

		"primary-tcp-snap-line",
		"secondary-tcp-snap-line",
		"dr-primary-tcp-snap-line",
		"dr-secondary-tcp-snap-line",

		"primary-mc-retrans-line",
		"secondary-mc-retrans-line",
		"dr-primary-mc-retrans-line",
		"dr-secondary-mc-retrans-line",

		"primary-tcp-retrans-line",
		"secondary-tcp-retrans-line",
		"dr-primary-tcp-retrans-line",
		"dr-secondary-tcp-retrans-line",
	};

const char *base::getChannelString(base::ChannelType type)
{
	switch(type)
	{
		case base::ChannelType::MAIN :
			return "MAIN";

		case base::ChannelType::SNAP:
			return "SNAP";

		case base::ChannelType::RETRANS:
			return "RETRANS";

		default:
			//@TODO: Assert that this does not happen
			return "UNKNOWN";
	}
}

Line::Line(const MarketDataApplication *app, FeedHandler *feedHandler, LineGroup *lineGroup, Thread *thread, const pugi::xml_node &lineNode, LineAPI *lineAPI, Start start, ProcessPacket processFunc, GetPackStats getPacketStats, GetPackStatsSnap getPacketStatsSnap, ReRequestMissedPackets reRequestMissedPackets)
	: _lineAPI(lineAPI)
	, _appInstance(app)
	, _name()
	, _feedHandler(feedHandler)
	, _lineGroup(lineGroup)
	, _thread(thread)
	, _priceLevelPool(nullptr)
	, _orderPool(nullptr)
	, _maintainOrders(feedHandler->_maintainOrders)
	, _sub(nullptr)
	, _lastSeqNo(0)
	, _headerSeqNo(0)
	, _endSeqNo(0)
	, _currentMsg(nullptr)
	, _snapshotEnabled(false)
	, _isSnapStarted(false)
	, _isSnapshotEnd(false)
	, _lineState(LineState::JUST_STARTED)
	, _contentType(ContentType::HB)
	, _xtSecs(0)
	, _xtNanoSecs(0)
	, parseFuncMap{{nullptr, false, 0, 0, NO_CALLBACK}}
	, _processPacket(processFunc)
	, _nativGetPacketStats(getPacketStats)
	, _nativGetPacketStatsSnap(getPacketStatsSnap)
	, _nativReRequestMissedPackets(reRequestMissedPackets)
	, _packet(nullptr)
	, _nativStart(start)
	, _orderMap(nullptr)
{
	strncpy(const_cast<char *>(_name), lineNode.getAttributeAsCString("name", "", false, ""), sizeof(_name));
	logConsole(DEBUG, "Creating the Line [ %s : %p ]", _name, this);
	createLineIPs(lineNode);
		
	uint64_t buckets = lineNode.getAttributeAsLlong("order-map-size", 1024*1024);
	double loadFactor = lineNode.getAttributeAsDouble("order-map-load-factor", 10.0);
	_orderMap =  new std::unordered_map<uint64_t, Order *>(); 

	//change bucket size only after changing max load factor
	_orderMap->max_load_factor(loadFactor);
	_orderMap->rehash(buckets);

	// allocate PriceLevel pool with next size chunk and unlimited max size
	uint32_t size = lineNode.getAttributeAsUInt("pricelevel-pool-chunk-size", 1024*1024);
	uint32_t setNextSize = lineNode.getAttributeAsUInt("pricelevel-pool-set-next-size", 1024 *1024);
	_priceLevelPool =  new PriceLevelPool(size); 
	_priceLevelPool->set_next_size(setNextSize);

	// allocate order pool with next size chunk  and unlimited max size
	size = lineNode.getAttributeAsUInt("order-pool-chunk-size", 1024*1024);
	setNextSize = lineNode.getAttributeAsUInt("set-next-size", 1024 *1024);
	_orderPool =  new OrderPool(size); 
	_orderPool->set_next_size(setNextSize);
}

void Line::registerCallbacks(CallBacks callbacks)
{
	_callbacks = callbacks;
}

bool Line::hasInterest(CallBacks callback) const
{
	return _callbacks & callback;
}

bool Line::hasStrictInterest(CallBacks callback) const
{
	return (_callbacks & callback) == callback;
}

void Line::createLineIPs(const pugi::xml_node &lineNode)
{
	for(uint32_t i = 0; i < static_cast<uint32_t>(IPType::IPTYPE_MAX); i++)
	{
		_IPs[i] = PacketAddress(IP::getParsedIPPort(_appInstance, lineNode, IP::IPString[i]), static_cast<IPType>(i), static_cast<ChannelType>(i/8), this, _lineGroup); 

		IP zeroIP;
		if(zeroIP != _IPs[i])
		{
			_lineGroup->networkReader()->addIPToNetworkReader(&_IPs[i]);
		}
	}
}

void Line::reset()
{
	_lastSeqNo = 0; 
	_headerSeqNo = 0;
	_endSeqNo = 0;
	_currentMsg = nullptr;
	_isSnapStarted = false;
	_isSnapshotEnd = false;
	_snapshotSyncNo = 0;
	_lineState = LineState::JUST_STARTED;
	_contentType = ContentType::HB;
}

void Line::start()
{
	// initialize the base Line
	//initialize the Native feed specefic line
	(this->*_nativStart)();
	(_lineAPI->*(_lineAPI->_onLineStarted))(this);
}

/**
 * insert the order from ordermap maintained at the line group level
 */
void Line::insert(uint64_t orderID, Order *order)
{
#ifdef __ORDERMAP_LATENCY__
	timespec mt = globalClock();
	_orderMap->insert(make_pair(orderID, order));
	getLatency(_lineGroup->_om, mt);
#else
	_orderMap->insert(make_pair(orderID, order));
#endif

}
/**
 * returns the order from ordermap maintained at the line group level
 */
Order* Line::find(uint64_t orderID) const
{
	std::unordered_map<uint64_t, Order *>::const_iterator itr = _orderMap->find(orderID);
	if(itr != _orderMap->end())
	{
		return itr->second;
	}
	else
	{
		logMessage(EXCEPTION, "For Line [ %s : %p ] could not find order [ %llu ] in the orderMap [ %p ] in the orderMap to modify order" , this->_name, this, orderID, _orderMap);
		return nullptr;
	}
}

/**
 * Erase the order from ordermap maintained at the line group level
 */
void Line::erase(uint64_t orderID) const
{
	std::unordered_map<uint64_t, Order *>::const_iterator itr = _orderMap->find(orderID);
	if(itr != _orderMap->end())
	{
		_orderMap->erase(itr);
		return;
	}
	else
	{
		logMessage(EXCEPTION, "For Line [ %s : %p ] could not find order [ %llu ] to erase in the orderMap [ %p ]" , this->_name, this, orderID, _orderMap);
		return;
	}
}

/**
 *	\brief: printStats 
 *		print stats of all the underlying lines
 */
void Line::printStats(const Thread *_thread) const
{
}

int32_t Line::process() 
{
	// Process the MAIN and RETRANS messages
	if(_packet->_packetAddress->_channelType != ChannelType::SNAP)
	{
		return processMainLinePacket();
	}
	else // Process the SNAP Message
	{
		return processSnapshotPacket();
	}
}

int32_t Line::processMainLinePacket()
{
	// if feed is already snapped
	if(_lineState == LineState::NORMAL || _lineState == LineState::GAPPED) 
	{
		//_nativGetPacketStats(_contentType, _headerSeqNo, _endSeqNo, _currentMsg);
		(this->*_nativGetPacketStats)();

		if(_contentType == ContentType::HB)
		{
			// We have been gapped as HB seq is expected seq no to arrive 
			// So we declare we are gapped and note the timestamp of gap
			if(_headerSeqNo > _lastSeqNo+1   &&   _lineState != LineState::GAPPED)
			{ 
				_lineState = LineState::GAPPED; 	
				reRequestMissedPacket();
			}
			return PROCESSED;
		}
		else if(_contentType == ContentType::DATA_PACKET)
		{

			// This packet has next useful message
			if(_headerSeqNo <= _lastSeqNo+1   &&   _endSeqNo >= _lastSeqNo+1)
			{
				// We have some useful messages to be processed
				processPacket();

				if(_lineState == LineState::GAPPED)
				{
					processGapCache();
				}
				return PROCESSED;
			}
			else if(_endSeqNo <= _lastSeqNo)
			{
				// We have already processed all messages in this packet
				return PROCESSED;
			}
			else
			{
				// We are facing fresh gap, add packet to gap cache in sorted way
				_lineState = LineState::GAPPED; 	
				int32_t retval = addToGapCache();
				reRequestMissedPacket();
				return retval;
			}
		}
		
		// Not HB/DATA packet not of any use
		return PROCESSED;
	}
	else if(_lineState == LineState::SNAPPING)
	{
		if(_contentType != ContentType::HB)
		{
			return addToSnapCache();
		}

		// return HB as processed
		return PROCESSED;
	}
	else // We are yet to decide where to snap or not
	{
		// We yet to complete snap. Cache the main/retranse line packets
		//_nativGetPacketStats(_contentType, _headerSeqNo, _endSeqNo, _currentMsg);
		(this->*_nativGetPacketStats)();

		// this is the first packet with seq no 1
		// or intial HBs with seq no 1 in the ideal period during early hours
		if(_snapshotEnabled)
		{
			if(_headerSeqNo == 1)
			{
				_lastSeqNo = 0;
				_lineState = LineState::NORMAL;

				// We have some useful messages to be processed
				if(_contentType != ContentType::HB)
				{
					processPacket();
				}
				return PROCESSED; 
			}
		
			// We request snapshot and add current packet to the
			// snapCache and processs after snapshot completion
			_lineState = LineState::SNAPPING;
			reRequestSnap();
			if(_contentType == ContentType::HB)
			{
				return PROCESSED;
			}
			else
			{
				return addToSnapCache();
			}
		}
		else
		{
			//snapshot is not enabled
			_lastSeqNo = _headerSeqNo -1;
			_lineState = LineState::NORMAL;

			// We have some useful messages to be processed
			if(_contentType != ContentType::HB)
			{
				processPacket();
			}
			return PROCESSED; 
		}
	}
}

int32_t Line::processSnapshotPacket()
{
	if(_lineState != LineState::SNAPPING)
	{
		return PROCESSED;
	}
	else
	{
		(this->*_nativGetPacketStatsSnap)();
		if(_contentType == ContentType::HB)
		{
			// We have been gapped as HB seq is greater than expected sequence no to arrive 
			// So we cannot declare that we are gapped, 
			// That is why we will immeditely re-request the snapshot 
			// Though we might get missing packet immediatly,
			if(_headerSeqNo > _lastSeqNo+1)
			{
				reRequestSnap();
			}

			return PROCESSED;
		}
		else if(_contentType == ContentType::DATA_PACKET)
		{
			// We have some useful messages to be processed

			// We have recieved the start of snapshot
			if(_isSnapStarted)
			{
				// Clear any snapshot channel gap cache packets
				// And set _lastSeqNo so that _headerSeqNo is the expected one
				// As well clear packets having _endSeqNo <= _snapshotSyncNo
				clearGapCache();
				_lastSeqNo = _headerSeqNo - 1;

				//@TODO: Do we really want to clear non-imp packets
				// as of now?
				//clearSnapCache(_snapshotSyncNo);
			}

			if(_headerSeqNo <= _lastSeqNo+1   &&   _endSeqNo >= _lastSeqNo+1)
			{
				//@TODO: assert that _isSnapStarted is true
				//This is a valid packet 
				processPacket();

				if(_isSnapshotEnd)
				{
					//@TODO: assert that  _lastSeqNo == _snapshotSyncNo 
					// clear snapshot packets from gap cache
					// And then process the packets after _snapshotSyncNo
					_lineState = LineState::NORMAL;

					//clear the packets from snapshot line
					clearGapCache();
					
					//process the packets from the real time mc line
					processSnapCache();
				}
			}
			else if(_endSeqNo <= _lastSeqNo)
			{
				// We have processed this packet
				return PROCESSED;
			}
			else
			{
				// we were snapping and then gapped
				if(_isSnapStarted)
				{
					// We are facing gap, add packet to gap cache and re-request the snapshot
					reRequestSnap();
					return addToGapCache();
				}
			}
		}
	}
}

void Line::processPacket()
{
	(_lineAPI->*(_lineAPI->_onPacketStart))(this);
	// calling actual feed specefic process
	(this->*_processPacket)();
	(_lineAPI->*(_lineAPI->_onPacketEnd))(this);
}

void Line::reRequestMissedPacket()
{
	// This function is called from 3 points.
	// Detected the gap because of the HB
	uint64_t requestTill = _headerSeqNo;
	
	if(!_gapCache.empty())
	{
		if(_gapCache.begin()->first._headerSeqNo < requestTill)
		{
			//request the minimal required packets
			requestTill = _gapCache.begin()->first._headerSeqNo;
		}
	}

	uint64_t requestFrom = _lastSeqNo;

	if(_lastSeqNo < _gapRequestedTill)
	{
		//Till this sequence we have already requested gap
		requestFrom = _gapRequestedTill;	
	}

	//Re-Requst all required packets
	if(requestFrom+1 <= requestTill -1)
	{
		logMessage(INFO, "Line [%s : %p] Unordered Data packet detected Gap. _lastSeqNo = %lu, _gapRequestedTill = %lu, next available _headerSeqNo = %lu. Requesting packets %lu-%lu", _name, this, _lastSeqNo, _gapRequestedTill, requestTill, requestFrom+1, requestTill-1);
		(this->*_nativReRequestMissedPackets)(requestFrom+1, requestTill-1);
		_gapRequestedTill = requestTill - 1;
	}
}

int32_t Line::addToGapCache()
{
	return addToCache(_gapCache);
}

// returns true if we are successfully recoverd from gap
void Line::processGapCache()
{
	static bool processingGapCache = false;
	if(!processingGapCache) // prevents recursive call to this function
	{
		processingGapCache = true;
		// process pakets after gap is filled 
		while(!_gapCache.empty())
		{
			std::map<SeqRange, Packet *>::iterator itr = _gapCache.begin();
			
			_packet = itr->second;
			
			if(itr->first._endSeqNo <= _lastSeqNo)
			{
				logMessage(DEBUG, "Line :[ %s: %p ], Ignoring the packet from cache as  _endSeqNo %lu <= _lastSeqNo %lu", _name, this, itr->first._endSeqNo, _lastSeqNo);
				itr->second->popDeleter();
				_gapCache.erase(itr);
			}
			else if(itr->first._headerSeqNo <= _lastSeqNo+1  &&  itr->first._endSeqNo >= _lastSeqNo+1)
			{
				logMessage(DEBUG, "Line :[ %s: %p ], Processing packet from cache as  _headerSeqNo = %lu  _endSeqNo %lu and _lastSeqNo %lu", _name, this, itr->first._headerSeqNo, itr->first._endSeqNo, _lastSeqNo);
				
				(this->*_nativGetPacketStats)();
				// We have some useful messages to be processed
				processPacket();

				itr->second->popDeleter();
				_gapCache.erase(itr);
			}
			else
			{
				break;
			}
		}

		if(_gapCache.empty())
		{
			_lineState = LineState::NORMAL;

			// we have been fully recovered from gap
			ASSERT(_endSeqNo == _lastSeqNo, "ProcessGapCache, fully recovered but _lastSeqNo != _endSeqNo");
			logMessage(INFO, "processGapCache : Line :[ %s: %p ], Fully recovered from Gap. as _endSeqNo %lu == _lastSeqNo %lu", _name, this, _endSeqNo, _lastSeqNo);
		}
		else
		{
			// we have not been recovered, might want to re-request the missing packet
			// _headerSeqNo is set appropriatly in the above loop
			// On some feeds if you request gap of 2500, they will send only 50.
			// Below call ensures that remaining 2450 are recalled again.
			reRequestMissedPacket();
		}
		processingGapCache = false;
	}
}

// After snapshot is complete process each packet 
// having _headerSeqNo > _snapshotSyncNo, from main line
void Line::processSnapCache()
{
	// process pakets after _snapshotSyncNo
	while(!_snapCache.empty())
	{
		std::map<SeqRange, Packet *>::iterator itr = _snapCache.begin();
		_packet = itr->second;
		int32_t retval = processMainLinePacket();
		if(!retval)
		{
			// ensure that _snapCache is not modified by above processMainLinePacket
			// function, otherwise this iterator will be invalidated
			itr->second->popDeleter();
}
		_snapCache.erase(itr);
	}
}

void Line::reRequestSnap()
{
	// start counting of the snap period from the when we recieve the snap start message
	logMessage(INFO, "Requesting snapshot for Line :[ %s: %p ] _lastSeqNo = %lu, _headerSeqNo = %lu", _name, this, _lastSeqNo, _headerSeqNo);
	_lineState = LineState::SNAPPING;
}

/**
 *	Add packet to the snapshot cache
 */
int32_t Line::addToSnapCache()
{
	return addToCache(_snapCache);
}

int32_t Line::addToCache(std::map<SeqRange, Packet *> &_cache)
{
	// Insert packet into map. If same packet is present, Process
	// The one with more no of the packets as line A and line B have different packaging
	SeqRange newRange(_headerSeqNo, _endSeqNo);
	auto itr = _cache.find(newRange);

	if(itr == _cache.end())
	{
		logMessage(INFO, "Line :[ %s: %p ], Adding packet to the cache _headerSeqNo = %lu  _endSeqNo %lu", _name, this, _headerSeqNo, _endSeqNo);
		_cache.insert(std::make_pair(newRange, _packet));
		return NOT_PROCESSED; 
	}
	else
	{
		SeqRange range = itr->first;

		//current packet has more useful message;
		if(newRange._endSeqNo > range._endSeqNo)
		{
			//free and erase older packet
			itr->second->popDeleter();
			_cache.erase(itr);

			logMessage(INFO, "Line :[ %s: %p ], Adding packet to the cache _headerSeqNo = %lu  _endSeqNo %lu", _name, this, _headerSeqNo, _endSeqNo);
			//write newer packet with greater _endSeqNo
			_cache.insert(std::make_pair(newRange, _packet));
			return NOT_PROCESSED; 
		}
		return PROCESSED; 
	}
}

// Erase every packet in the gap cache
void Line::clearGapCache()
{
	_gapCache.clear();
}

// Erase every packet in the Snap cache
void Line::clearSnapCache()
{
	_snapCache.clear();
}

/*
 *  Converts the _currentMsg + offset into the uint32_t id
 *	and then find the subscription in lineGroup sub map
 */
Subscription* Line::getIdSub32(uint16_t offset)
{
	uint64_t id = *reinterpret_cast<const uint32_t *>(_currentMsg + offset);
	return _lineGroup->getIdSub(id);
}

IP::IP()
{
	sin_family = AF_INET;
	sin_addr.s_addr = 0;
	sin_port = 0;
}

///< Check if IP string provided is valid one
IP IP::getParsedIPPort(const MarketDataApplication *_appInstance, const pugi::xml_node &node, const char *ipType, const std::string ipString)
{
	IP ip;
	std::size_t index;

	///< Check if string is in  IP:port format
	if((index= std::string(ipType).find("-mc-"))!= std::string::npos)
	{
		ip._type = SOCK_DGRAM;
	}
	else if((index= std::string(ipType).find("-tcp-"))!= std::string::npos)
	{
		ip._type = SOCK_STREAM;
	}
	else
	{
		///< Throw an error exception Error Invalid IP:PORT 
		char errorBuff[512];
					
		snprintf(errorBuff, sizeof(errorBuff), "<%s name=%s> %s attribute represents invalid argument name. Attribute name must contain \"-mc-\" or \"-tcp-\" to represent SOCK_DGRAM or SOCK_STREAM ", node.name(), node.getAttributeAsString("name", "").c_str(), ipType);
		std::string error(errorBuff);

		throw std::invalid_argument(error);
	}

	std::size_t found = ipString.find_first_of(':');

	///< Check if string is in  IP:port format
	if(found == std::string::npos)
	{
		///< Throw an error exception Error Invalid IP:PORT 
		char errorBuff[512];
					
		snprintf(errorBuff, sizeof(errorBuff), "<%s name=%s> %s attribute represents invalid IP:PORT[%s] provided on line. Please provide port", node.name(), node.getAttributeAsString("name", "").c_str(), ipType, ipString.c_str());
		std::string error(errorBuff);

		throw std::invalid_argument(error);
	}

	std::string rawIP = ipString.substr(0, found);
	///< Extract and check IP  is in  IPv4 format
	if (inet_aton(rawIP.c_str(), &ip.sin_addr) == 0)
	{
		///< Throw an error exception Error Invalid IP
		char errorBuff[512];
					
		snprintf(errorBuff, sizeof(errorBuff), "<%s name=%s> %s attribute represents invalid IP[%s] provided on line. Please provide correct IP", node.name(), node.getAttributeAsString("name", "").c_str(), ipType, rawIP.c_str());
		std::string error(errorBuff);
		throw std::invalid_argument(error);
	}	

	///< Extract and check PORT is unsigned numeric value between 0-65535
	std::string rawPORT = ipString.substr(found+1);
	try 
	{
		uint32_t port = std::stoul(rawPORT);
		if(port < 0 || port > 65535)
		{
			///< Throw an error exception Error Invalid IP
			char errorBuff[512];
						
			snprintf(errorBuff, sizeof(errorBuff), "<%s name=%s> %s attribute represents invalid PORT[%s] provided on line. Please provide correct PORT between 0-65535", node.name(), node.getAttributeAsString("name", "").c_str(), ipType, rawPORT.c_str());

			std::string error(errorBuff);
			throw std::invalid_argument(error);
		}
		ip.sin_port = htons(port);
	}
	catch(std::exception& e) 
	{
		///< Throw an error exception Error Invalid IP
		char errorBuff[512];
					
		snprintf(errorBuff, sizeof(errorBuff), "<%s name=%s> %s attribute represents invalid PORT[%s] provided on line. Please provide correct PORT between 0-65535", node.name(), node.getAttributeAsString("name", "").c_str(), ipType, rawPORT.c_str());

		std::string error(errorBuff);
		throw std::invalid_argument(error);
	}

	logConsole(TRACE, "For node [ %s : %s ] created %s=%s", node.name(), node.getAttributeAsString("name", "").c_str(), ipType, ipString.c_str());
	return ip;
}

IP IP::getParsedIPPort(const MarketDataApplication *_appInstance, const pugi::xml_node &lineNode, const char *ipType)
{
	return getParsedIPPort(_appInstance, lineNode, ipType, lineNode.getAttributeAsString(ipType, "0.0.0.0:0000"));
}

const std::string IP::toString() const
{
	char buffer[16];
	const char* result=inet_ntop(AF_INET , &sin_addr.s_addr, buffer, sizeof(buffer));
	
	char ipPort[21];
	snprintf(ipPort, sizeof(ipPort), "%s:%hu", buffer, ntohs(sin_port));	
	return std::string(ipPort); 
}

PacketAddress::PacketAddress()
	: IP()
	, _pLine(nullptr)
	, _lineGroup(nullptr)
	, _ipType(IPType::IPTYPE_MAX)
	, _channelType(ChannelType::CHANNELTYPE_MAX)
	, _pPacketQueue(nullptr)
{
}

PacketAddress::PacketAddress(const IP& ip, IPType ipType, ChannelType channelType, const Line *line, const LineGroup *lineGroup)
	: IP(ip)
	, _ipType(ipType)
	, _channelType(channelType)
	, _pLine(line)
	, _lineGroup(lineGroup)
	, _pPacketQueue(lineGroup->packetQueue())
{
}

BoostSPSCQueue<Packet> *PacketAddress::pPacketQueue() const
{
	return const_cast<BoostSPSCQueue<Packet> *>(_pPacketQueue);
}

Socket& PacketAddress::socket() const
{
	return const_cast<Socket&>(_socket);
}

const std::string PacketAddress::toString() const
{
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "line %s:%p , lineGroup %s:%p , channel %s, iptype %s, ip %s", _pLine->_name, _pLine, _lineGroup->_name, _lineGroup, getChannelString(_channelType), IPString[(uint32_t)_ipType], static_cast<const IP*>(this)->toString().c_str());	
	return buffer; 
}

bool IP::operator < (const IP &rhs) const
{
	if(sin_addr.s_addr < rhs.sin_addr.s_addr)
	{
		return true;
	}
	else if( sin_addr.s_addr == rhs.sin_addr.s_addr)
	{
		if(sin_port <= rhs.sin_port)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		return false;
	}
}

bool IP::operator == (const IP &rhs) const
{
	return  (sin_port == rhs.sin_port) &&  ( sin_addr.s_addr == rhs.sin_addr.s_addr);
}

bool IP::operator != (const IP &rhs) const
{
	return  (sin_port != rhs.sin_port) || ( sin_addr.s_addr != rhs.sin_addr.s_addr);
}
