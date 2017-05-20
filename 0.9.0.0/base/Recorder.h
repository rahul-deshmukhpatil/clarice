#ifndef __RECORDER_H__
#define __RECORDER_H__

#include <map>
#include "infra/thread/Thread.h"
#include "base/BaseCommon.h"
#include "base/Packet.h"

using namespace infra;

namespace base
{
	class Recorder
	{
		public:
			Recorder(const MarketDataApplication *app, const pugi::xml_node &node);
			~Recorder();
			void registerFeed();
			static void* _start(void *);
			void start();
			void stop();
			void* recorderLoop();
			BoostSPSCQueue<Packet>* registerNetworkReader(const NetworkReader *nr);

			const MarketDataApplication *_appInstance;
			const char _name[NAME_SIZE];
			Thread *_thread;

		private:
			std::vector<std::pair<const NetworkReader*, BoostSPSCQueue<Packet> *> > _recordQueues;
	};
}

#endif //__RECORDER_H__
