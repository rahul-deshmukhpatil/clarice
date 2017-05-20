#ifndef __EURONEXT_MESSEGES_H__
#define __EURONEXT_MESSEGES_H__

#include <stdint.h>

#define PACKED __attribute__((packed))

enum MessageType
{
	// System Messages
	
	// Application messages
};

enum PacketType
{
	DATA_PACKET = 501,
	SEQ_RESET = 1,
	HB_PACKET = 2,
	RETRANSE_RESPONSE	= 10,
	RETRANSE_REQUEST	= 20,
	SNAPSHOT_REQUEST	= 22, // REFRESH_REQUST
	SNAPSHOT_RESPONSE	= 23, // REFRESH_RESPONSE
	HB_RESPONSE			= 24
}

enum MessageType
{
	REFRESH_START = 580,
	REFRESH_END	  = 581,
}

struct PACKED EnxtPacketHeader 
{
	uint16_t length;
	uint16_t packetType;
	uint32_t seqNum;
	uint32_t sendTime;
	uint16_t serviceID;
	uint8_t	 deliveryFlag; // 0 - Real Time Uncompressed, 1 - Retransmission uncompressed, 17 - refresh zlib compresed
	uint8_t	 numOfMsgs;		// 0 in case of HB packet
};	//16

struct PACKED EnxtSeqReset // day one packet, with seq 1
{
	uint32_t nextSeqNum;	// this must be 2
}; //4

struct PACKED EnxtHBResBody
{
	char	sourceID[20]; // source	(client) requesting retransmission
};

struct PACKED EnxtRetrReq
{
	uint16_t length;	 // 44 = 16 header + 24 
	uint16_t packetType; // set to 20
	uint32_t seqNum;	 // Optional
	uint32_t sendTime;	 // Optional
	uint16_t serviceID;	 // Service ID of the broadcast stream/MC Line 
	uint8_t	 deliveryFlag;  // always 0
	uint8_t	 numOfMsgs;		// 1 message  
}; // 16

struct PACKED EnxtRetrReqBody
{
	uint32_t beginSeqNum;
	uint32_t endSeqNum;
	char	sourceID[20]; // source	(client) requesting retransmission
}; // 28

enum EnxtRejectReason
{
	ACCEPTED = 0,
	PERMISSION = 1,	// the ServiceID is not granted for the SourceID or an incorrect Source ID has been used
	INVALID_SEQ   = 2, // Rejected due to invalid sequence range
	MAX_SEQ_RANGE = 3, // Rejected due to max sequence range reached (> thresholds)
	MAX_REQ		  = 4, // Rejected due to max request reached in a day (> thresholds)
	NOT_AVAILABLE = 5, // Requested packets are no longer available
	INVALID_REQ	  = 6, // Retransmission request incorrectly formatted
	INV_SERV_ID	  = 7  // Rejected – Due to incorrect ServiceID
}

struct PACKED EnxtRetrResBody
{
	uint32_t sourceSeqNum; // Requested packet seq num
	char    sourceID[20]; // source (client) requesting retransmission
	uint8_t	status;	// A = Accepted, R = Rejected
	uint8_t rejectReason;
	uint8_t filler[2]; // reserved 
}; // 28

struct PACKED EnxtRefreshReqBody
{
	char	sourceID[20]; // source	(client) requesting retransmission
}; // 20

enum EnxtResponseRejectReason
{
	REFRESH_ACCEPTED = 0,
	REFRESH_PERMISSION = 1, // the ServiceID is not granted for the SourceID or an incorrect Source ID has been used
	REFRESH_INV_SERV_ID  = 2,  // Rejected – Due to incorrect ServiceID
	REFRESH_INVALID_REQ = 3, // Refresh request incorrectly formatted
	REFRESH_INVALID_SEQ   = 4, // Rejected due to incorrect packet type sent
	REFRESH_MAX_REQ = 5, // Rejected due to max request reached in a day (> thresholds)
	REFRESH_MAX_SEQ_RANGE = 6, // Rejected due unavailibility of refresh data 
	REFRESH_NOT_AVAILABLE = 7 // Refresh request rejected as sent to incorrect server (secondary instead of primary)
}

struct PACKED EnxtRefreshResBody
{
	uint32_t sourceSeqNum; // Requested packet seq num
	char    sourceID[20]; // source (client) requesting retransmission
	uint8_t	status;	// A = Accepted, R = Rejected
	uint8_t rejectReason;
	uint8_t filler[2]; // reserved 
}; // 28

struct PACKED EnxtMsgHeader 
{
	uint16_t msgSize;
	uint16_t msgType;
};

struct PACKED EnxtStartRefresh
{
	EnxtMsgHeader header;
	uint32_t lastSeqNum; // lastSeqNum + 1 = next expected packet on MC line
}; // 8

