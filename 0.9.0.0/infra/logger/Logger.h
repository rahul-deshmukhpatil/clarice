#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>
#include <vector>
#include <queue>

#include "infra/containers/spsc/BoostSPSCQueue.h"
#include "infra/InfraCommon.h"
#include "infra/pugixml/pugixml.h"
#include "infra/lock/Lock.h"

#include "base/BaseCommon.h"

namespace infra
{
// @TODO:
// Later this console log will write into the logger file directly
// once we have the proper queue and lock mechnism we want

#define logConsole(LEVEL,FORMAT...) {\
			Logger::_loggerInstance->printInFile(infra::LogLevel::LEVEL, __FILE__, __LINE__, __func__, FORMAT);\
			}

#define logConstructor(LEVEL,FORMAT...) if(infra::LogLevel::LEVEL >=  logger->level()) {\
				logger->logAndPut(infra::LogLevel::LEVEL, __FILE__, __LINE__, __func__, FORMAT);\
			}

#define logMessage(LEVEL,FORMAT...) if(infra::LogLevel::LEVEL >=  _thread->loggerClient()->level()) {\
				_thread->loggerClient()->logAndPut(infra::LogLevel::LEVEL, __FILE__, __LINE__, __func__, FORMAT);\
			}

#define LOG_MSG_SIZE 512

	/**
	 *	\brief Log Message structure.
	 *
	 * @TODO: See that how message could be printed with minimal effort
	 *			See how Log message first string with threadNo could be alredy formed
	 */
	class LogMessage: public BoostSPSCQueueElement
	{
		public:
			timespec		_timeStamp;		// Logging timestamp		
			const char	*_file;			// Name of source file
			const char	*_function;		// Function
			uint32_t	_lineNo;		// Line no of file
			LogLevel	_level;			// Level of log being printed
			char 		_message[LOG_MSG_SIZE];	// Message Buffer
	};

	/**
	 *	\brief LoggerClientHandle 
	 *			This is uniq per thread and represents the per thread message queue 
	 *	@TODO: replace the std::queue with the efficient data structure	
	 */
	class LoggerClientHandle 
	{
		public:
			LoggerClientHandle(const base::MarketDataApplication *appInstance, Thread *thread, uint32_t logQueueSize, LogLevel level);

 			//@return: Returns level of the loggerClientHandle
			inline LogLevel level() const { return _level;}

			void logAndPut(LogLevel LEVEL, const char *FILENAME, uint32_t LINE, const char *func, const char *FORMAT, ...);
			static bool compareLess(const LoggerClientHandle *lhs, const LoggerClientHandle *rhs);

			BoostSPSCQueue<LogMessage> _logMessageQueue;	// Queue of log messages
			const Thread	*_thread; 		//  Thread to which LoggerClientHandle is associated

		private:
			const base::MarketDataApplication *_appInstance;
			LogLevel _level;					// log level
			uint32_t _logQueueSize;				// size of queue

			friend class Logger;	// only Logger functions can call LoggerClientHandle 
			const FILE *_logFileHandle; /// file of the logger Instance;
	};



	/*
	 *	Logger: singleton instance of global logger, 
	 *				represent log writer thread
	 *
	 *  This is the first class created by application.
	 *	This runs as single thread, running in while loop,
	 *	it reads all the log SPSC queues written by processing
	 *	threads and writes the oldest log first in the file.
	 *	
	 */
	class Logger
	{
		public:
			static Logger* getLoggerInstance(const std::string& appName, const base::MarketDataApplication *appInstance, pugi::xml_document& doc);
			Logger(const std::string& appName, const base::MarketDataApplication *appInstance, pugi::xml_node LoggerNode);
			~Logger();

			LoggerClientHandle *getClientHandleForLogger(Thread *_thread);
			const FILE *getLogFileHandle() const;
			Thread *thread() const; // get the Logger::_thread
 			
			//@return: Returns level of the logger
			LogLevel level() const { return _level;}

			static LogLevel getLoggerLevelFromString(std::string level); 
			static const char *getLogLevelString(LogLevel level);
			void printInFile(LogLevel LEVEL, const char *FILENAME, uint32_t LINE, const char *func, const char *FORMAT, ...);

			void writeLog(LogMessage *msg, uint32_t pid, uint32_t tid);
			static Logger* _loggerInstance; // Global singleton _loggerInstance
			void stop(); //stop the logger

		private:

			static void *startLogger(void *arg);	// main routine of the Logger

			void *LoggerLoop();

			const base::MarketDataApplication *_appInstance;
			Thread *_thread;				// Loggers own thread
			
			std::mutex	_mutex;
			FILE *_logFileHandle;
			std::vector<LoggerClientHandle *> _loggerClientHandlesVector;	// vector of logger client handles
			std::map<Thread *, LoggerClientHandle *> _threadToClientHandlesMap;	// Thread to client handle map
			const char _name[NAME_SIZE];	// Name of logger
			std::string	_logFile;			// Log file for writing logs
			uint32_t _maxFileSize;			// Maximum size of the log file
			uint32_t _logQueueSize;			// Default value of every logQueue 
			LogLevel _level;				// Loggers own log level and default for other log queues
	};
}
#endif //__LOGGER_H__
