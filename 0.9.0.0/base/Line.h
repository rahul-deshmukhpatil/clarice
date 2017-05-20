#ifndef __LINE_H__
#define __LINE_H__

#include <arpa/inet.h>
#include <map>
#include <unordered_map>

#include "infra/containers/spsc/BoostSPSCQueue.h"
#include "infra/socket/Socket.h"
#include "base/BaseCommon.h"
#include "infra/InfraCommon.h"

using namespace infra;

namespace base
{
	enum ReturnValue
	{
		PROCESSED = 0,
		NOT_PROCESSED = -1
	};

	enum class LineState
	{
		JUST_STARTED,	// Intraday or morning start
		SNAPPING,		// Snapping becuase of intraday 
		NORMAL,			// Properly snapped
		GAPPED,			// It is gapped currently
	};

	enum class ContentType 
	{
		HB,
		DATA_PACKET,
	};

	class SeqRange
	{
		public:
		bool operator < (const SeqRange &rhs) const
		{
			return _headerSeqNo < rhs._headerSeqNo; 
		}

			SeqRange(uint64_t head, uint64_t end)
			: _headerSeqNo(head)
			, _endSeqNo(end)
			{
			}

			uint64_t _headerSeqNo;
			uint64_t _endSeqNo;
	};

	typedef void (Line::*NativFunc)();	
	class MessageFunc
	{
		public: 
		MessageFunc()
		: _nativFunc(nullptr)
		, _couldAssign(false)
		, _size(0)
		, _idOffset(0)
		, _callbacks(NO_CALLBACK)
		{
		}
		
		MessageFunc(NativFunc func, bool assign, uint16_t size, uint16_t offset, CallBacks callbacks)
		: _nativFunc(func)
		, _couldAssign(assign)
		, _size(size)
		, _idOffset(offset)
		, _callbacks(callbacks)
		{
		}

		NativFunc	_nativFunc;
		bool		_couldAssign;
		uint16_t	_size;
		uint16_t	_idOffset;
		uint32_t	_callbacks;
	};

	const char *getChannelString(ChannelType type);

	class IP : public sockaddr_in 
	{
		public:
			IP();
			static const char *IPString[24];	
			const std::string toString() const;
			bool operator < (const IP &rhs) const;
			bool operator == (const IP &rhs) const;
			bool operator != (const IP &rhs) const;
			
			static IP getParsedIPPort(const MarketDataApplication *_appInstance, const pugi::xml_node &node, const char *ipType, const std::string ipString);
			static IP getParsedIPPort(const MarketDataApplication *_appInstance, const pugi::xml_node &lineNode, const char *ipType);

			int32_t type() const
			{
				return _type;
			}

		private:
			int32_t _type;
	};

	class PacketAddress : public IP
	{
		public:
			PacketAddress();
			PacketAddress(const IP& ip, IPType ipType, ChannelType channelType, const Line *line, const LineGroup *lineGroup);
			
			using IP::operator ==;

			const std::string toString() const;
			BoostSPSCQueue<Packet> *pPacketQueue() const;
			Socket& socket() const;

			const Line *_pLine;
			const LineGroup *_lineGroup;
			IPType _ipType;
			ChannelType _channelType;
			BoostSPSCQueue<Packet>  *_pPacketQueue; 
			Socket	_socket;
	};

    class Line
    {
		public:

			typedef void (Line::*Start)();
			typedef void (Line::*GetPackStats)();
			typedef void (Line::*ProcessPacket)();
			typedef void (Line::*GetPackStatsSnap)();
			typedef bool (Line::*ReRequestMissedPackets)(uint64_t, uint64_t);

			Line(const MarketDataApplication *, FeedHandler *, LineGroup *, Thread *thread, const pugi::xml_node &, LineAPI *, Start, ProcessPacket processFunc, GetPackStats, GetPackStatsSnap, ReRequestMissedPackets);

            const char _name[NAME_SIZE];
			
			int32_t process();
			const ProcessPacket _processPacket;
			void reset();
			void insert(uint64_t orderID, Order *order);
			Order* find(uint64_t orderID) const;
			void erase(uint64_t orderID) const;
			void printStats() const;
			
			// Related with the sequencing of the packet
			Packet *_packet; // This is packet to be procesed
			uint64_t	_lastSeqNo;		//Last processed seq no
			uint64_t	_headerSeqNo;	//Seq no of first msg Or if HB packet this is next expected
			uint64_t	_endSeqNo;		//Seq no of last msg Or if HB then _endSeqNo = _headerSq-1
			uint64_t	_gapRequestedTill; //Seq no of last re-requesetd gap message 
			const char  *_currentMsg;	//Points to current msg to be processed within _packetData
			bool _snapshotEnabled;
			bool _isSnapStarted;	// We have recieved the start of snapshot
			bool _isSnapshotEnd;	// We have recieved the end of snapshot
			uint64_t _snapshotSyncNo; // To this seq no from the mainline we are synced.
			LineState	_lineState; // Is it just started[to be snapped], normal[snapped], gapped
			ContentType	_contentType; // It is data packet or just HB
			uint32_t _xtSecs;		// Commong msg exchange epoch in packet header or time msg
			uint32_t _xtNanoSecs;	// Commong msg exchange nano sec part from nano seconds time msg
			MessageFunc parseFuncMap[256];// = {{nullptr, false, 0}};

			void start();
			void printStats(const Thread *_thread) const;
			Subscription* getIdSub32(uint16_t offset);
			
			const MarketDataApplication	*_appInstance;			
			FeedHandler *_feedHandler;
			LineGroup *_lineGroup;
			const Thread *_thread;	
			PriceLevelPool *_priceLevelPool;//pool for allocating price levels
			OrderPool *_orderPool; // order object pool
			bool _maintainOrders;

			void registerCallbacks(CallBacks );
			bool hasInterest(CallBacks callback) const;
			bool hasStrictInterest(CallBacks callback) const;
			LineAPI *_lineAPI;

		protected:
			Subscription *_sub;
		
		private:
			void createLineIPs(const pugi::xml_node &lineNode);
			
			int32_t processMainLinePacket();
			int32_t processSnapshotPacket();

			void processPacket();
			void reRequestMissedPacket();
			int32_t addToGapCache();
			void processGapCache();
			void processSnapCache();
			void reRequestSnap();
			int32_t addToSnapCache();
			int32_t addToCache(std::map<SeqRange, Packet *> &_cache);
			void clearGapCache();
			void clearSnapCache();

			PacketAddress _IPs[static_cast<int>(IPType::IPTYPE_MAX)];

			GetPackStats _nativGetPacketStats;
			GetPackStatsSnap _nativGetPacketStatsSnap;
			ReRequestMissedPackets _nativReRequestMissedPackets;
			Start _nativStart;

			//This must be map of the shared_ptr and these Packets could
			//be assigned to the assigner thread. 
			std::map<SeqRange, Packet *> _snapCache;
			std::map<SeqRange, Packet *> _gapCache;

			std::unordered_map<uint64_t, Order*> *_orderMap; // order object map 
			CallBacks _callbacks;
    };
}

#endif //__LINE_H__
