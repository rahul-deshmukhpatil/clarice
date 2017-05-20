#include <netinet/in.h>
#include <stdint.h>

#include "infra/logger/Logger.h"
#include "infra/thread/Thread.h"

#include "base/MarketDataApplication.h"
#include "base/ProductInfo.h"
#include "base/Packet.h"
#include "base/Order.h"
#include "base/API.h"

#include "feeds/omxnordic/OMXNordicFeedHandler.h"
#include "feeds/omxnordic/OMXNordicLineGroup.h"
#include "feeds/omxnordic/OMXNordicLine.h"
#include "feeds/omxnordic/OMXNordicMessages.h"
#include "feeds/omxnordic/OMXNordicUtils.h"


using namespace base;
using namespace omxnordic;

void OMXNordicLine::initializeParseFunctions()
{
	// Application messages
	parseFuncMap[uint8_t(MessageType::Time)/*T*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::exchangeTime), false, sizeof(OMXNordicTime), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::MSeconds)/*T*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::exchangeMSeconds), false, sizeof(OMXNordicMSeconds), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::SystemEvent)/*S*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::systemEvent), false, sizeof(OMXNordicSystemEvent), 0, NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::StateMessage)/*O*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::stateMessage), false, sizeof(OMXNordicStateMessage), 0, STATUS);
	parseFuncMap[uint8_t(MessageType::TradingState)/*O*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::tradingState), true, sizeof(OMXNordicTradingState), offsetof(struct OMXNordicTradingState, symbolID), STATUS);
	parseFuncMap[uint8_t(MessageType::RefData)/*R*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::refData), true, sizeof(OMXNordicRefData), offsetof(struct OMXNordicRefData, symbolID), NO_CALLBACK);
//	parseFuncMap[uint8_t(MessageType::CombRefData)/*R*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::combRefData), false, sizeof(OMXNordicCombRefData), offsetof(struct OMXNordicCombRefData, symbolID), NO_CALLBACK);
//	parseFuncMap[uint8_t(MessageType::TickData)/*L*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::tickData), false, sizeof(OMXNordicTickData), offsetof(struct OMXNordicTickData, symbolID), NO_CALLBACK);
	parseFuncMap[uint8_t(MessageType::AddOrder)/*A*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::addOrder), true, sizeof(OMXNordicAddOrder), offsetof(struct OMXNordicAddOrder, symbolID), ORDER);
	parseFuncMap[uint8_t(MessageType::AddOrderMPID)/*A*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::addOrderMPID), true, sizeof(OMXNordicAddOrderMPID), offsetof(struct OMXNordicAddOrderMPID, symbolID), ORDER);
	parseFuncMap[uint8_t(MessageType::OrderExec)/*A*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::orderExec), false, sizeof(OMXNordicOrderExec), 0, ORDER);
	parseFuncMap[uint8_t(MessageType::OrderExecPx)/*A*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::orderExecPx), false, sizeof(OMXNordicOrderExecPx), 0, ORDER);
	parseFuncMap[uint8_t(MessageType::OrderCancel)/*A*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::orderCancel), false, sizeof(OMXNordicOrderCancel), 0, ORDER);
	parseFuncMap[uint8_t(MessageType::OrderDelete)/*A*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::orderDelete), false, sizeof(OMXNordicOrderDelete), 0, ORDER);
	parseFuncMap[uint8_t(MessageType::Trade)/*A*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::trade), true, sizeof(OMXNordicTrade), offsetof(struct OMXNordicTrade, symbolID), TRADE);
	parseFuncMap[uint8_t(MessageType::CrossTrade)/*A*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::crossTrade), true, sizeof(OMXNordicCrossTrade), offsetof(struct OMXNordicCrossTrade, symbolID), TRADE);
	parseFuncMap[uint8_t(MessageType::BrokenTrade)/*A*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::brokenTrade), false, sizeof(OMXNordicTrade), 0, TRADE);
	parseFuncMap[uint8_t(MessageType::Imbalance)/*A*/] = MessageFunc(static_cast<NativFunc>(&OMXNordicLine::imbalance), true, sizeof(OMXNordicImbalance), offsetof(struct OMXNordicImbalance, symbolID), TRADE);
}

