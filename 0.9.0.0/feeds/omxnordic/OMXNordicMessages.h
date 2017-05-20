#ifndef __OMXNORDIC_MESSEGES_H__
#define __OMXNORDIC_MESSEGES_H__

#include <stdint.h>

#define PACKED __attribute__((packed))

enum MessageType
{
	// System Messages
	Time = 'T',
	MSeconds = 'M',
	SystemEvent = 'S',
	StateMessage = 'O',
	RefData = 'R',
	TradingState= 'H',
	AddOrder = 'A',
	AddOrderMPID = 'F',
	OrderExec = 'E',
	OrderExecPx = 'C',
	OrderCancel = 'X',
	OrderDelete = 'D',
	Trade = 'P',
	CrossTrade = 'Q',
	BrokenTrade = 'B',
	Imbalance = 'I',
};

struct PACKED OMXNordicPacketHeader
{
	char session[10];
	uint32_t seqNum;
	uint16_t msgCount; 
};

struct PACKED OMXNordicMsgHeader
{
	uint16_t reserved1;
	uint8_t type;
};

struct PACKED OMXNordicTime
{
	OMXNordicMsgHeader header;
	uint32_t seconds;
	char	 reserved;
};

struct PACKED OMXNordicMSeconds
{
	OMXNordicMsgHeader header;
	uint16_t msecond;
	char reserved;
};

struct PACKED OMXNordicSystemEvent
{
	OMXNordicMsgHeader header;
	char eventCode; // O: Start of day, C : last message
};

struct PACKED OMXNordicStateMessage
{
	OMXNordicMsgHeader header;
	char segmentID[3];
	char eventCode;
};

struct PACKED OMXNordicRefData
{
	OMXNordicMsgHeader header;
	char symbolID[6];
	char symbol[16];
	char isin[12];
	char productType[3];
	char currency[3];
	char mic[4];
	char segmentID[3];
	uint64_t noteCodes;
	char roundLotSize[9];
};

enum TradingStatusEnum 
{
	HALTED = 'H',
	AUCTION = 'Q',
	TRADING = 'T'
};

struct PACKED OMXNordicTradingState
{
	OMXNordicMsgHeader header;
	char symbolID[6];
	char tradingState;
	char reserved;
	char reason[4];
};

struct PACKED OMXNordicAddOrder
{
	OMXNordicMsgHeader header;
	char orderID[9];
	char side;
	char quantity[9];
	char symbolID[6];
	char price[10];
};

struct PACKED OMXNordicAddOrderMPID
{
	OMXNordicMsgHeader header;
	char orderID[9];
	char side;
	char quantity[9];
	char symbolID[6];
	char price[10];
	uint32_t orderAttr;
};

struct PACKED OMXNordicOrderExec
{
	OMXNordicMsgHeader header;
	char orderID[9];
	char execQuantity[9];
	char tradeID[9];
	char participent[4];
	char counterParty[4];
};

struct PACKED OMXNordicOrderExecPx
{
	OMXNordicMsgHeader header;
	char orderID[9];
	char execQuantity[9];
	char tradeID[9];
	char printable;
	char tradePrice[10];
	char participent[4];
	char counterParty[4];
};

struct PACKED OMXNordicOrderCancel
{
	OMXNordicMsgHeader header;
	char orderID[9];
	char cancelledQty[9];
};

struct PACKED OMXNordicOrderDelete
{
	OMXNordicMsgHeader header;
	char orderID[9];
};

struct PACKED OMXNordicTrade
{
	OMXNordicMsgHeader header;
	char orderID[9];
	char tradeType;
	char quantity[9];
	char symbolID[6];
	char tradeID[9];
	char price[10];
	char participent[4];
	char counterParty[4];
};

struct PACKED OMXNordicCrossTrade
{
	OMXNordicMsgHeader header;
	char quantity[9];
	char symbolID[6];
	char crossPrice[10];
	char orderID[9];
	char crossType;
	char noofTrades[10];
};

struct PACKED OMXNordicBrokenTrade
{
	OMXNordicMsgHeader header;
	char tradeID[9];
};

struct PACKED OMXNordicImbalance
{
	OMXNordicMsgHeader header;
	char pairedQty[9];
	char imbalanceQty[9];
	char imbalanceDir;
	char symbolID[6];
	char eqPrice[10];
	char crossType;
	char bestBidPx[10];
	char bestBidQty[9];
	char bestAskPx[10];
	char bestAskQty[9];
};
#endif //__OMXNORDIC_MESSEGES_H__
