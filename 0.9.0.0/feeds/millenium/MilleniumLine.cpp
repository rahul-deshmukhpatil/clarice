#include <netinet/in.h>
#include <stdint.h>

#include "infra/logger/Logger.h"
#include "infra/thread/Thread.h"

#include "base/MarketDataApplication.h"
#include "base/ProductInfo.h"
#include "base/Packet.h"
#include "base/Order.h"
#include "base/API.h"

#include "feeds/millenium/MilleniumFeedHandler.h"
#include "feeds/millenium/MilleniumLineGroup.h"
#include "feeds/millenium/MilleniumLine.h"
#include "feeds/millenium/MilleniumMessages.h"


using namespace base;
using namespace millenium;

void MilleniumLine::initializeParseFunctions()
{
	// System Messages
	parseFuncMap[uint8_t(MessageType::LoginRequest)/*0x01*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::loginRequest), false, sizeof(MillLoginRequest), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::LoginResponse)/*0x02*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::loginResponse), false, sizeof(MillLoginResponse), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::LogoutRequest)/*0x05*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::logoutRequest), false, sizeof(MillLogoutRequest), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::ReplayRequest)/*0x03*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::replayRequest), false, sizeof(MillReplayRequest), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::ReplayResponse)/*0x04*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::replayResponse), false, sizeof(MillReplayResponse), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SnapshotRequest)/*0x81*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::snapshotRequest), true, sizeof(MillSnapshotRequest), offsetof(struct MillSnapshotRequest, instrID), ALL_CALLBACKS);
	parseFuncMap[uint8_t(MessageType::SnapshotResponse)/*0x82*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::snapshotResponse), false, sizeof(MillSnapshotResponse), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SnapshotComplete)/*0x83*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::snapshotComplete), false, sizeof(MillSnapshotComplete), 0, NO_CALLBACK);

	// Application messages
	parseFuncMap[uint8_t(MessageType::Time)/*0x54*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::exchangeTime), false, sizeof(MillTime), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SystemEvent)/*0x53*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::systemEvent), false, sizeof(MillSystemEvent), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SymDir)/*0x52*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::symDir), false, sizeof(MillSymDir), offsetof(struct MillSymDir, instrID), NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SymStatus)/*0x48*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::symStatus), true, sizeof(MillSymStatus), offsetof(struct MillSymStatus, instrID), STATUS);
	parseFuncMap[uint8_t(MessageType::AddOrder)/*0x41*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::addOrder), true, sizeof(MillAddOrder), offsetof(struct MillAddOrder, instrID), QUOTE|TRADE);
	parseFuncMap[uint8_t(MessageType::OrderDeleted)/*0x44*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::orderDeleted), true, sizeof(MillOrderDeleted), offsetof(struct MillOrderDeleted, instrID), QUOTE|TRADE);
	parseFuncMap[uint8_t(MessageType::OrderModified)/*0x55*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::orderModified), true, sizeof(MillOrderModified), offsetof(struct MillOrderModified, instrID), QUOTE|TRADE);
	parseFuncMap[uint8_t(MessageType::OrderBookClear)/*0x79*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::orderBookClear), true, sizeof(MillOrderBookClear), offsetof(struct MillOrderBookClear, instrID), QUOTE);
	parseFuncMap[uint8_t(MessageType::OrderExec)/*0x45*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::orderExec), true, sizeof(MillOrderExec), offsetof(struct MillOrderExec, instrID), QUOTE|TRADE);
	parseFuncMap[uint8_t(MessageType::OrderExecPxSize)/*0x43*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::orderExecPxSize), true, sizeof(MillOrderExecPxSize), offsetof(struct MillOrderExecPxSize, instrID), QUOTE|TRADE);
	parseFuncMap[uint8_t(MessageType::Trade)/*0x50*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::trade), true, sizeof(MillTrade), offsetof(struct MillTrade, instrID), TRADE);
	parseFuncMap[uint8_t(MessageType::OffBookTrade)/*0x78*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::offBookTrade), true, sizeof(MillOffBookTrade), offsetof(struct MillOffBookTrade, instrID), TRADE);
	parseFuncMap[uint8_t(MessageType::TradeBreak)/*0x42*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::tradeBreak), true, sizeof(MillTradeBreak), offsetof(struct MillTradeBreak, instrID), TRADE);
	parseFuncMap[uint8_t(MessageType::AuctionInfo)/*0x49*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::auctionInfo), true, sizeof(MillAuctionInfo), offsetof(struct MillAuctionInfo, instrID), CUSTOM);
	parseFuncMap[uint8_t(MessageType::Statistics)/*0x77*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::statistics), true, sizeof(MillStatistics), offsetof(struct MillStatistics, instrID), CUSTOM);
	parseFuncMap[uint8_t(MessageType::EnhancedTrade)/*0x90*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::enhancedTrade), true, sizeof(MillEnhancedTrade), offsetof(struct MillEnhancedTrade, instrID), TRADE);
	parseFuncMap[uint8_t(MessageType::RecoveryTrade)/*0x76*/] = MessageFunc(static_cast<NativFunc>(&MilleniumLine::recoveryTrade), true, sizeof(MillRecoveryTrade), offsetof(struct MillRecoveryTrade, instrID), TRADE);
}