struct PACKED EnxtRefreshEnd
{
	EnxtMsgHeader header;
	uint32_t lastSeqNum; // lastSeqNum + 1 = next expected packet on MC line
}; // 8

enum EnxtInstrState
{
	NA	= 0,
	AUCTION = 'A',
	HALTED = 'H',
	NULL_STATE = ' '
}

enum EnxtTradingState
{
	NA	= 0,
	SUSPENDED = 'S', // Suspended, Calculation suspended for indices 
	CALC_RESUMED = 'R' // For indices 
}

enum EnxtHaltReason
{
	NA	= 0,
	HALTED = 'R', // No Liquidity provider
	COLLAR = 'C', // Opening/trading price is outside of dynamic collars
	MANUAL_HALT = 'M', // By Market operations
	NULL_STATE = ' ' // No Info avaialable 
}

enum EnxtActAffectingState
{
	NA	= 0,
	TRADING = 'C',
	CANCLED_PROGRAM_OPENING = 'D',
	MANUAL_HALT = 'M',
	INIT_STAGE = 'N',
	OPENED = 'O',
	DEFFERED_PROGRAM_OPENING = 'P',
	AUTO_HALT = 'R',
	ONE_SIDE_PERIOD = 'Y',
	END_ONE_SIDE = 'Z'
	ORDER_ENTRY_CHANGE = ' '
}

struct PACKED EnxtStockStateChange
{
	EnxtMsgHeader header;
	uint32_t symbol;
	uint32_t sourceSeq; // assinged by system
	uint32_t seconds; // sourceTime since midgnight
	uint32_t symtemId; // Exchange/System ID
	uint16_t useconds; 
	uint8_t reserved[2];
	char	startDate[8]; // YYYYMMDD
	char	startTime[6]; // HHMMSS
	char	preOpenTime[6]; // HHMMSS
	char	orderEntryRejection; // 0 : N/A, N: Allowed, Y: Forbidden, Nulli ' '- Not provided
	char	instrState;
	char	tradingState;
	char	haltReason;
	char	actAffectState;
	char	instrStateTCS;
	char	periodSide;
	char	reserved1;
}; //52

enum EnxtSession
{
	EARLY_SESSION  = 'E',
	CLOSE_SESSION = 'C',
	LATE_SESSION  = 'L'
}

enum EnxtClassState
{
	EAMO = 1, //	
}

struct PACKED EnxtClassStateChange
{
	EnxtMsgHeader header
	uint32_t sourceSeq; // assinged by system
	uint32_t seconds; // sourceTime since midgnight
	uint32_t symtemId; // Exchange/System ID
	uint16_t useconds; 
	char	 instrGroupCode[3];
	char	 reserved;
	char	 session;
	char	 reserved1;
	uint32_t classState;
	char	startDate[8]; // YYYYMMDD
	char	startTime[6]; // HHMMSS
	char	preOpenTime[6]; // HHMMSS
	char	orderEntryRejection; // 0 : N/A, N: Allowed, Y: Forbidden, Nulli ' '- Not provided
	char	instrState;
	char	tradingState;
	char	haltReason;
	char	actAffectState;
	char	instrStateTCS;
	char	periodSide;
	char	reserved1;
}; 

struct PACKED EnxtMatchPrice // Speculative Open price
{
	EnxtMsgHeader header
	uint32_t symbol;
	uint32_t sourceSeq; // assinged by system
	uint32_t seconds; // sourceTime since midgnight
	uint32_t imPrice; // calculated with the PriceScaleCode
	uint32_t variation; // calculated with the VariationScaleCode
	uint32_t symtemId; // Exchange/System ID
	uint16_t useconds;
	uint8_t  priceScale;
	uint8_t  variationScale;
	uint32_t imvolume; // Volume that would	be exchanged if Auction occurred at this moment
};

struct PACKED EnxtTCSStateChange
{
	EnxtMsgHeader header
	uint32_t sourceSeq; // assinged by system
	uint32_t seconds; // sourceTime since midgnight
	uint32_t symtemId; // Exchange/System ID
	uint16_t useconds;
	char	 instrGroupCode[3];
	char	 changeTradingCycle;
	char	 reserved[2];
};

struct PACKED EnxtCollar
{
	EnxtMsgHeader header
	uint32_t symbol;
	uint32_t sourceSeq; // assinged by system
	uint32_t seconds; // sourceTime since midgnight
	uint32_t highCollar; // higher bound
	uint32_t lowCollar; // higher bound
	uint32_t symtemId; // Exchange/System ID
	uint16_t useconds;
	uint8_t	 highScaleCode;	
	uint8_t	 lowScaleCode;	
}; //32

