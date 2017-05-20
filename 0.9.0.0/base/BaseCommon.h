#ifndef __BASECOMMON_H__
#define __BASECOMMON_H__

#include "pool/object_pool.hpp"
#include "infra/gzstream/gzstream.h"
#include <stdint.h>
#include <string>
#include <boost/assert.hpp>

namespace base 
{

#if defined ASSERT_ENABLED 

	#define ASSERT(cond, msg) {\
		if(!(cond))\
		{\
			std::stringstream str;\
			str << msg;\
			BOOST_ASSERT_MSG(cond, str.str().c_str());\
		}\
	}
#else
	#define ASSERT(...) 
#endif

	#define NAME_SIZE 20
	
	enum IDType
    {
        NUMERIC = 0,
        ALPHANUM = 1,
		IDTYPE_UNKNOWN = 2
    };

	enum Side
	{
		BUY = 'B',
		SELL = 'S',
		SIDE_UNKNOWN = 'U'
	};

	class ID
    {
        public:
			ID();
			ID(char *strID);
			ID(uint64_t numericID);
            ~ID();

			IDType type() const {return _IDType;}
			
			std::string to_string() const;

			uint64_t numeric() const;
			const char* alphanum() const;
			
        private:
			union 
			{
            	uint64_t  _numericID;
	            char      _alphaNumID[20];
			};
            IDType _IDType;
    };

	extern "C" const char *CLARICE_HEADER;

	enum PlaybackError
	{
		RECORDING_FINISHED_ABNORMALLY = -3,
		RECORDING_CORRUPT = -2,
		RECORDING_FINISHED = -1,
		SUCCESS = 0,
	};

	enum class PlaybackMode : uint8_t
	{
		FILE_PLAYBACK = 0,
		LIVE_PLAYBACK = 1
	};

	extern "C" 
	{
		PlaybackMode playbackMode(const std::string &strMode);
		const char* playbackModeToStr(const PlaybackMode mode);
	}
	
	enum RecordField
	{
		HEADER = 0,
		TIME = 1,
		IP_PORT = 2,
		LENGTH = 3
	};

	typedef uint32_t CallBacks;

#define CLEAR_CALLBACKS(callbacks) callbacks = NO_CALLBACK
#define ALL_CALLBACKS(callbacks) callbacks = ALL_CALLBACKS
#define SET_CALLBACK(callbacks, type) callbacks |= type
#define CLEAR_CALLBACK(callbacks, type) callbacks &= ~type

	enum Callback
	{
		NO_CALLBACK		= 0x0000,
		TRADE			= 0x0001,
		ORDER			= 0x0002,
		BOOK			= 0x0006,	// 0x06 = ORDER(2) + BOOK(4)
		STATUS			= 0x0008,
		QUOTE			= 0x0016,	// 0x16 = BOOK(6)  + QUOTE(0x10)
		CUSTOM			= 0x0020,
		ALL_CALLBACKS	= 0xFFFF	// 0XFFFF
	};

	enum class ChannelType : uint32_t
	{
		MAIN,
		SNAP,
		RETRANS,
//		MAIN_TCP,
//		SEC_TCP,
//		RETRANS_TCP,
		CHANNELTYPE_MAX
	};

	enum class IPType : uint32_t
	{
		PRIM_MC,	// Primary/Secondary real time data MC line
		SEC_MC,

		DR_PRIM_MC,
		DR_SEC_MC,

		PRIM_TCP, // Primary/Secondary selective retranse data TCP line
		SEC_TCP,

		DR_PRIM_TCP,
		DR_SEC_TCP,

		PRIM_MC_SNAP, // Primary/Secondary snapshot data MC line
		SEC_MC_SNAP,

		DR_PRIM_MC_SNAP,
		DR_SEC_MC_SNAP,

		PRIM_TCP_SNAP, //  Primary/Secondary snapshot data TCP line 
		SEC_TCP_SNAP,

		DR_PRIM_TCP_SNAP,
		DR_SEC_TCP_SNAP,

		PRIM_MC_RETRANS, // Primary/Secondary selective retranse data MC line
		SEC_MC_RETRANS,

		DR_PRIM_MC_RETRANS,
		DR_SEC_MC_RETRANS,

		PRIM_TCP_RETRANS, // Primary/Secondary TCP retranse line
		SEC_TCP_RETRANS,

		DR_PRIM_TCP_RETRANS,
		DR_SEC_TCP_RETRANS,

		IPTYPE_MAX
	};
#define LATENCY_SIZE 5000
class FeedHandler;
	class Recording
	{
		public:
			Recording(const FeedHandler *feed, const std::string &filename, bool write);
			~Recording();
			
			infra::igzstream *_in;
			infra::ogzstream *_out;
			const std::string _filename;
			const FeedHandler *_feedHandler;
			infra::ogzstream *_reRecord;
	};

	class Base;
	class Order;
	class Book;
	class Quote;
	class PriceLevels;
	class PriceLevel;
	class Quote;
	class Trade;
	class Status;
	class Custom;
	class ProductInfo;

	class MarketDataApplication;
	class NetworkReader;
	class Subscription;
	class SubAndQueue;
	class SubscriptionAPI;
	class LineGroup;
	class Line;
	class Packet;
	class IP;
	class PacketAddress;
	class ApplicationAPI;
	class FeedAPI;
	class LineGroupAPI;
	class LineAPI;
	class SubscriptionAPI;
	class Recorder;
	enum class FeedID;
	
	typedef boost::object_pool<base::PriceLevel> PriceLevelPool;
	typedef boost::object_pool<base::Order> OrderPool;

	/*
	 * Clock instance to get the appliction time.
	 * It is updated constantly by the start loop in the startApplication
	 * and accessed heavily by the logger and whenever system time is required
	 * by the linehandlers and the processing threads 
	 */
inline timespec globalClock() ///< Return current time 
	{
		timespec globalClock;
#if 1 //REAL_TIME
		// Take time from real
		clock_gettime(CLOCK_REALTIME, &globalClock);
#else
		// Take time main application thread maintained time
		__int128 *ptr = reinterpret_cast<__int128 *>(&globalClock);
		*ptr = _time.load(memory_order_acquire);
#endif
		return globalClock;
	}

inline void getLatency(uint64_t *latencyArr, timespec &mt)
{
	timespec ct = globalClock();
	int64_t timeDiff = ((ct.tv_sec - mt.tv_sec) * 1000000000) + ct.tv_nsec - mt.tv_nsec; 
	
	if(timeDiff >= LATENCY_SIZE) 
	{
		timeDiff = LATENCY_SIZE - 1;
	} 

	++latencyArr[timeDiff];
}

}
#endif //__BASE_COMMON_H__
