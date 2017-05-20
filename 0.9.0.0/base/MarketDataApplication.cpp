#include <dlfcn.h>
#include <atomic>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "infra/pugixml/pugixml.h"
#include "infra/logger/Logger.h"

#include "base/BaseCommon.h"
#include "base/MarketDataApplication.h"
#include "base/FeedHandler.h"
#include "base/NetworkReader.h"
#include "base/Recorder.h"

using namespace base;
using namespace infra;

FeedHandlerConstructor* MarketDataApplication::feedHandlerConstructors = new FeedHandlerConstructor[FeedID::MAX_FEEDS];

bool MarketDataApplication::registerFeedConstructors(FeedID feedID, FeedHandlerConstructor feedHandlerConstructor)
{
	feedHandlerConstructors[static_cast<int>(feedID)] = feedHandlerConstructor;
}

void MarketDataApplication::refreshTime()
{
	clock_gettime(CLOCK_REALTIME, &_globalClock);
	_time.store( *(reinterpret_cast<__int128 *>(&_globalClock)), memory_order_release);
}

/**
 *	\brief: MarketDataApplication, 
 *			constructor
 *
 *	This is a hard coded starting routine for every market data application.
 *	Later only market data subscription will happen. There is no restriction on
 *	instances of market data created
 *	@input	: 
 				@configFile: Config file of the application
 *	@output	: Market data instance will be created, which inturn creates
 *				all the resources. All app->start() to run the app after this;
 *  @TODO: Think about returning the shared pointer
 *
 *	@algo:
 *		Get time of the day
 *		Parse the config file
 * 		if Config file is valid
 *				1> create and start the loger
 *				2> create the main thread
 *				3> create the NetworkReader threads.
 *				4> create the FeedHandlers
 *						create the LineGroup Threads
 *							create the lines
 *				
 *				
 *		else
 *			raise and exception
 */
MarketDataApplication::MarketDataApplication(std::string configFile, FeedAPI *feedAPI)
	: _appInstance(this)
	, _configFile(configFile)
	, _thread(nullptr)
	, _mutex()
	, _feedAPI(feedAPI)
	, _name()
	, _bindThreads(MANUALLY)
	, _lastcpu(1)
	, _recorder(nullptr)
{
	refreshTime();
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(_configFile.c_str());
	if(result)
	{
		pugi::xml_node configNode = doc.child("config");
		pugi::xml_node ApplicationHandlerNode = configNode.child("ApplicationManager");
		if(ApplicationHandlerNode)
		{
			strncpy(const_cast<char *>(_name), ApplicationHandlerNode.getAttributeAsCString("name" , "", false, ""), sizeof(_name));

			_bindThreads = getThreadMode(ApplicationHandlerNode.getAttributeAsCString("bind-threads", "manually", false, "automatically,manually,false"));
			_lastcpu = ApplicationHandlerNode.getAttributeAsUInt("first-cpu", 1) - 1;

			pugi::xml_node LoggerNode = configNode.child("Logger");
			if(LoggerNode)
			{
				_loggerInstance = new Logger(_name, this, LoggerNode);
				///< We immeidatly start the logger thread, Do not wait to start the app
				_loggerInstance->thread()->start();
			}
			else
			{
				/// Raise an exception that logger is not present
				std::string error("No Logger Node found !!!");
				throw std::invalid_argument(error);
			}

			/// create the main thread
			_thread = new Thread(this, ApplicationHandlerNode, _name, this, MarketDataApplication::start);	
			
			///< create the NetworkReader threads
			createNetworkReaders(configNode);

			///< create the FeedHandlers 
			createFeedHandlers(configNode);
		}
		else
		{
			std::string error("No Application Manager Node found !!!");
			throw std::invalid_argument(error);
		}
	}
	else
	{
		char errorBuff[512];

		///< Throw an error exception. Can not parse the file or file is not present
		snprintf(errorBuff, sizeof(errorBuff), "Could not parse the xml config file[%s]. Check if file exists or do xmllint on file to check if it is well formatted !!!", _configFile.c_str());
	
		std::string error = errorBuff;
		throw std::invalid_argument(error);
	}
}

/**
 * \brief: getRecorder
 *			get the Recorder thread
 */
Recorder* MarketDataApplication::getRecorder() const
{
	return _recorder;
}

/**
 *	\brief: createNetworkReaders 
 *			Creates difference threads whos job is just to read network packets
 *			And queueing the packets into the assigner threads queues
 */
