#include <pthread.h> 
#include <sched.h>
#include <thread>
#include <string.h>

#include "infra/logger/Logger.h"
#include "infra/thread/Thread.h"
#include "base/MarketDataApplication.h"


using namespace infra;

ObjToThreadMap Thread::_objToThreadMap;

/**
 *	Thread : constructor
 *			This class is wrapper on std::thread. 
 *			Creates dormat thread, Thread::start gets it active
 *			via startWrapper by calling _startRoutine.	
 *
 *	@input:	
 *			@app			: Main app instance. Necessory for getting 
 *								loggerInstance related with it
 *			@configNode		: Config node entity the config file to which thread will represent
 *			@_name			: User given thread name as seen by OS 
 *			@_obj			: Buisness object related with the thread,
 *								ie Logger, AppThread, Assigner, Processor/worker thread
 *			@_startRoutine	: Buisness object start/loop function
 *
 *	@Algo:
 *			See if thread related with same buisness object is already created	
 *				if yes, this is an exceptional situation
 *				else insert the thread into map<obj, Thread>
 */
Thread::Thread(const base::MarketDataApplication *app, const pugi::xml_node configNode, const char *threadName, void *obj, void* (*funPtrStart)(void *))
		: _name()
		,  _thread(this)
		, _logger(nullptr)
		, _stdThread(nullptr)
		, _threadID(0)
		, _threadState(ThreadState::THREAD_INITIALIZED)
		, _obj(obj)
		, _cpu(-1)
		, _startRoutine(funPtrStart)
		, _appInstance(app)
{
	// Lock for _objToThreadMap, simulteneous creation of threads
	// Though all the threads are tried to create in initial startup mode.
	// When applicaion is just main single thread
	WLock wlock(_mutex);
	strncpy(const_cast<char *>(_name), threadName, sizeof(_name));

	_cpu = configNode.getAttributeAsInt("cpu", -1);
	int32_t numCPU = sysconf( _SC_NPROCESSORS_ONLN );
	//CPU indexing starts from 0
	if(app->_bindThreads == BindThreads::MANUALLY && _cpu >= numCPU)
	{
		char errorBuff[512];

		// Throw an error exception. Can not allocate cpu specified
		snprintf(errorBuff, sizeof(errorBuff), "<%s name=\"%s\"> Can not bind thread %s to cpu core %d. Server has only %d cpu cores and requested to allocated cpu core %d !!!", configNode.name(), configNode.getAttributeAsCString("name", ""), _name, _cpu, numCPU, _cpu);
	
		std::string error = errorBuff;
		throw std::invalid_argument(error);
	}

	ObjToThreadMap::iterator itr = _objToThreadMap.find(obj);
	if(itr != _objToThreadMap.end())
	{
		char errorBuff[512];

		// Throw an error exception. Thread related to this node(Object) is already created 
		snprintf(errorBuff, sizeof(errorBuff), "Thread related with no <%s name=\"%s\"> is already created. Previous thread [%s : %p]", configNode.name(), configNode.getAttributeAsCString("name", ""), _name, itr->second);
	
		std::string error = errorBuff;
		throw std::invalid_argument(error);
	}

	_objToThreadMap.insert(std::pair<void *, Thread *>(obj, this));
	logConsole(INFO, "Creating new Thread [ %s : 0x%x ]", _name, this);	
}

/**
 * Create the native std::thread instance and start it
 * Calls the registered start function
 * Do not log anything in this function
 */
void Thread::start()
{
	// Lock to check if thread is not started by two parent threads.
	WLock wlock(_mutex);
	//Start the thread only if it is initialized
	if(_threadState == ThreadState::THREAD_INITIALIZED)
	{
		//@TODO: call thread start function
		_logger = const_cast<Logger *>(_appInstance->getLoggerInstance())->getClientHandleForLogger(this);
		_threadState =  ThreadState::THREAD_STARTED;

		//logConsole(INFO, "Starting Thread [ %s : 0x%x ]", _name, this);	

		//_stdThread = new std::thread(_startRoutine, _obj);
		_stdThread = new std::thread(&Thread::startWrapper, this);

		char name[30];
		pthread_setname_np(_stdThread->native_handle(), _name);
		pthread_getname_np(_stdThread->native_handle(), name, 30);
		//logConsole(INFO, "Started Thread [ %s=%s : 0x%x ]", name, _name, this);
	} 
	else
	{
		//@TODO: Log a warning on std::err
	}
}

/**
 * \brief: Calls the main activiy function of worker thread 
 * 			Sets the threadID and log the function 
 */
