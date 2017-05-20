#include "infra/logger/Logger.h"
#include "infra/thread/Thread.h"
#include "base/BaseCommon.h"
#include "base/MarketDataApplication.h"
#include "base/NetworkReader.h"
#include "base/FeedHandler.h"
#include "base/LineGroup.h"
#include "base/Line.h"
#include "base/Recorder.h"

using namespace infra;
using namespace base;

Recorder::Recorder(const MarketDataApplication *app, const pugi::xml_node &node)
	: _appInstance(app)
	, _name()
	, _thread(nullptr)
{
	strncpy(const_cast<char *>(_name), "recorder", sizeof(_name));
	_thread = new Thread(_appInstance, node, _name, this, Recorder::_start);	
}

/*
 * create a recorder queue for each network recorder 
 */
BoostSPSCQueue<Packet>* Recorder::registerNetworkReader(const NetworkReader *nr)
{
	for(auto pair : _recordQueues)
	{
		// duplicate registration for same network reader
		if(pair.first == nr)
		{
			return pair.second;
		}
	}

	BoostSPSCQueue<Packet> *queue = new BoostSPSCQueue<Packet>(); 
	logConsole(DEBUG, "Registering NetworkReader [ %s : %p ] to Recorder [ %s : %p ] with queue", nr->_name, nr, _name, this, queue);
	_recordQueues.push_back(std::make_pair(nr, queue));
	return queue;
}

Recorder::~Recorder()
{
	delete _thread;
}

/**
 * Starts the actual OS thread instance
 */
void Recorder::start()
{
	_thread->start();
	return;
}

/**
 * Stops actual OS thread instance
 */
void Recorder::stop()
{
	_thread->signalToStop();

	while(!_thread->isStopped());

	logConsole(INFO, "Recorder [ %s : %p ] is stopped", _name, this);
	return;
}

/**
 * Calls the processing function
 */
void* Recorder::_start(void *obj)
{
	static_cast<Recorder *>(obj)->recorderLoop();
	return nullptr;
}

/**
 * Recording loop 
 */
void* Recorder::recorderLoop()
{
	logMessage(INFO, "Recorder [ %s : %p ] started", _name, this);

	while(_thread->isActive())
	{
		std::vector<std::pair<const NetworkReader*, BoostSPSCQueue<Packet> *> >::const_iterator itr = _recordQueues.begin();
		for(; itr != _recordQueues.end(); itr++)
		{
			std::shared_ptr<Packet> record = itr->second->popPtr(); 
			if(record)
			{
				Packet *packet = record.get(); // This is packet to be recorded 
				FeedHandler *fh = packet->_packetAddress->_lineGroup->_feedHandler;
				logMessage(TRACE, "Got packet to write for FeedHandler [ %s : %p ]", fh->_name, fh);
			
				std:string header = CLARICE_HEADER;
				IP ip = *packet->_packetAddress;
				header = header + "," + std::to_string(packet->_rt.tv_sec) + "." + std::to_string(packet->_rt.tv_nsec) + "," + ip.toString() + "," + std::to_string(packet->packetDataLen()) + ",";
				Recording *recording = fh->_recording;
				if(recording)
				{
					recording->_out->write(header.c_str(), header.size());
					recording->_out->write(packet->packetData(), packet->packetDataLen());
					recording->_out->write("\n", 1);
				}
			}
		}
	}

	logMessage(DEBUG, "Recorder [ %s : %p ] is stopping now", _name, this);
	_thread->stop();
}
