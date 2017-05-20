#include "pool/object_pool.hpp"

#include "infra/logger/Logger.h"
#include "base/MarketDataApplication.h"
#include "base/FeedHandler.h"
#include "base/LineGroup.h"
#include "base/NetworkReader.h"
#include "base/API.h"
#include "base/Subscription.h"
#include "base/Line.h"
#include "base/Packet.h"


using namespace base;
using namespace infra;

LineGroup::LineGroup(const MarketDataApplication *app, FeedHandler *feedHandler, const pugi::xml_node &lineGroupNode, LineConstructor lineConstructor, LineGroupAPI *lineGroupAPI)
	: _lineGroupAPI(lineGroupAPI)
	, _appInstance(app)
	, _feedHandler(feedHandler)
	, _thread(nullptr)
	, _packetQueue(new BoostSPSCQueue<Packet>())
	, _lineConstructor(lineConstructor)
	, _name()
	, _networkReader(app->getNetworkReader(lineGroupNode.getAttributeAsCString("network-reader", "", true, ""), lineGroupNode))
{
	strncpy(const_cast<char *>(_name), lineGroupNode.getAttributeAsCString("name", "", false, ""), sizeof(_name));
	_thread = new Thread(_appInstance, lineGroupNode, _name, this, LineGroup::_start);	

	createLines(lineGroupNode);
	
	_networkReader->registerLineGroup(this);
}

void LineGroup::registerCallbacks(CallBacks callbacks)
{
	NameToLineMap::iterator itr = _lineMap.begin();

	for( ; itr != _lineMap.end(); itr++)
	{
		itr->second->registerCallbacks(callbacks);		
	}
}

/*
 * \brief: networkReader
 *			returns the NetworkReader
 */
NetworkReader *LineGroup::networkReader() const
{
	return _networkReader;
}

/**
 *	\brief: createLines
 *			Creates all Lines under for LineGroup. 
 */
void LineGroup::createLines(const pugi::xml_node &lineGroupNode)
{
	pugi::xml_node lines = lineGroupNode.child("Lines");
	if(lines)
	{
		for (pugi::xml_node_iterator it = lines.begin(); it != lines.end(); ++it)
		{
			pugi::xml_node lineNode = *it;
			std::string nameOfChildNode = lineNode.name();
			if(nameOfChildNode == "Line")
			{
				std::string lineName = lineNode.getAttributeAsString("name", "", false, "");
			
				const NameToLineMap::iterator itr = _lineMap.find(lineName);
				if(itr == _lineMap.end())
				{
					Line *pLine = _lineConstructor(_appInstance, _feedHandler, this, _thread, lineNode, _lineGroupAPI);
					_lineMap.insert(std::make_pair(lineName, pLine));
					logConsole(DEBUG, "Inserted line [ %s : %p ] into map", lineName.c_str(), pLine);
				}
				else
				{
					///< Throw an error exception duplicate line name 
					char errorBuff[512];
					
					snprintf(errorBuff, sizeof(errorBuff), "Duplicate Line Name found in config. Line is already present %s : %p into map", lineName.c_str(), itr->second);
					std::string error(errorBuff);

					throw std::invalid_argument(error);
				}
			}
			else
			{
				///< Unintended node specified in the config file, raise exception log 
				logConsole(WARN, "<Lines> config node has unexpected child node <%s>", nameOfChildNode.c_str());
			}
		}
	}
}

/*
 *	Get the symbol subscription which must be active and has _prodInfo
 *	Otherwise return null
 */
Subscription* LineGroup::getIdSub(uint64_t symbolID)
{
	Subscription *sub = nullptr;
	auto itr = _idSubMap.find(symbolID);

	//Got the subscription and it is activ one
	if(itr != _idSubMap.end())
	{
		sub = itr->second;
	}
	else
	{
		sub = _feedHandler->getIdSub(symbolID);
		_idSubMap.insert(make_pair(symbolID, sub));	
	}
	
	ASSERT(sub->_prodInfo != nullptr, "sub->_prodInfo : " << sub->_prodInfo);

	if(sub->_isActive && sub->_prodInfo)
	{
		return sub;
	}
	else
	{
		return nullptr;
	}
}

/**
 * Starts the actual OS thread instance
 */
void LineGroup::start()
{
	_thread->start();
	return;
}

/**
 * Stops actual OS thread instance
 */
