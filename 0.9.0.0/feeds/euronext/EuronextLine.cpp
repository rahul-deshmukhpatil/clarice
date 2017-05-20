#include <netinet/in.h>
#include <stdint.h>

#include "infra/logger/Logger.h"
#include "infra/thread/Thread.h"

#include "base/MarketDataApplication.h"
#include "base/ProductInfo.h"
#include "base/Packet.h"
#include "base/Order.h"
#include "base/API.h"

#include "feeds/euronext/EuronextFeedHandler.h"
#include "feeds/euronext/EuronextLineGroup.h"
#include "feeds/euronext/EuronextLine.h"
#include "feeds/euronext/EuronextMessages.h"


using namespace base;
using namespace euronext;

void EuronextLine::initializeParseFunctions()
{
	// System Messages
	parseFuncMap[uint8_t(MessageType::LoginRequest)/*0x01*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::loginRequest), false, 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::LoginResponse)/*0x02*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::loginResponse), false, 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::LogoutRequest)/*0x05*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::logoutRequest), false, 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::ReplayRequest)/*0x03*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::replayRequest), false, 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::ReplayResponse)/*0x04*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::replayResponse), false, 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SnapshotRequest)/*0x81*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::snapshotRequest), true, offsetof(struct MillSnapshotRequest, instrID), ALL_CALLBACKS);
	parseFuncMap[uint8_t(MessageType::SnapshotResponse)/*0x82*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::snapshotResponse), false, 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SnapshotComplete)/*0x83*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::snapshotComplete), false, 0, NO_CALLBACK);

	// Application messages
	parseFuncMap[uint8_t(MessageType::Time)/*0x54*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::exchangeTime), false, 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SystemEvent)/*0x53*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::systemEvent), false, 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SymDir)/*0x52*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::symDir), false, offsetof(struct MillSymDir, instrID), NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SymStatus)/*0x48*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::symStatus), true, offsetof(struct MillSymStatus, instrID), STATUS);
	parseFuncMap[uint8_t(MessageType::AddOrder)/*0x41*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::addOrder), true, offsetof(struct MillAddOrder, instrID), QUOTE|TRADE);
	parseFuncMap[uint8_t(MessageType::OrderDeleted)/*0x44*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::orderDeleted), true, offsetof(struct MillOrderDeleted, instrID), QUOTE|TRADE);
	parseFuncMap[uint8_t(MessageType::OrderModified)/*0x55*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::orderModified), true, offsetof(struct MillOrderModified, instrID), QUOTE|TRADE);
	parseFuncMap[uint8_t(MessageType::OrderBookClear)/*0x79*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::orderBookClear), true, offsetof(struct MillOrderBookClear, instrID), QUOTE);
	parseFuncMap[uint8_t(MessageType::OrderExec)/*0x45*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::orderExec), true, offsetof(struct MillOrderExec, instrID), QUOTE|TRADE);
	parseFuncMap[uint8_t(MessageType::OrderExecPxSize)/*0x43*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::orderExecPxSize), true, offsetof(struct MillOrderExecPxSize, instrID), QUOTE|TRADE);
	parseFuncMap[uint8_t(MessageType::Trade)/*0x50*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::trade), true, offsetof(struct MillTrade, instrID), TRADE);
	parseFuncMap[uint8_t(MessageType::OffBookTrade)/*0x78*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::offBookTrade), true, offsetof(struct MillOffBookTrade, instrID), TRADE);
	parseFuncMap[uint8_t(MessageType::TradeBreak)/*0x42*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::tradeBreak), true, offsetof(struct MillTradeBreak, instrID), TRADE);
	parseFuncMap[uint8_t(MessageType::AuctionInfo)/*0x49*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::auctionInfo), true, offsetof(struct MillAuctionInfo, instrID), CUSTOM);
	parseFuncMap[uint8_t(MessageType::Statistics)/*0x77*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::statistics), true, offsetof(struct MillStatistics, instrID), CUSTOM);
	parseFuncMap[uint8_t(MessageType::EnhancedTrade)/*0x90*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::enhancedTrade), true, offsetof(struct MillEnhancedTrade, instrID), TRADE);
	parseFuncMap[uint8_t(MessageType::RecoveryTrade)/*0x76*/] = MessageFunc(static_cast<NativFunc>(&EuronextLine::recoveryTrade), true, offsetof(struct MillRecoveryTrade, instrID), TRADE);
}

// System Messages
void EuronextLine::loginRequest()
{
}

void EuronextLine::loginResponse()
{
}

void EuronextLine::logoutRequest()
{
}

void EuronextLine::replayRequest()
{
}

void EuronextLine::replayResponse()
{
}

void EuronextLine::snapshotRequest()
{
}

void EuronextLine::snapshotResponse()
{
}

void EuronextLine::snapshotComplete()
{
}


// Application messages
void EuronextLine::exchangeTime()
{
	_xtSecs = reinterpret_cast<const MillTime*>(_currentMsg)->seconds; 
}

void EuronextLine::systemEvent()
{
	// Do nothing
}

void EuronextLine::symDir()
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

void EuronextLine::symStatus()
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

void EuronextLine::addOrder()
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

void EuronextLine::orderDeleted()
{

	const MillOrderDeleted *delOrderMsg = reinterpret_cast<const MillOrderDeleted *>(_currentMsg);
	
	Order::Delete(this, _sub, MessageType::OrderDeleted, _lastSeqNo, _xtSecs, delOrderMsg->nanoSecs, delOrderMsg->orderID);
}