struct PACKED EnxtTimeTable
{
	EnxtMsgHeader header
	uint32_t sourceSeq; // assinged by system
	uint32_t seconds; // sourceTime since midgnight
	uint32_t symtemId; // Exchange/System ID
	uint16_t useconds;
	char	 instrGroupCode[3];
	char	 session;
	char	 timePreOpen1[6];
	char	 timeOpen1[6];
	char	 timeClose1[6];
	char	 timePreOpen2[6];
	char	 timeOpen2[6];
	char	 timeClose2[6];
	char	 timePreOpen3[6];
	char	 timeOpen3[6];
	char	 timeClose3[6];
	char	 eod[6];
	char	 reserved[2];
}; // 84

// Each time Reference Data messages (553) are sent, prior to the first 553 message
// There is a 550 message per multicast group containing 553 messages

struct PACKED EnxtStartRefData
{
	EnxtMsgHeader header
	char	 indicator; // This field indicates the start of the Instrument	characteristic flow. Always takes the value 'S'.
	char	 filler[3];
};

struct PACKED EnxtEndRefData
{
	EnxtMsgHeader header
	char	 indicator; // This field indicates the end of the Instrument	characteristic flow. Always takes the value 'E'.
	char	 filler[3];
}

struct PACKED EnxtRefData
{
	EnxtMsgHeader header
	uint32_t symbol;
	uint32_t sourceSeq; // assinged by system
	uint32_t seconds; // sourceTime since midgnight
	uint32_t prevClosePrice;
	uint32_t systemID;
	uint32_t prevTradinVol; // on last trade date
	uint32_t tickSize; // only for tradable instruments
	uint16_t useconds;
	uint16_t exchangeCode;
	uint16_t instrType;
	char	 eventDate[8];
	char	 instrName;
	char	 periodIndicator; // M morning : effective today, E evening: effective next trading day
	char	 typeOfMarket;
	char	 countryCode; // Country code of location for the corporate headquarters of the company that issued the instrument.
	char	 currency[3];
	char	 instrGroupCode[3];
	char	 instrCategory;
	char	 isin[12];
	char	 lastTradingDay;
	char	 undIsin[12];
	char	 expiryDate[8];
	char	 firstSettlementDate[8];
	char	 typeOfDerivatives;
	char	 BICDepository[11];
	char	 ICB[4];
	char	 mic[4];
	char	 undWarrantIsin[12];
	char	 depositoryList[25];
	char	 mainDepository[5];
	char	 typeOfCarporateEvent[2];
	char	 timeLagEuronextUTC[5]; //Effective difference time between CET (Euronext time) and UTC.
	char	 timeLagMiFIDRegUTC[5]; //Effective time difference between	MiFID regulators and UTC
	char	 CFI[6];
	char	 quantityNotation[4]; // Specifies the nature of the amount expression used for negotiating the	instrument on the market.
	char	 indexSetOfVarPriceTick; // Tick size based on the price of instrument.
	char	 marketFeedCode[2];
	char	 MICList[24];
	char	 industryCode[4];
	char	 reserved[4];
	char	 finMktCode[3];
	char	 USindicator[7];
	char	 reserved1[2];
	uint64_t prevDayCapTraded;
	uint64_t nomMktPrice;
	uint64_t lotSize;
	uint64_t numInstrCirc;
	uint64_t sharesOut;
	uint64_t authShares;
	char	 reserved2[3];
	char	 repoIndiactor;
	uint8_t  lastAdjPriceScaleCode;
	uint8_t  typeOfUnitExp;
	uint8_t  marketIndicator;
	uint8_t  prevDayCapScaleCode;
	uint8_t  taxcode;
	uint8_t  nomMktPriceScaleCode;
	uint8_t  lotSizeScaleCode;
	uint8_t  fixPriceTickScaleCode;
	char	 menmonic[5];
	char	 tradingCode[12];
	char	 reserved3[3];
	uint32_t strikePrice;
	char	 strikeCur[3];
	uint8_t  strikeScaleCode;
	uint32_t currencyCoef;
	uint8_t  currencyCoefScaleCode;
	uint8_t  tradingCurIndicator;
	uint8_t  strikeCurIndicator;
	char	 reserved4;
};

struct PACKED EnxtSettlementice
{
	EnxtMsgHeader header
	uint32_t symbol;
	uint32_t seconds; // sourceTime since midgnight
	uint32_t sourceSeq; // assinged by system
	uint32_t price;
	uint32_t systemID;
	uint16_t useconds;
	uint8_t priceScale;
	char	reserved;
};