// Application messages
void OMXNordicLine::exchangeTime()
{
	_xtSecs = reinterpret_cast<const OMXNordicTime*>(_currentMsg)->seconds; 
}

void OMXNordicLine::exchangeMSeconds()
{
	_xtNanoSecs = reinterpret_cast<const OMXNordicTime*>(_currentMsg)->seconds * 1000; 
}

void OMXNordicLine::refData()
{
	const OMXNordicRefData *refDataMsg = reinterpret_cast<const OMXNordicRefData *>(_currentMsg);

	char isin[13];
	strncpy(isin, refDataMsg->isin, sizeof(isin));
	isin[12] = '\0';

	ProductInfo *prodInfo = _feedHandler->getProductInfo(this
			, MessageType::RefData
			, getUint64(refDataMsg->symbolID, sizeof(refDataMsg->symbolID))
			, ""
			, isin 
			, _lastSeqNo
			, _xtSecs
			, _xtNanoSecs);

	//sub->_isActive is skipped to raise prodInfo for all symbols
	Subscription *sub = const_cast<Subscription *>(prodInfo->_sub);

	//Raise the callback
	sub->_prodInfo->update(this, MessageType::RefData, _lastSeqNo, _xtSecs, _xtNanoSecs);
	(_lineAPI->*(_lineAPI->_onProductInfo))(sub->_prodInfo);

}

void OMXNordicLine::systemEvent()
{
}

void OMXNordicLine::stateMessage()
{
/*
		const OMXNordicSymStatus *symStatusMsg = reinterpret_cast<const OMXNordicSymStatus *>(_currentMsg);

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
			, symStatusMsg->nanoseconds
			, state);
	*/
}

void OMXNordicLine::tradingState()
{
		const OMXNordicTradingState *symStatusMsg = reinterpret_cast<const OMXNordicTradingState*>(_currentMsg);

		InstrState state = InstrState::UNKNOWN;
		switch(symStatusMsg->tradingState)
		{
			case TradingStatusEnum::HALTED:
				state = InstrState::HALTED;
				break;

			case TradingStatusEnum::TRADING:
				state = InstrState::OPEN;
				break;

			case TradingStatusEnum::AUCTION:
				state = InstrState::AUCTION;
				break;
		}

		_sub->_status.update(this
			, _sub
			, MessageType::TradingState
			, _lastSeqNo
			, _xtSecs
			, _xtNanoSecs 
			, state);
}

void OMXNordicLine::addOrder()
{
	const OMXNordicAddOrder *addOrderMsg = reinterpret_cast<const OMXNordicAddOrder*>(_currentMsg);
	Order::Add(this
		, _sub
		, MessageType::AddOrder
		, _lastSeqNo
		, _xtSecs
		, _xtNanoSecs
		, getUint64Price(addOrderMsg->price)
		, getUint64(addOrderMsg->quantity, sizeof(addOrderMsg->quantity))
		, static_cast<Side>(addOrderMsg->side)
		, getUint64(addOrderMsg->orderID, sizeof(addOrderMsg->orderID)));
}

void OMXNordicLine::addOrderMPID()
{
	const OMXNordicAddOrderMPID *addOrderMsg = reinterpret_cast<const OMXNordicAddOrderMPID*>(_currentMsg);
	Order::Add(this
		, _sub
		, MessageType::AddOrderMPID
		, _lastSeqNo
		, _xtSecs
		, _xtNanoSecs
		, getUint64Price(addOrderMsg->price)
		, getUint64(addOrderMsg->quantity, sizeof(addOrderMsg->quantity))
		, static_cast<Side>(addOrderMsg->side)
		, getUint64(addOrderMsg->orderID, sizeof(addOrderMsg->orderID)));
}