void EuronextLine::orderModified()
{
	const MillOrderModified *modOrderMsg = reinterpret_cast<const MillOrderModified *>(_currentMsg);
	Order::ModifyPxSize(this
		, _sub
		, MessageType::OrderModified
		, _lastSeqNo
		, _xtSecs
		, modOrderMsg->nanoSecs
		, modOrderMsg->orderID
		, modOrderMsg->newPrice8 / 100000000.0
		, modOrderMsg->newQuantity);
}

void EuronextLine::orderBookClear()
{
}

void EuronextLine::orderExec()
{
	const MillOrderExec *execOrderMsg = reinterpret_cast<const MillOrderExec *>(_currentMsg);
	Order::Execute(this
			, _sub
			, MessageType::OrderExec
			, _lastSeqNo
			, _xtSecs
			, execOrderMsg->nanoSecs
			, execOrderMsg->orderID);
}

void EuronextLine::orderExecPxSize()
{

	const MillOrderExecPxSize *execPxSizeMsg = reinterpret_cast<const MillOrderExecPxSize *>(_currentMsg);
		Order::ExecutePxSize(this
			, _sub
			, MessageType::OrderExecPxSize
			, _lastSeqNo
			, _xtSecs
			, execPxSizeMsg->nanoSecs
			, execPxSizeMsg->orderID
			, execPxSizeMsg->px8/100000000.0
			, execPxSizeMsg->execQuantity
			, execPxSizeMsg->dispQuantity); 
}

void EuronextLine::trade()
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

void EuronextLine::offBookTrade()
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

void EuronextLine::tradeBreak()
{
}

void EuronextLine::auctionInfo()
{
}

void EuronextLine::statistics()
{
}

void EuronextLine::enhancedTrade()
{
}

void EuronextLine::recoveryTrade()
{
}

Line* EuronextLine::_EuronextLine(const MarketDataApplication *app, FeedHandler *feedHandler, LineGroup *lineGroup, Thread *thread, const pugi::xml_node& lineNode, LineAPI *pLineAPI)
{
	return new EuronextLine(app, reinterpret_cast<EuronextFeedHandler *>(feedHandler), reinterpret_cast<EuronextLineGroup *>(lineGroup), thread, lineNode, pLineAPI); 
}

EuronextLine::EuronextLine(const MarketDataApplication *app, EuronextFeedHandler *euronextFeedHander, EuronextLineGroup *pEuronextLineGroup, Thread *thread, const pugi::xml_node & lineNode, LineAPI *pLineAPI)
	: Line(app, euronextFeedHander, pEuronextLineGroup, thread, lineNode, pLineAPI,static_cast<Start>(&EuronextLine::start), static_cast<ProcessPacket>(&EuronextLine::processPacket),static_cast<GetPackStats>(&EuronextLine::getPacketStats),static_cast<GetPackStatsSnap>(&EuronextLine::getPacketStatsSnap),static_cast<ReRequestMissedPackets>(&EuronextLine::reRequestMissedPackets))
{
}

void EuronextLine::_start(Line *pLine)
{
	return reinterpret_cast<EuronextLine *>(pLine)->start();
}

void EuronextLine::start()
{
	logConsole(DEBUG, "Starting the Line %s", _name);
	initializeParseFunctions();
}

//void EuronextLine::getPacketStats(ContentType& _contentType, uint64_t& _headerSeqNo, uint64_t& _endSeqNo, const char*& _firstMsg)
void EuronextLine::_getPacketStats(Line *pLine)
{
	return reinterpret_cast<EuronextLine *>(pLine)->getPacketStats();
}

void EuronextLine::getPacketStats()
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

void EuronextLine::_getPacketStatsSnap(Line *pLine)
{
	return reinterpret_cast<EuronextLine *>(pLine)->getPacketStatsSnap();
}

void EuronextLine::getPacketStatsSnap()
{
	getPacketStats();

	_isSnapStarted = false;
	_lastSeqNo = _headerSeqNo - 1;
}

bool EuronextLine::_reRequestMissedPackets(Line *pLine, uint64_t first, uint64_t last)
{
	return reinterpret_cast<EuronextLine *>(pLine)->reRequestMissedPackets(first, last);
}

bool EuronextLine::reRequestMissedPackets(uint64_t first, uint64_t last)
{
}

/**
 * Process each message within the packet
 * this function here ensures that _processPacket above is called only once per packet.
 */
void EuronextLine::processPacket()
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



void EuronextLine::processMsg_rev()
{
	uint8_t type = (uint8_t) ((MillMessageHeader *)_currentMsg)->type;
	
	// There is _processorQueue and this message could be assigned to
	// designated thread to process
	logMessage(TRACE, "Processing Message %lu : %lu.%lu",_lastSeqNo, _packet->_recordTime.tv_sec, _packet->_recordTime.tv_nsec);
	
	if(parseFuncMap[type]._couldAssign)
	{
		if(hasInterest(parseFuncMap[type]._callbacks))
		{
			_lineGroup->_mt = _appInstance->globalClock();
			_sub = getIdSub32(parseFuncMap[type]._idOffset);
			getLatency(_lineGroup, _lineGroup->_sf);

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
		_lineGroup->_mt = _appInstance->globalClock();
		(this->*func)();
	}
}

