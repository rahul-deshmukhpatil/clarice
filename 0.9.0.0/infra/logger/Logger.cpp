#include <string.h>
#include <sys/syscall.h>
#include <time.h>

#include <algorithm>

#include "infra/logger/Logger.h"
#include "infra/thread/Thread.h"
#include "infra/lock/Lock.h"
#include "infra/pugixml/pugixml.h"

#include "base/MarketDataApplication.h" 

using namespace infra;
using namespace base;

Logger* Logger::_loggerInstance; 

/**
 *	getLoggerInstance: get single instance of logger.
 *
 *	@precondition	: Called when application started as single thread instance
 *	@input		 	: Take logger config node 
 *	@output			: Creates the logger if not present.
 *						If tried to call second time, log exception
 *	@return			: Returns pointer to singlton _loggerInstance
 *
 * 	@algo:
 *			If not have _loggerInstance
 * 				create _loggerInstance with opening log file to write;
 *				and start the _thread; 
 */
Logger* Logger::getLoggerInstance(const std::string& appName, const MarketDataApplication *appInstance, pugi::xml_document& doc)
{
	if(!_loggerInstance)
	{
		new Logger(appName, appInstance, doc );
		//@TODO _loggerInstance->_thread->start(Logger::loop);	
	}
	return _loggerInstance;
}

/**
 * Try to open the log file
 * 
 * if path does not exists,
 * 		create path.
 * 		if can not createPath, throw an exception.
 * create file
 * Log the customised log message that logger has been instantiated
 */
Logger::Logger(const std::string& appName, const MarketDataApplication *appInstance, pugi::xml_node LoggerNode)
				: _appInstance(appInstance)
				, _thread(nullptr)
				, _mutex()
				, _logFileHandle(nullptr)
				, _loggerClientHandlesVector()
				, _threadToClientHandlesMap()
				, _name()
				, _logFile("")
				, _maxFileSize(1024*1024*1024)
				, _logQueueSize(1024)
				, _level(LogLevel::SANTY)
{
	strncpy(const_cast<char *>(_name), LoggerNode.getAttributeAsCString("name" , "", false, ""), sizeof(_name));
	//TODO: intialize _name, maxFileSize, logQueueSize and level from the config
	_logFile		= appName + "-" + LoggerNode.getAttributeAsString("file", _name);
	_logQueueSize	= LoggerNode.getAttributeAsUInt("logger-queue-size", _logQueueSize);
	_level			= Logger::getLoggerLevelFromString(LoggerNode.getAttributeAsString("level", "SANTY"));
	_maxFileSize	= LoggerNode.getAttributeAsUInt("size", _maxFileSize); 
	_logFileHandle	= fopen(_logFile.c_str(), "w");

	const_cast<MarketDataApplication *>(_appInstance)->setLoggerInstance(this);

	_loggerInstance = this;
	if(_logFileHandle)
	{
		_thread	= new Thread(appInstance, LoggerNode, _name, this, &Logger::startLogger);
	}
	else
	{
		///<@TODO: Raise an exception. 
	}
}

Logger::~Logger()
{
	WLock wlock(_mutex);

	// delete the logger client handles
	std::vector<LoggerClientHandle *>::iterator itr = _loggerClientHandlesVector.begin();
	for(; itr != _loggerClientHandlesVector.end(); itr++)
	{
		delete *itr;
	}
	_loggerClientHandlesVector.clear();		

	//flush and close the file handler
	fflush(_logFileHandle);
	fclose(_logFileHandle);
}

/**
 *	\brief:	getentHandleForLogger(Thread *_thread, uint32_t _logQueueSize = 1024, LogLevel _level = INFOClientHandleForLogger, 
 *			get loggerClientHandle for the thread to write logs into queue
 *
 *	@input	: Thread*, which needs the handle
 *			_logQueueSize, size of logQueue
 *			_level, level of logging message 
 * 
 * 	@output : create the logger client handle if not present for the thread.
 *  @returns: LoggerClientHandle* representing the log queue of thread
 *	@TODO	: implement the function createOrGetLoggerHandle with locks
 *	@algo	: 
 *			 Check if client handle for thread is already present in _threadToClientHandlesMap
 *			 if present
 *				return it;
 *			 else
 *			 	create shared_ptr to new loggerClientHandle;
 *				add shared_ptr to map;
 *				add shared_ptr to vector;
 *				return shared_ptr;
 *
 */	