struct PACKED EnxtAuctionSummery
{
	EnxtMsgHeader header
	uint32_t symbol;
	uint32_t seconds; // sourceTime since midgnight
	uint32_t sourceSeq; // assinged by system
	uint32_t firstPrice;
	uint32_t lastPrice;
	uint32_t highestPrice;
	uint32_t lowestPrice;
	uint32_t cumQuantity;
	uint32_t variation;
	uint32_t systemID;
	uint16_t useconds;
	uint16_t lastPriceType;
	char	 tickDirection;
	char	 instrValPrice;
	uint8_t priceScale;
	uint8_t variationScale;
};

struct PACKED EnxtQuote
{
	EnxtMsgHeader header
	uint32_t symbol;
	uint32_t sourceSeq; // assinged by system
	uint32_t seconds; // sourceTime since midgnight
	uint32_t quoteLinkIDl;
	uint32_t askPrice;
	uint32_t askSize;
	uint32_t bidPrice;
	uint32_t bidSize;
	uint32_t systemID;
	uint16_t askOrders;
	uint16_t bidOrders;
	uint16_t useconds;
	uint8_t  askMktOrder;
	uint8_t  bidMktOrder;
	char	 quoteCond;
	uint8_t  quoteNumber;
	uint32_t priceScale;
	char	 reserved;
};

struct PACKED EnxtWeightedAvgSpread
{
	EnxtMsgHeader header
	uint32_t symbol;
	uint32_t sourceSeq; // assinged by system
	uint32_t seconds; // sourceTime since midgnight
	uint32_t buyPrice;
	uint32_t sellPrice;
	uint32_t moneyAmount;
	uint32_t systemID;
	uint16_t useconds;
	uint8_t  priceScale;
	uint8_t  moneyScale;
};

struct PACKED EnxtOrderUpdate
{
	EnxtMsgHeader header
	uint32_t symbol;
	uint32_t seconds; // sourceTime since midgnight
	uint32_t sourceSeq; // assinged by system
	uint32_t price;
	uint32_t aggrVol;
	uint32_t volume;
	uint32_t linkId;
	uint32_t orderID;
	uint32_t systemID;
	uint16_t useconds;
	uint16_t numOfOrders;
	char	 side;
	char	 OrderType;
	char	 action;
	char	 priceScale;
	uint32_t orderDate;
	uint32_t orderPrioDate;
	uint32_t orderPrioTime;
	uint16_t useconds;
	uint16_t reserved;
};

struct PACKED EnxtOrderRetransmission
{
	EnxtMsgHeader header	
	uint32_t seconds; // sourceTime since midgnight
	uint32_t sourceSeq; // assinged by system
	uint16_t tradingEngineID;
	uint8_t  instanceID;
	char	 retranseIndicator;
} // 16

struct PACKED EnxtRTindex
{
	EnxtMsgHeader header
	uint32_t symbol; 
	uint32_t seconds; // sourceTime since midgnight
	uint32_t indexLevel;
	uint32_t foreRunner;
	uint32_t sessionHigh;
	uint32_t sessionLow;
	uint32_t perctCapital;
	uint32_t varFromPrevDay;
	uint32_t systemID;
	uint16_t useconds;
	uint16_t numOfSecs;
	char	 indexCode;
	char	 typeOfLevel;
	uint8_t  indexScale;
	uint8_t  perctScale;
	uint8_t  varScale;
	uint8_t  rebroadcastInd;
	char	 reserved[2];
};

struct PACKED EnxtIndexSummery
{
	EnxtMsgHeader header
	uint32_t symbol; 
	uint32_t seconds; // sourceTime since midgnight
	uint32_t prelOpeningLevel;
	uint32_t prelOpeningTime;
	uint32_t openingLevel;
	uint32_t openingTime;
	uint32_t confirmRefLevel;
	uint32_t confirmRefTime;
	uint32_t percentVarPrevClose;
	uint32_t highLevel;
	uint32_t highTime;
	uint32_t lowLevel;
	uint32_t lowTime;
	uint32_t clearingLevel;
	uint32_t clearingTime;
	uint32_t liqTime;
	uint32_t systemID;
	uint16_t useconds;
	char	 typeOfLevel;
	uint8_t  levelScale;
	uint8_t  varScale;
	char 	 reserved[3];
};

struct PACKED EnxtIndexComp
{
	EnxtMsgHeader header
	uint32_t symbol; 
	uint32_t seconds; // sourceTime since midgnight
	uint32_t instrWeight;
	uint32_t instrFactor;
	uint32_t prevClose;
	uint32_t systemID;
	uint16_t useconds;
	uint16_t numOfComp;
	char	 isin[12];
	char	 mnemo[5];
	char	 indexFreq;
	uint8_t	 priceScale;
	uint8_t	 levelScale;
};
#endif //__EURONEXT_MESSEGES_H__
