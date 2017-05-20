#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>
#include <netinet/in.h> 
#include <unistd.h>
#include <sys/syscall.h>

#include "infra/containers/spsc/BoostSPSCQueue.h"

#include "base/BaseCommon.h"

using namespace base;
using namespace infra;

namespace base 
{
	#define	PACKET_DATA_SIZE 1500
	class Packet : public BoostSPSCQueueElement
	{
		public:
			const PacketAddress	*_packetAddress;

			Packet()
			: _packetAddress(nullptr)
			, _packetDataLen(0)
			, _rt{.tv_sec = 0, .tv_nsec = 0}
			, _recordTime{.tv_sec = 0, .tv_nsec = 0}
			{
			}
		
			~Packet()
			{
				//@TODO: below assignement is not compulsory as it will
				//		be alway over written
				//		and could be included only in debug builds.
				//		and could be removed from release build
				_packetAddress = nullptr;
				_packetDataLen = 0;
				_rt.tv_sec = 0;
				_rt.tv_nsec = 0;
				_recordTime.tv_sec = 0;
				_recordTime.tv_nsec = 0;
				//fprintf(stderr, "%ld deleting : %p \n", syscall(SYS_gettid), this);
				//Call the base BoostSPSCQueueElement destructor
			}

			void setPacketDataLen(uint32_t packetDataLen)
			{
				_packetDataLen = packetDataLen;
			}
			
			uint32_t packetDataLen()
			{
				return _packetDataLen;
			}

			char *packetData()
			{
				return _packetData;
			}

			timespec         _rt; //recieve time
			timespec			_recordTime;
		private:
			char			_packetData[PACKET_DATA_SIZE];
			uint32_t		_packetDataLen;
	};
}

#endif //__PACKET_H__