LoggerClientHandle *Logger::getClientHandleForLogger(Thread *thread)
{
	// lock for both _threadToClientHandlesMap and _loggerClientHandlesVector
	WLock wlock(_mutex);

	//@TODO: Take write lock
	std::map<Thread *, LoggerClientHandle *>::iterator itr = _threadToClientHandlesMap.find(thread);
	if(itr != _threadToClientHandlesMap.end())
	{
		//@TODO: This should never happened. Assert required
		return itr->second;
	}
	else
	{
		LoggerClientHandle *loggerClientHandle =  new LoggerClientHandle(_appInstance, thread, _logQueueSize, _level);	
		_threadToClientHandlesMap.insert(std::pair<Thread *, LoggerClientHandle *>(thread, loggerClientHandle));
		_loggerClientHandlesVector.push_back(loggerClientHandle);
		return loggerClientHandle;
	}
}

// @return : file handle to write in the file
const FILE *Logger::getLogFileHandle() const
{
	return _logFileHandle;
}

/**
 * \brief : thread
 *		returns the thread which runs in loop and writes log to the file 
 */
Thread *Logger::thread() const
{
	return _thread;
}

/**
 * getLoggerLevelFromString
 *		get the log level from the string
 *		default devel is SANTY
 */

LogLevel Logger::getLoggerLevelFromString(std::string level)
{
	if(level == "TRACE")
		return LogLevel::TRACE;
	else if(level == "DEBUG")
		return LogLevel::DEBUG;
	else if(level == "SANTY")
		return LogLevel::SANTY;
	else if(level == "INFO")
		return LogLevel::INFO;
	else if(level == "WARN")
		return LogLevel::WARN;
	else if(level == "ERROR")
		return LogLevel::ERROR;
	else if(level == "EXCEPTION")
		return LogLevel::EXCEPTION;
	else if(level == "CRITICAL")
		return LogLevel::CRITICAL;
	else
		return LogLevel::SANTY; ///< Default one, atleast return this
}

/**
 *  Returns the Log level string used by the logger macros.
 * The order of the log levels in switch case is scrambled to get 
 * the better hit at INFO logger level which will be used by most
 * production ready systems
 */
const char *Logger::getLogLevelString(LogLevel level)
{
	switch(level)
	{
		case LogLevel::INFO:
			return "INFO";

		case LogLevel::WARN:
			return "WARN";
		
		case LogLevel::DEBUG:
			return "DEBUG";

		case LogLevel::SANTY:
			return "SANTY";
		
		case LogLevel::ERROR:
			return "ERROR";
		
		case LogLevel::EXCEPTION:
			return "EXCEPTION";
		
		case LogLevel::CRITICAL:
			return "CRITICAL";
		
		case LogLevel::TRACE:
			return "TRACE";
		
		default:
			return "UNKNOWN";
	}
}

/**
 * \brief : printInFile
 *			
 */
void Logger::printInFile(LogLevel LEVEL, const char *FILENAME, uint32_t LINE, const char *func, const char *FORMAT, ...)
{
	if(LEVEL >= _level) 
	{
		LogMessage msg;
		msg._timeStamp.tv_sec = globalClock().tv_sec;
		msg._timeStamp.tv_nsec = globalClock().tv_nsec;
		msg._file = FILENAME;
		msg._function = func;
		msg._lineNo = LINE;
		msg._level = LEVEL;

		va_list ARGS;
		va_start(ARGS, FORMAT);
		vsnprintf(msg._message, LOG_MSG_SIZE, FORMAT, ARGS);

		writeLog(&msg, getpid(), syscall(SYS_gettid));

/*
		fprintf(stderr, "%d.%09ld [%d,%ld] %s %s %u %s ", tv_sec, tv_nsec, getpid(), syscall (SYS_gettid), Logger::getLogLevelString(LEVEL), FILENAME, LINE, func);

		va_list ARGS;
		char BUFFER[LOG_MSG_SIZE];
		va_start(ARGS, FORMAT);
		vsnprintf(BUFFER, LOG_MSG_SIZE, FORMAT, ARGS);
		va_end(ARGS);
		fprintf(stderr, " %s\n", BUFFER);
		*/
	}
}

/**
 * \brief : startLogger 
 *		Static wrapper function around LoggerLoop	
 */
void* Logger::startLogger(void *arg)
{
	static_cast<Logger *>(arg)->LoggerLoop();	
	return nullptr;
}

