#ifndef __MARKETDATAPPLICATION_H__
#define __MARKETDATAPPLICATION_H__

#include <time.h>
#include <set>
#include <map>

#include "infra/InfraCommon.h"
#include "infra/thread/Thread.h"
#include "infra/lock/Lock.h"
#include "infra/logger/Logger.h"
#include "base/BaseCommon.h"

using namespace infra;

namespace base 
{

	enum Error
	{
		INVALID_FEEDNAME = -1,
		SUCESS = 0,
		REGEX_SUBSCRIPTION = 1
	};


	typedef FeedHandler* (*FeedHandlerConstructor)(MarketDataApplication *, const pugi::xml_node&, FeedAPI *feedAPI);
	/**
	 * \brief MarketDataApplication: 
	 *			This is main class, Initialize the resources properly
	 *			once created, create the subscritions and start the app
	 */ 
	class MarketDataApplication
	{
		public:
			static bool registerFeedConstructors(FeedID feedID, FeedHandlerConstructor feedHandlerConstructor);
			void refreshTime();
			static FeedHandlerConstructor *feedHandlerConstructors;

			MarketDataApplication(std::string, FeedAPI *);
			~MarketDataApplication();
			Recorder* getRecorder() const; //get recorder instance
			const Logger *getLoggerInstance() const; 		///< get the logger server instance
			void start(); 									///< Start the main thread associated with the application
			void stop(); 						///< stop Thread
			BindThreads getThreadMode(const char *); // return core allocation mode
			void stopComponents();		///< Stop the child threads, started in the start function
			void printStats();		// print the stats
			void stopApp() const;	// check to see if we could stop app in file mode

			void setLoggerInstance(Logger *logger)	///< set the logger instance in Logger constructor
			{
				_loggerInstance = logger;
			}

			FeedHandler	*getFeedHandler(const char *) const;
			NetworkReader *getNetworkReader(const char *, const pugi::xml_node &) const;

			int32_t subscribe(const char *, const char *, Subscription **) const;
			int32_t subscribe(const char *, const uint64_t , Subscription **) const;

			BindThreads	_bindThreads;
			mutable uint32_t       _lastcpu;
			const char	_name[NAME_SIZE];			///< Logical name of an application 

		private:
			static void* start(void *); 		///< calls startApplication
			void MarketDataApplicationLoop(); 	///< start function of main thread, which keeps eye on stats and health of app	

			void createNetworkReaders(const pugi::xml_node &);
			void createFeedHandlers(const pugi::xml_node &);
			FeedHandler* feedHandlerConstructor(const pugi::xml_node &, const std::string &);

			void startFeedHandlers() const;
			void startNetworkReaders() const;

			MarketDataApplication *_appInstance;	///< Pointer to self for logging on console
			Thread		*_thread; 			///< Main application thread
			Logger		*_loggerInstance;	///< Logger Instance
			std::string	_configFile;		///< config file path
			pugi::xml_parse_result _config;	///< config xml handle
			std::mutex	_mutex;	 			///< Mutex lock for whole class data members 
			struct timespec _globalClock;		///< Maintains the current Time
			std::atomic<__int128> _time;
			
			typedef std::map<std::string, NetworkReader *> NameToNetworkReaderMap;
			NameToNetworkReaderMap _networkReaderMap; ///< Name-NetworkReader map

			typedef std::map<std::string, FeedHandler *> NameToFeedHandlerMap;
			NameToFeedHandlerMap _feedHandlerMap; ///< Name-FeedHandler map

			FeedAPI *_feedAPI;
			Recorder *_recorder; //recorder instance
	};
}


#endif //__MARKETDATAPPLICATION_H__