void MarketDataApplication::createNetworkReaders(const pugi::xml_node &configNode)
{
	pugi::xml_node networkReaders = configNode.child("NetworkReaders");
	if(networkReaders)
	{
		for (pugi::xml_node_iterator it = networkReaders.begin(); it != networkReaders.end(); ++it)
		{
			pugi::xml_node readerNode = *it;
			std::string nameOfChildNode = readerNode.name();
			if(nameOfChildNode == "NetworkReader")
			{
				std::string readerName = readerNode.getAttributeAsString("name" , "", false, "");
				const NameToNetworkReaderMap::iterator itr = _networkReaderMap.find(readerName);
				if(itr == _networkReaderMap.end())
				{
					NetworkReader *networkReader = new NetworkReader(this, readerNode);
					_networkReaderMap.insert(std::make_pair(readerName, networkReader));
					logConsole(DEBUG, "Inserted network reader [ %s : %p ] into map", readerName.c_str(), networkReader);
				}
				else
				{
					///< Throw an error exception duplicate reader name 
					char errorBuff[512];
					
					snprintf(errorBuff, sizeof(errorBuff), "Duplicate NetworkReader Name found in config. NetworkReader is already present [ %s : %p ] into map", readerName.c_str(), itr->second);
					std::string error(errorBuff);

					throw std::invalid_argument(error);
				}
			}
			else
			{
				///< Unintended node specified in the config file, raise exception log 
				logConsole(WARN, "<NetworkReaders> config node has unexpected child node <%s>", nameOfChildNode.c_str());
			}
		}
	}
}

/**
 *	feedHandlerConstructor:	
 *			Calls the feed specific feedManger
 */
FeedHandler* MarketDataApplication::feedHandlerConstructor(const pugi::xml_node& feedHandlerNode, const std::string &feedHandlerName)
{
	std::string feedType = feedHandlerNode.getAttributeAsString("feed", "", false, "");
	FeedID	feedID = FeedHandler::getFeedID(feedType);
	const char *libName = "";
	switch(feedID)
	{
		case FeedID::MILLENIUM:
				libName="libcd-millenium.so";
				break;
			
		case FeedID::OMXNORDIC:
				libName="libcd-omxnordic.so";
				break;
			
		default:
			///Raise an exception:
			{
				///< Throw an error exception no network reader found 
				char errorBuff[512];

				snprintf(errorBuff, sizeof(errorBuff), "Can not create FeedHandler belonging to node <FeedHandler name=\"%s\"> as did not find library for FeedHandler [ %u : %s ]. Check \"feed\" argument", feedHandlerName.c_str(), feedID, feedType.c_str());

				std::string error(errorBuff);
				throw std::invalid_argument(error);
			}
	}

	///@TODO: Load the libarary
	void * handle = dlopen(libName, RTLD_LOCAL | RTLD_LAZY);
	if (!handle) 
	{
		char errorBuff[512];

		snprintf(errorBuff, sizeof(errorBuff), "Could not create FeedHandler belonging to node  <FeedHandler name=\"%s\"> as could not load library %s. Please check if libarary is compiled and included in the release: ", feedHandlerName.c_str(), libName);

		std::string error(errorBuff);
		error = error + dlerror();
		throw std::invalid_argument(error);
	}

	return feedHandlerConstructors[static_cast<int>(feedID)](_appInstance, feedHandlerNode, _feedAPI);
}

/**
 *	\brief: createFeedHandlers
 *			Creates all Feed managers. 
 */
void MarketDataApplication::createFeedHandlers(const pugi::xml_node &configNode)
{
	pugi::xml_node feedHandlers = configNode.child("FeedHandlers");
	bool createRecorder = false;
	if(feedHandlers)
	{
		int i = 0;
		for (pugi::xml_node_iterator it = feedHandlers.begin(); it != feedHandlers.end(); ++it)
		{
			pugi::xml_node feedHandlerNode = *it;
			std::string nameOfChildNode = feedHandlerNode.name();
			if(nameOfChildNode == "FeedHandler")
			{
				std::string feedHandlerName = feedHandlerNode.getAttributeAsString("name" , "", false, "");
				bool record = feedHandlerNode.getAttributeAsBool("record" , false);
				if(record)
				{
					_recorder = new Recorder(this, configNode);
				}

				const NameToFeedHandlerMap::iterator itr = _feedHandlerMap.find(feedHandlerName);
				if(itr == _feedHandlerMap.end())
				{
					FeedHandler *feedHandler = feedHandlerConstructor(feedHandlerNode, feedHandlerName);
					_feedHandlerMap.insert(std::make_pair(feedHandlerName, feedHandler));
					logConsole(DEBUG, "Inserted feedHandler [ %s : %p ] into map", feedHandlerName.c_str(), feedHandler);

					if(feedHandler->_recording)
					{
						createRecorder = true;
					}
				}
				else
				{
					///< Throw an error exception duplicate feedHandler name 
					char errorBuff[512];
					
					snprintf(errorBuff, sizeof(errorBuff), "Duplicate FeedHandler Name found in config. FeedHandler is already present [ %s : %p ] into map", feedHandlerName.c_str(), itr->second);
					std::string error(errorBuff);

					throw std::invalid_argument(error);
				}
			}
			else
			{
				///< Unintended node specified in the config file, raise exception log 
				logConsole(WARN, "<FeedHandlers> config node has unexpected child node <%s>", nameOfChildNode.c_str());
			}
		}
	}
}