/**
 * Stops actual OS thread instance
 */
void Logger::stop()
{
	_thread->signalToStop();

	while(!_thread->isStopped());

	logConsole(INFO, "Logger [ %s : %p ] is stopped", _name, this);
	return;
}

/**
 *	\brief operator<(const timespec &lhs, const timval &rhs)
 *			Used by the logger thread to sort the logging queues
 *			to print the latest logs
 */
inline bool operator < (const timespec &lhs, const timespec &rhs)
{
	if(lhs.tv_sec == rhs.tv_sec)
	{
		if(lhs.tv_nsec < rhs.tv_nsec)
		{
			return true;
		} 
		else
		{
			return false;
		} 
	}
	else if(lhs.tv_sec < rhs.tv_sec)
	{
		return true;
	}
	else // lhs.tv_sec > rhs.tv_sec
	{
		return false;
	}
}

inline bool operator <= (const timespec &lhs, const timespec &rhs)
{
	if(lhs < rhs)
		return true;

	if(lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec == rhs.tv_nsec)
	{
		return true;
	} 
	return false;
}

/**
  * Change the algorithm to print the logs in the timely manner
  * Currently it prints first log from every LoggerClientHandle queue.
	  *  sort the vector depending upon its front element.
	  *  for all the loggerClientHandle print all the messages with lowest timestamp
  *
  */
void* Logger::LoggerLoop()
{
	logMessage(INFO, "Starting the Logger [ %s : %p ] thread [ %s : %p ] with LoggerLoop: Will write the logs", _name, this, _thread->_name, _thread);

	while(_thread->isActive())
	{
		// Below lock is required as _loggerClientHandlesVector could be changing
		RLock rlock(_mutex);
		if(! _loggerClientHandlesVector.empty())
		{
			for(auto itr : _loggerClientHandlesVector)
			{
				std::shared_ptr<LogMessage> pMsg = nullptr; ;
				while(pMsg = itr->_logMessageQueue.popPtr())
				{
					writeLog(pMsg.get(), getpid(), itr->_thread->getThreadID());
				}
			}
		}

		timespec syncTime = globalClock();
		if(!(syncTime.tv_sec % 10))
		{
			fflush(_logFileHandle);
		}
	}
	_thread->stop();
	return nullptr;
}

/**
 * \brief: writeLog
 *		Actual write in the log file
 *		Write mutex is already aquired in the main Logger::LoggerLoop
 */
void Logger::writeLog(LogMessage *msg, uint32_t pid, uint32_t tid)
{
	struct tm  ts;
    char       buf[80];
    char       funbuf[50];
	time_t     now = msg->_timeStamp.tv_sec;
	ts = *localtime(&now);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %Z %H:%M:%S", &ts);
	snprintf(funbuf, 50, "%s:%u::%s", msg->_file, msg->_lineNo, msg->_function);
	fprintf(_logFileHandle, "%s.%-.6ld [%-5d,%-5d] %-5s %-50s %s\n", buf, msg->_timeStamp.tv_nsec, pid, tid, Logger::getLogLevelString(msg->_level), funbuf , msg->_message);
}

/**
 * LoggerClientHandle: creats loggerHandle 
 *	@precondition	: should be only called by the getClientHandleForLogger function 
 */
LoggerClientHandle::LoggerClientHandle(const MarketDataApplication *appInstance, Thread *thread, uint32_t logQueueSize, LogLevel level)
		: _logMessageQueue()
		, _thread(thread)
		, _appInstance(appInstance)
		, _level(level)
		, _logQueueSize(logQueueSize)
		, _logFileHandle(appInstance->getLoggerInstance()->getLogFileHandle())
{
}

/**
 * logAndPut : Writes the log message in the queue
 */
void LoggerClientHandle::logAndPut(LogLevel LEVEL, const char *FILENAME, uint32_t LINE, const char *func, const char *FORMAT, ...)
{
	va_list ARGS;
	va_start(ARGS, FORMAT);

	std::shared_ptr<LogMessage> pMsg = _logMessageQueue.pushPtrSpin();

	pMsg->_timeStamp = globalClock();
	pMsg->_file = FILENAME;
	pMsg->_function = func;
	pMsg->_lineNo = LINE;
	pMsg->_level = LEVEL;

	vsnprintf(pMsg->_message, LOG_MSG_SIZE, FORMAT, ARGS);
	return;
}