void OMXNordicLine::orderExec()
{
	const OMXNordicOrderExec *execOrderMsg = reinterpret_cast<const OMXNordicOrderExec *>(_currentMsg);
	Order::ExecuteSize(this
			, MessageType::OrderExec
			, _lastSeqNo
			, _xtSecs
			, _xtNanoSecs 
			, getUint64(execOrderMsg->orderID, sizeof(execOrderMsg->orderID))
			, getUint64(execOrderMsg->execQuantity, sizeof(execOrderMsg->execQuantity)));
}

void OMXNordicLine::orderExecPx()
{
	const OMXNordicOrderExecPx *execPxSizeMsg = reinterpret_cast<const OMXNordicOrderExecPx *>(_currentMsg);
		Order::ExecutePxSize(this
			, MessageType::OrderExecPx
			, _lastSeqNo
			, _xtSecs
			, _xtNanoSecs
			, getUint64(execPxSizeMsg->orderID, sizeof(execPxSizeMsg->orderID))
			, getUint64Price(execPxSizeMsg->tradePrice)
			, getUint64(execPxSizeMsg->execQuantity, sizeof(execPxSizeMsg->execQuantity)));
}

void OMXNordicLine::orderCancel()
{
	const OMXNordicOrderCancel *cancelOrderMsg = reinterpret_cast<const OMXNordicOrderCancel *>(_currentMsg);
	Order::CancelSize(this
		, MessageType::OrderCancel
		, _lastSeqNo
		, _xtSecs
		, _xtNanoSecs 
		, getUint64(cancelOrderMsg->orderID, sizeof(cancelOrderMsg->orderID))
		, getUint64(cancelOrderMsg->cancelledQty, sizeof(cancelOrderMsg->cancelledQty)));
}

void OMXNordicLine::orderDelete()
{
	const OMXNordicOrderDelete *delOrderMsg = reinterpret_cast<const OMXNordicOrderDelete *>(_currentMsg);
	
	Order::Delete(this
				, MessageType::OrderDelete
				, _lastSeqNo
				, _xtSecs
				, _xtNanoSecs 
				, getUint64(delOrderMsg->orderID, sizeof(delOrderMsg->orderID)));
}

void OMXNordicLine::trade()
{
	const OMXNordicTrade *tradeMsg = reinterpret_cast<const OMXNordicTrade*>(_currentMsg);
	_sub->_trade.update(this
			, MessageType::Trade
			, _lastSeqNo
			, _xtSecs
			, _xtNanoSecs 
			, getUint64Price(tradeMsg->price)
			, getUint64(tradeMsg->quantity, sizeof(tradeMsg->quantity))
			, SIDE_UNKNOWN 
			, getUint64(tradeMsg->tradeID, sizeof(tradeMsg->tradeID)));
}

void OMXNordicLine::crossTrade()
{
}

void OMXNordicLine::brokenTrade()
{
}

void OMXNordicLine::imbalance()
{
}

Line* OMXNordicLine::_OMXNordicLine(const MarketDataApplication *app, FeedHandler *feedHandler, LineGroup *lineGroup, Thread *thread, const pugi::xml_node& lineNode, LineAPI *pLineAPI)
{
	return new OMXNordicLine(app, reinterpret_cast<OMXNordicFeedHandler *>(feedHandler), reinterpret_cast<OMXNordicLineGroup *>(lineGroup), thread, lineNode, pLineAPI); 
}

OMXNordicLine::OMXNordicLine(const MarketDataApplication *app, OMXNordicFeedHandler *omxnordicFeedHander, OMXNordicLineGroup *pOMXNordicLineGroup, Thread *thread, const pugi::xml_node & lineNode, LineAPI *pLineAPI)
	: Line(app, omxnordicFeedHander, pOMXNordicLineGroup, thread, lineNode, pLineAPI,static_cast<Start>(&OMXNordicLine::start), static_cast<ProcessPacket>(&OMXNordicLine::processPacket),static_cast<GetPackStats>(&OMXNordicLine::getPacketStats),static_cast<GetPackStatsSnap>(&OMXNordicLine::getPacketStatsSnap),static_cast<ReRequestMissedPackets>(&OMXNordicLine::reRequestMissedPackets))
{
}