/**
 *  \brief: getFeedHandler
 *						
 */
FeedHandler *MarketDataApplication::getFeedHandler(const char *feedHandlerName) const
{
	NameToFeedHandlerMap::const_iterator itr = _feedHandlerMap.find(feedHandlerName);
	if(itr != _feedHandlerMap.end())
	{
		return itr->second;
	}
	else
	{
		return nullptr;
	}
}

/**
 *  \brief: getNetworkReader
 *						
 */
NetworkReader *MarketDataApplication::getNetworkReader(const char *readerName, const pugi::xml_node &configNode) const
{
	NameToNetworkReaderMap::const_iterator itr = _networkReaderMap.find(readerName);
	if(itr != _networkReaderMap.end())
	{
		return itr->second;
	}
	else
	{
		if(strcmp(readerName, ""))
		{
			char errorBuff[512];

			///< Throw an error exception. Could not find the networkReader with valid name 
			snprintf(errorBuff, sizeof(errorBuff), "<%s name=\"%s\"> Could not find NetworkReader %s.", configNode.name(), configNode.getAttributeAsCString("name", ""), readerName);
			std::string error = errorBuff;
			throw std::invalid_argument(error);
		}
		return nullptr;
	}
}

/*
 * subscribe using symbol name
 */
int32_t MarketDataApplication::subscribe(const char *feedName, const char *symbol, Subscription **sub) const
{
	FeedHandler *fh = getFeedHandler(feedName);

	if(!fh)
	{
		return INVALID_FEEDNAME;
	}

	fh->subscribe(symbol, sub);
}

/*
 * subscribe using symbol id 
 */
int32_t MarketDataApplication::subscribe(const char *feedName, const uint64_t symbolID, Subscription **sub) const
{
	FeedHandler *fh = getFeedHandler(feedName);

	if(!fh)
	{
		return INVALID_FEEDNAME;
	}

	return fh->subscribe(symbolID, sub);
}

/**
 * Starts the actual OS thread instance
 */
void MarketDataApplication::start()
{
	_thread->start();
	return;
}

/**
 * Calls the main processing function
 */
void* MarketDataApplication::start(void *obj)
{
	static_cast<MarketDataApplication *>(obj)->MarketDataApplicationLoop();
	return nullptr;
}

/**
 * \brief: MarketDataApplicationLoop
 * 			First send start signal to all the components 	
 * 			And then run continuosly to check the App health. 
 *	@algo:	
 *			1> start all the feed managers, 
 *				ensures that consuming threads are ready to eat
 *			2> start all the network readers(producers)
 *			
 *			while(thread is active)
 *				Update Time
 *			 	keep checking health, and do the management work
 */
void  MarketDataApplication::MarketDataApplicationLoop()
{
	logMessage(INFO, "Starting the MarketDataApplication [ %s : %p ] thread [ %s : %p ] with MarketDataApplication: Will Startall FeedHandlers and NetworkReaders", _name, this, _thread->_name, _thread);

	if(_recorder)
	{
		_recorder->start();
	}

	startFeedHandlers();
	startNetworkReaders();

	uint64_t stopTime = 0;
	while(_thread->isActive())
	{
		refreshTime();
	}

	// stop has been called for this thread,
	// Call the recursive stop for all member resources(resources)
	stopComponents();

	// Print all the stats
	printStats();

	_thread->stop();
}

/**
 *	\brief: startFeedHandlers
 *		starts all the feed managers under this application
 */
void MarketDataApplication::startFeedHandlers() const
{
	logMessage(INFO, "MarketData Application [ %s : %p ] is starting FeedHandlers", _name, this);
	NameToFeedHandlerMap::const_iterator itr = _feedHandlerMap.begin();
	for( ; itr != _feedHandlerMap.end(); itr++)
	{
		itr->second->start(_thread);
	}
}

/**
 *	\brief: startNetworkReaders
 *		starts all the feed managers under this application
 */
