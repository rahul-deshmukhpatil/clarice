#ifndef __PACKETDATA_H__
#define __PACKETDATA_H__

using namespace infra;

namespace base
{

#define PACKET_SIZE 1500
	enum PacketState
	{
		FREED 	= 0,
		WRITTEN = 1,
		READ	= 2,
		PacketStateMax	
	};
	

	enum ChannelType
	{
		UNKNOWN = 0,
		
		/**
		 * Primary/Secondary real time data MC line
		 */
		PRIM_MC_A = 1,
		PRIM_MC_B = 2,
		SEC_MC_A  = 3,	
		SEC_MC_B  = 4,	
		
		/**
		 * Primary/Secondary snapshot data MC line
		 */
		PRIM_MC_SNAP_A = 1,
		PRIM_MC_SNAP_B = 2,
		SEC_MC_SNAP_A  = 3,	
		SEC_MC_SNAP_B  = 4,	
		
		/**
		 * Primary/Secondary selective retranse data MC line
		 */
		PRIM_MC_RETRANSE_A = 1,
		PRIM_MC_RETRANSE_B = 2,
		SEC_MC_RETRANSE_A  = 3,	
		SEC_MC_RETRANSE_B  = 4,	

		/**
		 * GLOBAL Primary/Secondary snapshot data MC line
		 */
		GLOBAL_PRIM_MC_SNAP_A = 1,
		GLOBAL_PRIM_MC_SNAP_B = 2,
		GLOBAL_SEC_MC_SNAP_A  = 3,	
		GLOBAL_SEC_MC_SNAP_B  = 4,	

		/**
		 * GLOBAL Primary/Secondary selective retranse data MC line
		 */
		GLOBAL_PRIM_MC_RETRANSE_A = 1,
		GLOBAL_PRIM_MC_RETRANSE_B = 2,
		GLOBAL_SEC_MC_RETRANSE_A  = 3,	
		GLOBAL_SEC_MC_RETRANSE_B  = 4,	
	};

	class PacketQueueElement 
	{
		PacketState	_packetState;	//PacketState in the Queue
	};

	class PacketData
	{
		TimeStamp	_time; 			// Time when packet was recieved by market data server
		ChannelType	_channelType;	// If this is mc/tcp, main/retranse/snap channel		
		Connection* _connection;	// Connection to which PacketData belongs to, Initialised at constant time
		LineHandler* _lineHandler;	// LineHandler which gonna process this packet;
		size_t		_packetLen;		// Length of valid data in below char array
		char		_packetData[PACKET_SIZE]; // Actual contents of the packet
	};		
}
#endif // __PACKETDATA_H__