// System Messages
void MilleniumLine::loginRequest()
{
}

void MilleniumLine::loginResponse()
{
}

void MilleniumLine::logoutRequest()
{
}

void MilleniumLine::replayRequest()
{
}

void MilleniumLine::replayResponse()
{
}

void MilleniumLine::snapshotRequest()
{
}

void MilleniumLine::snapshotResponse()
{
}

void MilleniumLine::snapshotComplete()
{
}


// Application messages
void MilleniumLine::exchangeTime()
{
	_xtSecs = reinterpret_cast<const MillTime*>(_currentMsg)->seconds; 
}

void MilleniumLine::systemEvent()
{
	// Do nothing
}

void MilleniumLine::symDir()
{
	const MillSymDir *symDirMsg = reinterpret_cast<const MillSymDir *>(_currentMsg);

		// routine must not be here via _processor thread
		char isin[13];
		strncpy(isin, symDirMsg->isin, sizeof(isin));
		isin[12] = '\0';

		ProductInfo *prodInfo = _feedHandler->getProductInfo(this
				, MessageType::SymDir 
				, symDirMsg->instrID
				, ""
				, isin 
				, _lastSeqNo
				, _xtSecs
				, symDirMsg->nanoSecs);

		//sub->_isActive is skipped to raise prodInfo for all symbols
		Subscription *sub = const_cast<Subscription *>(prodInfo->_sub);
	
	//Raise the callback
	sub->_prodInfo->update(this, MessageType::SymDir, _lastSeqNo, _xtSecs, symDirMsg->nanoSecs);
	(_lineAPI->*(_lineAPI->_onProductInfo))(sub->_prodInfo);

	if(hasInterest(STATUS))
	{
		if(sub)
		{
			InstrState state = InstrState::UNKNOWN;
			
			switch(symDirMsg->status)
			{
				case SymStatusEnum::ACTIVE:
					state = InstrState::OPEN;
					break;

				case SymStatusEnum::HALTED:
					state = InstrState::HALTED;
					break;

				case SymStatusEnum::SUSPENDED:
					state = InstrState::SUSPENDED;
					break;

				case SymStatusEnum::INACTIVE:
					state = InstrState::CLOSED;
					break;
			}

			sub->_status.update(this
				, sub
				, SymDir
				, _lastSeqNo
				, _xtSecs
				, symDirMsg->nanoSecs
				, state);

			//_isActive is not checked in process_rev above
			// Othrewise no other sub callback needs this check
		}
	}
}

void MilleniumLine::symStatus()
{
		const MillSymStatus *symStatusMsg = reinterpret_cast<const MillSymStatus *>(_currentMsg);

		InstrState state = InstrState::UNKNOWN;
		switch(symStatusMsg->trdStatus)
		{
			case TradingStatusEnum::HALTED:
				state = InstrState::HALTED;
				break;

			case TradingStatusEnum::TRADING:
				state = InstrState::OPEN;
				break;

			case TradingStatusEnum::AUCTION_OPEN:
				state = InstrState::AUCTION_OPEN;
				break;

			case TradingStatusEnum::POSTCLOSE:
				state = InstrState::CLOSED;
				break;

			case TradingStatusEnum::CLOSED:
				state = InstrState::CLOSED;
				break;

			case TradingStatusEnum::AUCTION_CLOSE:
				state = InstrState::AUCTION_CLOSE;
				break;

			case TradingStatusEnum::AUCTION_REOPEN:
				state = InstrState::AUCTION_INTRA;
				break;

			case TradingStatusEnum::END_OF_TRADE_REPORTING:
				state = InstrState::CLOSED;
				break;

			case TradingStatusEnum::NO_ACTIVE_SESSION:
				state = InstrState::SUSPENDED;
				break;

			case TradingStatusEnum::END_OF_POST_CLOSE:
				state = InstrState::CLOSED;
				break;

			case TradingStatusEnum::PRETRADING:
				state = InstrState::PREOPEN;
				break;

			case TradingStatusEnum::CLOSING_PRICE_PUBL:
				state = InstrState::CLOSED;
				break;
		}

		_sub->_status.update(this
			, _sub
			, MessageType::SymStatus
			, _lastSeqNo
			, _xtSecs
			, symStatusMsg->nanoSecs
			, state);
}