void LineGroup::stop()
{
	_thread->signalToStop();

	while(!_thread->isStopped());

	logConsole(INFO, "LineGroup [ %s : %p ] is stopped", _name, this);
	return;
}


void LineGroup::printLatency(const Thread *_thread, const char *latencyType, const uint64_t *arr) const
{
	uint64_t total = 0;
	for(int i = 0; i < LATENCY_SIZE ; i++)
	{
		total += arr[i];	
	}

	if(!total)
	{
		total = 1;
	}

	uint64_t cumSum = 0;
	uint64_t step = 0;
	for(int i = 0; i < LATENCY_SIZE ; i++)
	{
		cumSum += arr[i];
		uint64_t  perctentile = (cumSum * 100)/ total;
		if(perctentile >= step)
		{
			logMessage(INFO, "LineGroup [ %s : %p ]: %lu %s latency : %d] %lu [ %lu, %lu / %lu]", _name, this, total, latencyType, i, perctentile, arr[i], cumSum, total);
			if(step >= 90 && step < 99)
			{
				step += 1;
			}
			else
			{
				step += 10;
			}
		}
	}
}

/**
 *	\brief: printStats 
 *		print stats of all the underlying lines
 */
void LineGroup::printStats(const Thread *_thread) const
{
	logMessage(INFO, "LineGroup [ %s : %p ] stats", _name, this);
#if defined(__QWRITEREAD_LATENCY__)
	printLatency(_thread, "Queue Write Read", _ql);
#endif 

#if defined(__NEXTREAD_LATENCY__)
	printLatency(_thread, "Next Read", _readLatency);
#endif 

#if defined(__MSG_LATENCY__)
	printLatency(_thread, "Message Processing", _ml);
#endif 

#if defined(__PACKET_LATENCY__)
	printLatency(_thread, "Net Packet to Callback Raise", _rl);
#endif 

#if defined(__SUBMAP_LATENCY__)
	printLatency(_thread, "Subscription find Latency", _sf);
#endif 

#if defined(__ORDERMAP_LATENCY__)
	printLatency(_thread, "Order Map Latency", _om);
#endif 

#if defined(__ORDERPOOL_LATENCY__)
	printLatency(_thread, "Order Pool Latency", _op);
#endif 
}

/**
 * Calls the processing function
 */
void* LineGroup::_start(void *obj)
{
	static_cast<LineGroup *>(obj)->lineGroupLoop();
	return nullptr;
}

/**
 * \brief: lineGroupLoop
 * 		Reads all the queues belonging to IPs of this lines belonging to this LineGroup
 *
 */
void LineGroup::lineGroupLoop()
{
	logMessage(INFO, "Started LineGroup [ %s : %p ]", _name, this);
	for(auto itr : _lineMap)
	{
		itr.second->start();
	}

	(_lineGroupAPI->*(_lineGroupAPI->_onLineGroupStarted))(this);

	timespec lastReadTime = globalClock();
	while(_thread->isActive())
	{
		Packet *readPtr = _packetQueue->pop();
		timespec ct = globalClock();
		if(readPtr)
		{
			Line *pLine = const_cast<Line *>(readPtr->_packetAddress->_pLine);
			pLine->_packet = readPtr;

			{
#if defined(__QWRITEREAD_LATENCY__)
				timespec rt = readPtr->_rt;
				uint64_t timeDiff = ((ct.tv_sec - rt.tv_sec) * 1000000000) + ct.tv_nsec - rt.tv_nsec; 

				if(timeDiff >= LATENCY_SIZE) 
				{
					timeDiff = LATENCY_SIZE - 1;
				} 
				++_ql[timeDiff];
#endif

#if defined(__NEXTREAD_LATENCY__)
				timeDiff = ((ct.tv_sec - lastReadTime.tv_sec) * 1000000000) + ct.tv_nsec - lastReadTime.tv_nsec; 
				if(timeDiff >= LATENCY_SIZE) 
				{
					timeDiff = LATENCY_SIZE - 1;
				} 
				++_readLatency[timeDiff];
#endif
			}

			int32_t retVal = pLine->process();
			pLine->_packet = nullptr;
			lastReadTime = globalClock();
			if(!retVal)
			{
			readPtr->popDeleter();
		}
		}
		else
		{
			lastReadTime = globalClock();
		}
	}

	logMessage(DEBUG, "LineGroup [ %s : %p ] is stopping now", _name, this);
	_thread->stop();
}