void OMXNordicLine::_start(Line *pLine)
{
	return reinterpret_cast<OMXNordicLine *>(pLine)->start();
}

void OMXNordicLine::start()
{
	logConsole(DEBUG, "Starting the Line %s", _name);
	initializeParseFunctions();
}

//void OMXNordicLine::getPacketStats(ContentType& _contentType, uint64_t& _headerSeqNo, uint64_t& _endSeqNo, const char*& _firstMsg)
void OMXNordicLine::_getPacketStats(Line *pLine)
{
	return reinterpret_cast<OMXNordicLine *>(pLine)->getPacketStats();
}

void OMXNordicLine::getPacketStats()
{
	OMXNordicPacketHeader *pHeader = (OMXNordicPacketHeader *) _packet->packetData();
	_headerSeqNo = pHeader->seqNum;
	_endSeqNo 	 = _headerSeqNo + pHeader->msgCount - 1;
	_currentMsg  = (char *)pHeader + sizeof(OMXNordicPacketHeader);

	if(!pHeader->msgCount)
	{
		_contentType = ContentType::HB;
	}
	else
	{
		_contentType = ContentType::DATA_PACKET;
	}
}

void OMXNordicLine::_getPacketStatsSnap(Line *pLine)
{
	return reinterpret_cast<OMXNordicLine *>(pLine)->getPacketStatsSnap();
}

void OMXNordicLine::getPacketStatsSnap()
{
	getPacketStats();

	_isSnapStarted = false;
	_lastSeqNo = _headerSeqNo - 1;
}

bool OMXNordicLine::_reRequestMissedPackets(Line *pLine, uint64_t first, uint64_t last)
{
	return reinterpret_cast<OMXNordicLine *>(pLine)->reRequestMissedPackets(first, last);
}

bool OMXNordicLine::reRequestMissedPackets(uint64_t first, uint64_t last)
{
}

/**
 * Process each message within the packet
 * this function here ensures that _processPacket above is called only once per packet.
 */
void OMXNordicLine::processPacket()
{
	//PacketHeader has been already processed
	uint64_t i = _headerSeqNo;

	for(; i <= _lastSeqNo; i++)
	{
		//Skip the message
		_currentMsg += parseFuncMap[((OMXNordicMsgHeader *)_currentMsg)->type]._size;
	}

	for(; i <= _endSeqNo; i++)
	{
		_lastSeqNo++;
		uint64_t len = parseFuncMap[((OMXNordicMsgHeader *)_currentMsg)->type]._size;
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

void OMXNordicLine::processMsg_rev()
{
	uint8_t type = (uint8_t) ((OMXNordicMsgHeader *)_currentMsg)->type;
	
	// There is _processorQueue and this message could be assigned to
	// designated thread to process
	logMessage(TRACE, "Processing Message %lu : %lu.%lu",_lastSeqNo, _packet->_recordTime.tv_sec, _packet->_recordTime.tv_nsec);
	
	if(parseFuncMap[type]._couldAssign)
	{
		if(hasInterest(parseFuncMap[type]._callbacks))
		{
			_lineGroup->_mt = globalClock();
#ifdef __SUBMAP_LATENCTY__
			_sub = _lineGroup->getIdSub(getUint64(_currentMsg + parseFuncMap[type]._idOffset, 6));
			getLatency(_lineGroup, _lineGroup->_sf, _lineGroup->_mt);
#else
			_sub = _lineGroup->getIdSub(getUint64(_currentMsg + parseFuncMap[type]._idOffset, 6));
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