void MilleniumLine::addOrder()
{
	const MillAddOrder *addOrderMsg = reinterpret_cast<const MillAddOrder*>(_currentMsg);
	Order::Add(this
		, _sub
		, MessageType::AddOrder
		, _lastSeqNo
		, _xtSecs
		, addOrderMsg->nanoSecs
		, (addOrderMsg->px8)/100000000.0
		, addOrderMsg->quantity
		, static_cast<Side>(addOrderMsg->side)
		, addOrderMsg->orderID);
}

void MilleniumLine::orderDeleted()
{

	const MillOrderDeleted *delOrderMsg = reinterpret_cast<const MillOrderDeleted *>(_currentMsg);
	
	Order::Delete(this, MessageType::OrderDeleted, _lastSeqNo, _xtSecs, delOrderMsg->nanoSecs, delOrderMsg->orderID);
}

void MilleniumLine::orderModified()
{
	const MillOrderModified *modOrderMsg = reinterpret_cast<const MillOrderModified *>(_currentMsg);
	Order::ModifyPxSize(this
		, MessageType::OrderModified
		, _lastSeqNo
		, _xtSecs
		, modOrderMsg->nanoSecs
		, modOrderMsg->orderID
		, modOrderMsg->newPrice8 / 100000000.0
		, modOrderMsg->newQuantity);
}

void MilleniumLine::orderBookClear()
{
}

void MilleniumLine::orderExec()
{
	const MillOrderExec *execOrderMsg = reinterpret_cast<const MillOrderExec *>(_currentMsg);
	Order::Execute(this
			, MessageType::OrderExec
			, _lastSeqNo
			, _xtSecs
			, execOrderMsg->nanoSecs
			, execOrderMsg->orderID);
}

void MilleniumLine::orderExecPxSize()
{

	const MillOrderExecPxSize *execPxSizeMsg = reinterpret_cast<const MillOrderExecPxSize *>(_currentMsg);
		Order::ExecutePxSize(this
			, MessageType::OrderExecPxSize
			, _lastSeqNo
			, _xtSecs
			, execPxSizeMsg->nanoSecs
			, execPxSizeMsg->orderID
			, execPxSizeMsg->px8/100000000.0
			, execPxSizeMsg->execQuantity
			, execPxSizeMsg->dispQuantity); 
}

void MilleniumLine::trade()
{
	const MillTrade *tradeMsg = reinterpret_cast<const MillTrade*>(_currentMsg);
	_sub->_trade.update(this
			, MessageType::Trade
			, _lastSeqNo
			, _xtSecs
			, tradeMsg->nanoSecs
			, tradeMsg->px8/100000000.0
			, tradeMsg->execQuantity
			, SIDE_UNKNOWN 
			, tradeMsg->tradeID);
}

void MilleniumLine::offBookTrade()
{
	//OFF book trades are not contained within the subscription object
	// Raise the local trade object
	const MillTrade *tradeMsg = reinterpret_cast<const MillTrade*>(_currentMsg);
	base::Trade trade(this
		, _sub
		, MessageType::OffBookTrade
		, _lastSeqNo
		, _xtSecs
		, tradeMsg->nanoSecs
		, (tradeMsg->px8)/100000000.0
		, tradeMsg->execQuantity
		, SIDE_UNKNOWN
		, tradeMsg->tradeID);
}

void MilleniumLine::tradeBreak()
{
}

void MilleniumLine::auctionInfo()
{
}

void MilleniumLine::statistics()
{
}

void MilleniumLine::enhancedTrade()
{
}

void MilleniumLine::recoveryTrade()
{
}

Line* MilleniumLine::_MilleniumLine(const MarketDataApplication *app, FeedHandler *feedHandler, LineGroup *lineGroup, Thread *thread, const pugi::xml_node& lineNode, LineAPI *pLineAPI)
{
	return new MilleniumLine(app, reinterpret_cast<MilleniumFeedHandler *>(feedHandler), reinterpret_cast<MilleniumLineGroup *>(lineGroup), thread, lineNode, pLineAPI); 
}

