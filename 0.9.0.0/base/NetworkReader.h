#ifndef __NETWORK_READER_H__
#define __NETWORK_READER_H__

#include <mutex>
#include <vector>
#include <map>
#include <set>

#include "infra/InfraCommon.h"
#include "base/BaseCommon.h"
#include "base/Packet.h"
#include "base/Recorder.h"

using namespace infra;

namespace base 
{
	class NetworkReader
	{
		public:
			NetworkReader(const MarketDataApplication *app, const pugi::xml_node &readerNode);
			void registerLineGroup(LineGroup *lineGroup);
			void start(); // Start the main thread associated with the NetworkReader 
			void stop(); // Stop the NetworkReader thread
			static void* start(void *); // calls networkReaderLoop 
			void addIPToNetworkReader(const PacketAddress* pPacketAddress);
			void removeIPFromNetworkReader(const PacketAddress* pPacketAddress);
			void networkReaderLoop(); // main loop for live env
			void networkReaderLoopFile(); // main loop for playback
			int32_t readPacket(Recording *); //read paacket from recording stream
			void updateIPVector();	// add/delete ips from _ipReadVector 

			const char _name[NAME_SIZE];	// Name of thread belonging to network reader
			bool _stopApp;

		private:
			const MarketDataApplication *_appInstance; // App instance
			Thread 		*_thread;

			std::vector<const PacketAddress *> _ipReadVector;  // read these IPs for packets
			
			std::mutex		_mutexToAddRemove;
			PlaybackMode 	_mode;
			bool			_updateIPVector;
			std::vector<const PacketAddress*> _addIPs; // All IPs to add to read 
			std::vector<const PacketAddress*> _removeIPs; // All IPs to add to read 

			std::map<FeedHandler *, std::vector<Recording *> > _feedStreams;
			BoostSPSCQueue<Packet>* _recordQueue;
	};
}

#endif //__NETWORK_READER_H__