void Thread::startWrapper()
{
	uint32_t numCPU = sysconf( _SC_NPROCESSORS_ONLN );
	_threadID = syscall(SYS_gettid);
	if(_appInstance->_bindThreads == BindThreads::AUTOMATICALLY)
	{
		_cpu = ++_appInstance->_lastcpu; 
		if(_cpu >= numCPU)
		{
			char errorBuff[512];

			// Throw an error exception. Can not allocate cpu specified
			snprintf(errorBuff, sizeof(errorBuff), "Can not bind thread %s to automatically allocated cpu core %d. Server has only 0-%d cpu cores and requested to allocated cpu core %d !!!", _name, _cpu, numCPU - 1, _cpu);

			std::string error = errorBuff;
			throw std::invalid_argument(error);
		}
	}

	if(_cpu != -1 && (_appInstance->_bindThreads == BindThreads::AUTOMATICALLY || _appInstance->_bindThreads == BindThreads::MANUALLY))
	{
		size_t size = CPU_ALLOC_SIZE(numCPU);
		cpu_set_t *cpumask = CPU_ALLOC(numCPU);
		
		if(cpumask == NULL)
		{
			char errorBuff[512];

			// Throw an error exception. Can not allocate cpu specified
			snprintf(errorBuff, sizeof(errorBuff), "Could not allocate memory for CPU mask of bytes %lu while binding thread %s to cpu core %u", size, _name, _cpu);
		
			std::string error = errorBuff;
			throw std::invalid_argument(error);
		}

		CPU_ZERO_S(size, cpumask);
		CPU_SET_S(_cpu, size, cpumask);
		if( sched_setaffinity(_threadID, size, cpumask) == -1)
		{
			char errorBuff[512];

			// Throw an error exception. Can not allocate cpu specified
			snprintf(errorBuff, sizeof(errorBuff), "Could not bind thread %s to cpu core %u", _name, _cpu);
		
			std::string error = errorBuff;
			throw std::invalid_argument(error);
		}
	
		logMessage(INFO, "Started Thread [ %s[%u] : %p ] binded to cpu core %d", _name, _threadID, this, _cpu);
	}
	else
	{
		logMessage(INFO, "Started Thread [ %s[%u] : %p ] not binded to any core", _name, _threadID, this);
	}

	_startRoutine(_obj);
}

/**
 * Signals the start function of the thread, started above, 
 * to stop and free the resources held
 * Stop the thread only if it has been started
 *
 *	@input:
 *		logger of the parent thread stopping this thread
 *		If parent thread is not of type infra::Thread
 *		i.e thread created in the hierarchy of clarice market 
 *		data application
 */
void Thread::signalToStop()
{
	WLock wlock(_mutex);
	if(_threadState == ThreadState::THREAD_STARTED)
	{
		_threadState = ThreadState::THREAD_SINGLED_STOP;
		//This will get out the module out of loop
		//@TODO: join the thread and change the state to stopped
	}
	else
	{
		logMessage(EXCEPTION, "Thread [ %s : %p ] is currently in state %s and signaled to stop", _name, this, stateName());
		if(_threadState != ThreadState::THREAD_STOPPED)
		{
			_threadState = ThreadState::THREAD_SINGLED_STOP;
		}
	}
}

/*
 * Main loop of the thread is done, Now stop the thread
 */
void Thread::stop()
{
	WLock wlock(_mutex);
	if(_threadState == ThreadState::THREAD_SINGLED_STOP)
	{
		_threadState = ThreadState::THREAD_STOPPED;
		//This will get out the module out of loop
		//@TODO: join the thread and change the state to stopped
	}
	else
	{
		logMessage(EXCEPTION, "Thread [ %s : %p ] is currently in state %s and tried to stop", _name, this, stateName());
		_threadState = ThreadState::THREAD_STOPPED;
	}
}

/**
 *	returns the state.
 *	Taking lock for the synchronization, do not call this function unless needed
 */
ThreadState Thread::state() const
{
	RLock rlock(_mutex);
	return _threadState;
}

/**
 *	returns the state name string.
 *	@pre-cond: 	must be called from Thread member function
 *				which must have taken atleast rlock(_mutex)
 *
 */
const char* Thread::stateName() const
{
	switch(_threadState)
	{
		case ThreadState::THREAD_INITIALIZED :
			return "THREAD_INITIALIZED";

		case ThreadState::THREAD_STARTED:
			return "THREAD_STARTED";

		case ThreadState::THREAD_SINGLED_STOP:
			return "THREAD_SINGLED_STOP";
		
		case ThreadState::THREAD_STOPPED:
			return "THREAD_STOPPED";

		default:
			logMessage(EXCEPTION, "Tried to convert unknown state %d to string", _threadState);
			return "THREAD_STATE_UNKNOWN";
	}
}

/**
 *	returns if thread is active.
 */
bool Thread::isActive() const
{
	return _threadState == ThreadState::THREAD_STARTED;
}

/**
 *	returns if thread is singned to stop.
 */
bool Thread::isSingledToStop() const
{
	return _threadState == ThreadState::THREAD_SINGLED_STOP;
}

/**
 *	returns if thread is active.
 */
bool Thread::isStopped() const
{
	return _threadState == ThreadState::THREAD_STOPPED;
}

/**
 * Returns the os-specefic threadID
 */
long int Thread::getThreadID() const
{
	return _threadID;
}

/**
 * Returns the client handle of the thread
 */
LoggerClientHandle *Thread::loggerClient() const
{
	return _logger;
}

/**
 * Thread destructor:
 *		Stop the thread if it has not been stopped already	
 */
Thread::~Thread()
{
	if(state() != ThreadState::THREAD_STOPPED);
	{
		// @TODO: deleter is stopping thread, log warning
		// @ Should have been stopped by the owner first
		stop();
	}

	_stdThread = nullptr;
	_logger = nullptr;
}