void MarketDataApplication::startNetworkReaders() const
{
	logMessage(INFO, "MarketData Application [ %s : %p ] is starting NetworkReaders", _name, this);
	NameToNetworkReaderMap::const_iterator itr = _networkReaderMap.begin();
	for( ; itr != _networkReaderMap.end(); itr++)
	{
		itr->second->start();
	}
}

/**
 *	\brief: shouldStop 
 *			Check if atleast one network reader is reading from file.
 *			If all network readers have signled to stop application signal to stop via SIGINT 2.
 */
void MarketDataApplication::stopApp() const
{
	NameToNetworkReaderMap::const_iterator itr = _networkReaderMap.begin();
	for( ; itr != _networkReaderMap.end(); itr++)
	{
		if(!itr->second->_stopApp)
		{
			//network reader is active, do not stop app
			return;
		}
	}

	//All network readers are running empty, signal to stop the application in next 5 secs
	usleep(5 * 1000000);
	kill(getpid(), 15);
	return;
}

/**
 * Free the resources, stops the other threads 
 */
void MarketDataApplication::stopComponents()
{
	logMessage(INFO, "MarketData Application [ %s : %p ] is stopping", _name, this);
	//stop network readers
	logMessage(INFO, "MarketData Application [ %s : %p ] is stopping NetworkReaders", _name, this);
	NameToNetworkReaderMap::const_iterator nr = _networkReaderMap.begin();
	for( ; nr != _networkReaderMap.end(); nr++)
	{
		nr->second->stop();
	}

	//stop feed handlers
	logMessage(INFO, "MarketData Application [ %s : %p ] is stopping FeedHandlers", _name, this);

	NameToFeedHandlerMap::const_iterator feed = _feedHandlerMap.begin();
	for( ; feed != _feedHandlerMap.end(); feed++)
	{
		feed->second->stop(_thread);
	}

	if(_recorder)
	{
		_recorder->stop();
	}
}

/**
 * Print all the stats for resources 
 */
void MarketDataApplication::printStats()
{
	logMessage(INFO, "MarketData Application [ %s : %p ] is stats", _name, this);
	//Print network reader stats
	NameToNetworkReaderMap::const_iterator nr = _networkReaderMap.begin();
	for( ; nr != _networkReaderMap.end(); nr++)
	{
//		nr->second->printStats();
	}

	//print feed handlers stats
	NameToFeedHandlerMap::const_iterator feed = _feedHandlerMap.begin();
	for( ; feed != _feedHandlerMap.end(); feed++)
	{
		feed->second->printStats(_thread);
	}
}

/**
 * Get the logger server instance associated with the application 
 */
const Logger *MarketDataApplication::getLoggerInstance() const
{
	return _loggerInstance;
}

/**
 * Stops all the threads related with the application 
 */
void MarketDataApplication::stop()
{
	_thread->signalToStop();
	while(!_thread->isStopped());

	logMessage(INFO, "MarketDataApplication [ %s : %p ] is stopped", _name, this);
	return;
}

/**
 * Release all the resources(memory...) and threads(stop and delete)
 */
MarketDataApplication::~MarketDataApplication()
{
	//delete network readers
	NameToNetworkReaderMap::const_iterator netReader = _networkReaderMap.begin();
	for( ; netReader != _networkReaderMap.end(); netReader++)
	{
		NetworkReader *nr = netReader->second;
		delete nr;
	}
	_networkReaderMap.clear();

	//stop feed handlers
	NameToFeedHandlerMap::const_iterator feed = _feedHandlerMap.begin();
	for( ; feed != _feedHandlerMap.end(); feed++)
	{
		FeedHandler *fh = feed->second; 
		delete fh;
	}
	_feedHandlerMap.clear();

	if(_recorder)
	{
		delete _recorder;
	}
	
	delete _thread;

	_loggerInstance->stop();
	delete _loggerInstance;
}

/*
 * Get how to allocate cores to the threads
 * bind-threads : Default value manual.
 *				automatically:	Assign thread no automatically,
 *							Ensure that there are sufficient cores are available. Otherwise
 *							if all cores are allocated it will raise exception
 *				manually	 : consider core given with "cpu" config option on 
 *								respective config nodes to bind
 *				false		 : No thread binding and "cpu" config option on nodes is
 */
BindThreads MarketDataApplication::getThreadMode(const char *mode)
{
	if(!strcmp(mode, "automatically"))
	{
		return BindThreads::AUTOMATICALLY;
	}
	else if(!strcmp(mode, "manually"))
	{
		return BindThreads::MANUALLY;
	}
	else if(!strcmp(mode, "false"))
	{
		return BindThreads::FALSE;
	}

	ASSERT(false, mode << " : Thread bind mode is incorrect");
	return BindThreads::FALSE;
}
