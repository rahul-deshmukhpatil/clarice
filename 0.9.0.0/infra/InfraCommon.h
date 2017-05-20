#ifndef __INFRACOMMON_H__
#define __INFRACOMMON_H__

namespace pugi 
{
	class xml_node;
}

namespace infra 
{
	#define NAME_SIZE 20
	class LogMessage;
	class LoggerClientHandle;
	class Thread;
	enum LogLevel
	{
			TRACE	= 0, ///< Log all the members from the message
			DEBUG	= 1, ///< Something trivial
			SANTY	= 2, ///< Sanitly checks like cross book
			INFO	= 3, ///< Something less trivial
			WARN	= 4, ///< High usage,
			ERROR	= 5, ///< No subscription found. Can not login to recovery
			EXCEPTION	= 6, ///< an exception has occured
			CRITICAL  = 7, ///< System soon gonna fail
			UNKNOWN
	};
}
#endif //__COMMON_H__
