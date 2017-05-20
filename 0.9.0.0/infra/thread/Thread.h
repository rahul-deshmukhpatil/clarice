#ifndef __THREAD_H__
#define __THREAD_H__

#include <stdio.h>
#include <stdarg.h>
#include <thread>
#include <map>

#include "infra/pugixml/pugixml.h"
#include "infra/InfraCommon.h"
#include "infra/lock/Lock.h"
#include "base/BaseCommon.h"

namespace infra 
{
	enum BindThreads 
	{
		AUTOMATICALLY,
		MANUALLY,
		FALSE
	};

	enum ThreadState
	{
		THREAD_INITIALIZED,	// Thread is initialized 
		THREAD_STARTED,		// Thread is active/running
		THREAD_SINGLED_STOP,// Signled thread to stop
		THREAD_STOPPED		// Thread is stopped
	};

	class Thread;
	typedef void* (*start_routine)(void *);
	typedef std::map<void *, Thread *> ObjToThreadMap;
	
	class Thread
	{
		public:
			Thread(const base::MarketDataApplication *app, const pugi::xml_node configNode, const char *threadName, void *obj, void* (*funPtrStart)(void *)); // Constructor
			~Thread();	 		// Destructor, stops thread first
			void start(); 		// Starts the thread 
			void startWrapper();// Start the thread;			   
			void signalToStop();// Signals the thread to stop
			void stop();		// Stops the thread 
			uint32_t getID() const;		// Get threadID in constant Time
			ThreadState state() const;	// Current thread state 
			const char* stateName() const;	// Current thread state string 
			bool isActive() const;
			bool isSingledToStop() const;
			bool isStopped() const;

			long int getThreadID() const;	// Returns the threadID in constant minimal time
			LoggerClientHandle *loggerClient() const; // returns the logger handle

			static ObjToThreadMap _objToThreadMap;
			const char  _name[NAME_SIZE];	// Name of thread

		private:
			Thread		*_thread;		// self pointer for logginf into file
			LoggerClientHandle *_logger; // logger client queue to which thread will write logs	
			uint32_t 	_threadID;		// Threads Id saved 
			ThreadState _threadState;	// current state of thread
			std::thread *_stdThread;	// std::thread handle
			void 		*_obj;			// obj of class to which thread belongs
			start_routine _startRoutine;	// thread entry of function
			int32_t		_cpu;			// cpu to which to bind thread  
			mutable std::mutex	_mutex;		// mutex for state change and cpu allocation
			const base::MarketDataApplication *_appInstance; // market data application instance to which thread belongs
	};
}
#endif //__THREAD_H__
