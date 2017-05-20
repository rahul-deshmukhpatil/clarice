#ifndef __MILLENIUM_MESSEGES_H__
#define __MILLENIUM_MESSEGES_H__

#include <stdint.h>

#define PACKED __attribute__((packed))

enum MessageType
{
	// System Messages
	LoginRequest = 0x01,
	LoginResponse = 0x02,
	LogoutRequest = 0x05,
	ReplayRequest = 0x03,
	ReplayResponse = 0x04,
	SnapshotRequest = 0x81,
	SnapshotResponse = 0x82,
	SnapshotComplete = 0x83,
	
	// Application messages
	Time = 0x54,
	SystemEvent = 0x53,
	SymDir = 0x52,
	SymStatus = 0x48,
	AddOrder = 0x41,
	OrderDeleted = 0x44,
	OrderModified = 0x55,
	OrderBookClear = 0x79,
	OrderExec = 0x45,
	OrderExecPxSize = 0x43,
	Trade = 0x50,
	OffBookTrade = 0x78,
	TradeBreak = 0x42,
	AuctionInfo = 0x49,
	Statistics = 0x77,
	EnhancedTrade = 0x90,
	RecoveryTrade = 0x76,
};

struct PACKED MillPacketHeader 
{
	uint16_t	length;
	uint8_t		messageCount;
	char		marketDataGroup;
	uint32_t	seqNum;
};

struct PACKED MillMessageHeader
{
	uint8_t length;
	uint8_t type; // MessageType
};

struct PACKED MillLoginRequest
{
	MillMessageHeader header;
	char username[6];
	char passowrd[10];
};

enum class LoginResponseType : char
{
	LOGIN_ACCEPTED	= 'A',
	COMPID_INACTIVE_LOCKED = 'a',
	LOGIN_LIMIT_REACHED = 'b',
	SERVICE_UNAVAILABLE = 'c',
	CONCURRENT_LIMIT_REACHED = 'd',
	FAILED_OTHER = 'e'
};

struct PACKED MillLoginResponse
{
	MillMessageHeader header;
	LoginResponseType status;
};

struct PACKED MillLogoutRequest
{
	MillMessageHeader header;
	//empty struct
};

struct PACKED MillReplayRequest
{
	MillMessageHeader header;
	char		marketDataGroup;
	uint32_t	firstMessage;
	uint16_t	count;
};

enum class ReplayResponseType: char
{
	REQUEST_ACCEPTED = 'A',
	REQUEST_LIMIT_REACHED = 'D',
	INVALID_MARKET_DATA_GROUP = 'I',
	OUT_OF_RANGE = 'O',
	REPLAY_UNAVAILABLE = 'U',
	CONCURRENT_LIMIT_REACHED = 'c',
	UNSUPPORTED_MESSAGE_TYPE = 'd',
	FAILED_OTHER = 'e'
};

struct PACKED MillReplayResponse
{
	MillMessageHeader header;
	char		marketDataGroup;
	uint32_t	firstMessage;
	uint16_t	count;
	ReplayResponseType status;
};

enum class SnapTypeEnum : uint8_t
{
	ORDER_BOOK = 0,
	SYMBOL_STATUS = 1,
	INSTRUMENT = 2,
	TRADES = 3,
	STATISTICS = 4,
	AUCTION_INFO = 6
};

struct PACKED MillSnapshotRequest
{
	MillMessageHeader header;
	uint32_t seqNum;
	char	 segment[6];
	uint32_t instrID;
	char 	 subBook;
	SnapTypeEnum snapType;
	uint64_t recFromTime;
	uint32_t reqID;
};

enum class SnapshotResponseStatus : uint8_t
{
	REQUEST_ACCEPTED = 'A',
	OUT_OF_RANGE = 'O',
	SNAPSHOT_UNAVAILABLE = 'U',
	INVALID_SYMBOL_SEGMENT = 'a',
	REQUEST_LIMIT_REACHED = 'b',
	CONCURRENT_LIMIT_REACHED = 'c',
	UNSUPPORTED_MESSAGE_TYPE = 'd',
	FAILED_OTHER = 'e'
};

struct PACKED MillSnapshotResponse
{
	MillMessageHeader header;
	uint32_t seqNum;
	uint32_t ordCount;
	SnapshotResponseStatus status;
	SnapTypeEnum snapType;
	uint32_t reqID;
};