MilleniumLine::MilleniumLine(const MarketDataApplication *app, MilleniumFeedHandler *milleniumFeedHander, MilleniumLineGroup *pMilleniumLineGroup, Thread *thread, const pugi::xml_node & lineNode, LineAPI *pLineAPI)
	: Line(app, milleniumFeedHander, pMilleniumLineGroup, thread, lineNode, pLineAPI,static_cast<Start>(&MilleniumLine::start), static_cast<ProcessPacket>(&MilleniumLine::processPacket),static_cast<GetPackStats>(&MilleniumLine::getPacketStats),static_cast<GetPackStatsSnap>(&MilleniumLine::getPacketStatsSnap),static_cast<ReRequestMissedPackets>(&MilleniumLine::reRequestMissedPackets))
{
}

void MilleniumLine::_start(Line *pLine)
{
	return reinterpret_cast<MilleniumLine *>(pLine)->start();
}

void MilleniumLine::start()
{
	logConsole(DEBUG, "Starting the Line %s", _name);
	initializeParseFunctions();
}

//void MilleniumLine::getPacketStats(ContentType& _contentType, uint64_t& _headerSeqNo, uint64_t& _endSeqNo, const char*& _firstMsg)
void MilleniumLine::_getPacketStats(Line *pLine)
{
	return reinterpret_cast<MilleniumLine *>(pLine)->getPacketStats();
}

void MilleniumLine::getPacketStats()
{
	MillPacketHeader *pHeader = (MillPacketHeader *) _packet->packetData();
	_headerSeqNo = pHeader->seqNum;
	_endSeqNo 	 = _headerSeqNo + pHeader->messageCount - 1;
	_currentMsg  = (char *)pHeader + sizeof(MillPacketHeader);

	if(!pHeader->messageCount)
	{
		_contentType = ContentType::HB;
	}
	else
	{
		_contentType = ContentType::DATA_PACKET;
	}
}

void MilleniumLine::_getPacketStatsSnap(Line *pLine)
{
	return reinterpret_cast<MilleniumLine *>(pLine)->getPacketStatsSnap();
}

void MilleniumLine::getPacketStatsSnap()
{
	getPacketStats();

	_isSnapStarted = false;
	_lastSeqNo = _headerSeqNo - 1;
}

bool MilleniumLine::_reRequestMissedPackets(Line *pLine, uint64_t first, uint64_t last)
{
	return reinterpret_cast<MilleniumLine *>(pLine)->reRequestMissedPackets(first, last);
}

bool MilleniumLine::reRequestMissedPackets(uint64_t first, uint64_t last)
{
}

/**
 * Process each message within the packet
 * this function here ensures that _processPacket above is called only once per packet.
 */
void MilleniumLine::processPacket()
{
	//PacketHeader has been already processed
	uint64_t i = _headerSeqNo;

	for(; i <= _lastSeqNo; i++)
	{
		//Skip the message
		_currentMsg += ((MillMessageHeader *)_currentMsg)->length;
	}

	for(; i <= _endSeqNo; i++)
	{
		_lastSeqNo++;
		uint64_t len = ((MillMessageHeader *)_currentMsg)->length;
		processMsg_rev();
		// increment the message pointer correctly including length of message header
		_currentMsg += len;	
	}

	//@TODO: assert the below check
	if(_currentMsg != _packet->packetData() + _packet->packetDataLen())
	{
		logMessage(EXCEPTION, "Wrong Packet Processing _currentMsg %p != %p [packet Start:%p + Packet Len %u]", _currentMsg, _packet->packetData()+_packet->packetDataLen(), _packet->packetData(), _packet->packetDataLen());	
	}
}

void MilleniumLine::processMsg_rev()
{
	uint8_t type = (uint8_t) ((MillMessageHeader *)_currentMsg)->type;
	
	// There is _processorQueue and this message could be assigned to
	// designated thread to process
	logMessage(TRACE, "Processing Message %lu : %lu.%lu",_lastSeqNo, _packet->_recordTime.tv_sec, _packet->_recordTime.tv_nsec);
	
	if(parseFuncMap[type]._couldAssign)
	{
		if(hasInterest(parseFuncMap[type]._callbacks))
		{
			_lineGroup->_mt = globalClock();
#ifdef __SUBMAP_LATENCTY__
			_sub = getIdSub32(parseFuncMap[type]._idOffset);
			getLatency(_lineGroup, _lineGroup->_sf, _lineGroup->_mt);
#else
			_sub = getIdSub32(parseFuncMap[type]._idOffset);
#endif
			if(_sub)
			{	
				NativFunc func = parseFuncMap[type]._nativFunc;
				(this->*func)();
			}
		}
	}
	else // process the message locally, line/feed specefic message or for group of symbols 
	{
		_sub = nullptr;
		NativFunc func = parseFuncMap[type]._nativFunc;
		_lineGroup->_mt = globalClock();
		(this->*func)();
	}
}

