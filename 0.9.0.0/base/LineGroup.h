#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <map>
#include <unordered_map>

#include "base/BaseCommon.h"
#include "infra/InfraCommon.h"
#include "infra/containers/spsc/BoostSPSCQueue.h"

using namespace infra;

namespace base
{
	typedef Line* (*LineConstructor)(const base::MarketDataApplication *, FeedHandler *, LineGroup *, Thread *, const pugi::xml_node&, LineAPI *);
	class LineGroup
	{
		public:
			LineGroup(const MarketDataApplication *app, FeedHandler *feedHandler, const pugi::xml_node & lineGroupNode, LineConstructor lineConstructor, LineGroupAPI *lineGroupAPI);
			void registerCallbacks(CallBacks );
			NetworkReader *networkReader() const;
			Subscription* getIdSub(uint64_t);
			void start();
			void stop();
			void printLatency(const Thread *, const char *latencyType, const uint64_t *arr) const;
			void printStats(const Thread *) const;
			static void* _start(void *); 		///< calls lineGroupLoop 
			void lineGroupLoop(); 				///< main loop

			const char _name[NAME_SIZE];						// Name of the Line group

			BoostSPSCQueue<Packet> *packetQueue() const
			{
				return _packetQueue;
			}

			const FeedHandler* feedHandler() const {return _feedHandler;}
			FeedHandler *_feedHandler;				// Feed manager pointer
			Thread *_thread;

#if defined(__LATENCY__)
			uint64_t _rl[LATENCY_SIZE]; // recieve latency from packet read to callback
			uint64_t _ml[LATENCY_SIZE]; // message processing to callback fire latency
			uint64_t _readLatency[LATENCY_SIZE]; // Time took to detect read packet availibility 
			uint64_t _ql[LATENCY_SIZE]; // avrage read latency of net reader q 
			uint64_t _sf[LATENCY_SIZE]; // Subscription finding latency 
			uint64_t _om[LATENCY_SIZE]; // order map latency 
			uint64_t _op[LATENCY_SIZE]; // order pool latency 
#endif // __LATENCY__

			timespec		_mt; // message time
			const MarketDataApplication	*_appInstance;			// App instance pointer
			
		private:
			void createLines(const pugi::xml_node &configNode);	


			typedef std::map<std::string, Line *> NameToLineMap; 
			NameToLineMap _lineMap;			// name to line map

			NetworkReader *_networkReader;	// Network reader to which all lines of group belong

			BoostSPSCQueue<Packet>  *_packetQueue; 
			LineConstructor _lineConstructor;

			std::unordered_map<uint64_t, Subscription *> _idSubMap;
			std::map<char *, Subscription *> _symbolSubMap;

			LineGroupAPI *_lineGroupAPI;
	};
}
#endif //__CONNECTION_H__