enum TradingStatusEnum : char 
{
	HALTED = 'H',
	TRADING = 'T',
	AUCTION_OPEN = 'a',
	POSTCLOSE = 'b',
	CLOSED	= 'c',
	AUCTION_CLOSE = 'd',
	AUCTION_REOPEN = 'e',
	END_OF_TRADE_REPORTING = 'v',
	NO_ACTIVE_SESSION = 'w',
	END_OF_POST_CLOSE = 'x',
	PRETRADING = 'y',
	CLOSING_PRICE_PUBL = 'z'
};

struct PACKED MillSnapshotComplete
{
	MillMessageHeader header;
	uint32_t seqNum;
	char	 segment[6];
	uint32_t instrID;
	uint8_t	 reserved;
	uint8_t	 subBook;
	TradingStatusEnum trdStatus;
	SnapTypeEnum snapType;
	uint32_t reqID;
};

/*
 * Number of seconds since midnight. Midnight is defined in terms of the
 * local time for the server
 */
struct PACKED MillTime
{
	MillMessageHeader header;
	uint32_t seconds;
};

enum class EventCodeEnum : char
{
	END_OF_DAY = 'C',
	START_OF_DAY = 'O',
};

struct PACKED MillSystemEvent
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	char	eventCode;
};

enum class SymStatusEnum : char 
{
	ACTIVE = ' ', // It is going to trade
	HALTED = 'H', // It is halted as of now
	SUSPENDED = 'S', // It is suspended state
	INACTIVE = 'a' // Not going to trade
};

struct PACKED MillSymDir
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint32_t instrID;
	uint8_t reserved[2];
	SymStatusEnum status;
	char 	isin[12];
	uint32_t pxBandTol4;
	uint32_t dynCirctBrkrTol4;
	uint32_t stCirctBrkrTol4;
	char	segment[6];
	char	reserved2[6];
	char	currency[3];
	char	reserved3[40];
};

enum class HaltReasonEnum : uint32_t
{
	PRICE_MOVEMENT = '1',
	RECEIVED_ANNOUNCEMENT = '2',
	ANTICIPATION_OF_ANNCMNT = '3',
	SYSTEM_PROBLEMS = '4',
	OTHER = '5',
	REFERENCE_DATA_UPDATE = '6',
	INSTRUMENT_CRCT_BRKR_TRIPPED = 0x00310031, // 101
	MATCHING_PARTITION_SUSPENDED = 0x39393938, // 9998
	SYSTEM_SUSPENDED			 = 0x39393939, // 9999
	REASON_NOT_AVAILABLE		 = ' ', //space
};

enum class SessionChangeReasonEnum : uint8_t
{
	SCHEDULED_TRANSITION = 0,
	EXTND_BY_MKT_SURVEILLANCE = 1,
	SHORTENED_BY_MKT_SURVEILLANCE = 2,
	MKT_ORDER_IMBALANCE = 3,
	PRICE_OUTSIDE_RANGE = 4,
	CIRCUIT_BREAKER_TRIPPED = 5,
	UNAVAILABLE = 9
};

enum class SubBookEnum : uint8_t
{
	REGULAR = 1,
	OFF_BOOK = 2
};

struct PACKED MillSymStatus
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint32_t instrID;
	char	 reserved[2];
	TradingStatusEnum trdStatus; 
	uint8_t  reserved1;
	HaltReasonEnum haltReason;
	SessionChangeReasonEnum sessionChgReason;
	SubBookEnum subBook;
};

enum SideEnum
{
	BUY = 'B',
	SELL = 'S'
};

struct PACKED MillAddOrder
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t orderID;
	char	 side;
	uint64_t quantity;
	uint32_t instrID;
	char	 reserved[2];
	uint64_t px8;
	uint8_t	 flags;
	char	 reserved2[8];
};

struct PACKED MillOrderDeleted 
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t orderID;
	char	 reserved;
	uint32_t instrID;
};

struct PACKED MillOrderModified
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t orderID;
	uint64_t newQuantity;
	uint64_t newPrice8;
	uint8_t	 flags;	
	uint32_t instrID;
	char	 reserved[8];
};

struct PACKED MillOrderBookClear
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint32_t instrID;
	char	 reserved[3];
};

struct PACKED MillOrderExec
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t orderID;
	uint64_t execQuantity;
	uint64_t tradeID;
	uint32_t instrID;
};

struct PACKED MillOrderExecPxSize
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t orderID;
	uint64_t execQuantity;
	uint64_t dispQuantity;
	uint64_t tradeID;
	char	 printable;
	uint64_t px8;
	uint32_t instrID;
	char	 reserved[8];
};

struct PACKED MillTrade
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t execQuantity;
	uint32_t instrID;
	char	 reserved[2];
	uint64_t px8;
	uint64_t tradeID;
	char	 reserved1[30];
};

struct PACKED MillOffBookTrade 
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t execQuantity;
	uint32_t instrID;
	char	 reserved[2];
	uint64_t px8;
	uint64_t tradeID;
	char	 tradeType[4];
	char 	 tradeTime[8]; // MMDDYYYY
	char	 tradeDate[8]; // HH:MM:SS
	char	 tradedCurrency[4];
	uint64_t originalPrice8;
	char	 execVenue[5];
	char	 flag;
	char	 isin[12];
	char	 reserved1[13];
};

enum class TradeTypeEnum : char
{
	ONBOOK_TRADE = 'T',
	OFFBOOK_TRADE = 'N'
};

struct PACKED MillTradeBreak 
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t tradeID;
	TradeTypeEnum tradeType;
	uint32_t instrID;
	char	 isin[12];
};

enum class AuctionType : char
{
	AUCTION_REOPEN = 'A',
	AUCTION_CLOSE = 'C',
	AUCTION_OPEN = 'O',
};

struct PACKED MillAuctionInfo
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t pairedQuantity;
	char	 reserved[9];
	uint32_t instrID;
	char	 reserved1[2];
	uint64_t px8;
	char	 auctionType;
};

enum class StatisticsTypeEnum : char
{
	OPEN_PRICE = 'O',
	CLOSE_PRICE = 'C',
	PREV_CLOSE_PRICE = 'P'
};

enum class PriceIndEnum : char
{
	AUCTION_TRADE = 'A',
	REGULAR_TRADE = 'B',
	MID_OF_BBO	  = 'C',
	LAST_REGULAR_TRADE = 'D',
	LAST_AUCTION_TRADE = 'E',
	MANUAL = 'F',
	PREV_CLOSE =  'I',
	BEST_BID = 'U',
	BEST_OFFER = 'V',
	NONE = 'W', //used when publishing previous close px
	PRICE_UNAVAILABLE = 'Z' //used when a value is cleared manually by market surveillance
};

struct PACKED MillStatistics 
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint32_t instrID;
	char	 reserved[2];
	StatisticsTypeEnum statistics;
	uint64_t px8;
	char	 openCloseIndicator;
	char	 reserved1;
};

enum class ActionTypeEnum : char
{
	CANCELLED_TRADE = 'C',
	TRADE = 'N'
};

struct PACKED MillEnhancedTrade 
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t execQuantity;
	uint32_t instrID;
	char	 reserved[2];
	uint64_t px8;
	char	 reserved1[8];
	char	 auctionType;
	char	 offBookTradeType[4];
	uint64_t tradeID;
	char 	 tradeTime[8]; // MMDDYYYY
	char	 tradeDate[8]; // HH:MM:SS
	ActionTypeEnum actionType;
	char	 tradedCurrency[4];
	uint64_t originalPrice8;
	char	 execVenue[5];
	char	 flag;
	char	 reserved2[29];
	char	 buyerID[11];
	char	 sellerID[11];
};

struct PACKED MillRecoveryTrade 
{
	MillMessageHeader header;
	uint32_t nanoSecs;
	uint64_t execQuantity;
	uint32_t instrID;
	char	 reserved[2];
	uint64_t px8;
	uint64_t tradeID;
	char	 auctionType;
	char	 offBookTradeType[4];
	char 	 tradeTime[8]; // MMDDYYYY
	char	 tradeDate[8]; // HH:MM:SS
	char	 actionType;
	char	 tradedCurrency[4];
	uint64_t originalPrice8;
	char	 execVenue[5];
	char	 flag;
	char	 reserved1[29];
	char	 buyerID[11];
	char	 sellerID[11];
};
#endif //__MILLENIUM_MESSEGES_H__
