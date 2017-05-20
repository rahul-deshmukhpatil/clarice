#ifndef __BASE_H__
#define __BASE_H__

#include <stdint.h>
#include <sys/time.h>

#include "base/BaseCommon.h" 

namespace base 
{
	enum UpdateType
	{
		ORDER_UPDATE = 'O',
		TRADE_UPDATE = 'T',
		BOOK_UPDATE	= 'B',
		PRICELEVEL_UPDATE	= 'L',
		QUOTE_UPDATE = 'Q',
		CUSTOM_UPDATE = 'C',
		STATUS_UPDATE = 'S',
		PRODUCTINFO_UPDATE= 'P',
		UNKNOWN_UPDATE = 'U'
	};

	class Base
	{
		public:
			// Called when uniq object per Sub is created. 
			// I.e Trade, Order, Custom and Book in Sub
			inline Base(Subscription *sub, UpdateType type)
				: _line(nullptr)
				, _seqNum(0)
				, _xt{.tv_sec = 0, .tv_nsec = 0}
				, _sub(sub)
				, _type(type)
				, _msgType(0)
			{
			}

			//Order init, trade init
			// When dynamically allocated objects from pool are initialized
			// Or temporary offbook trades on function stack are constructed
			inline void update(Line *line, Subscription *sub, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, UpdateType type)
			{
				_line = line;
				_seqNum = seqNo;
				_xt.tv_sec = exSeconds;
				_xt.tv_nsec = exUSeconds;
				_sub = sub;
				_type = type;
				_msgType = msgType;
			}
			
			// When uniq object inside sub is modified
			// Uniq objects inside the sub are expted to arrive only on same line and same thread
			// Trade, status, custom, Order modify and remove
			inline void update(Line *line, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds)
			{
				_line = line;
				_seqNum = seqNo;
				_xt.tv_sec = exSeconds;
				_xt.tv_nsec = exUSeconds;
				_msgType = msgType;
			}

			inline void update(uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds)
			{
				_seqNum = seqNo;
				_xt.tv_sec = exSeconds;
				_xt.tv_nsec = exUSeconds;
				_msgType = msgType;
			}

			// Or temporary offbook trades on function stack are constructed
			inline Base(Line *line, Subscription *sub, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, UpdateType type)
				: _line(line)
				, _seqNum(seqNo)
				, _xt{.tv_sec = exSeconds, .tv_nsec = exUSeconds}
				, _sub(sub)
				, _type(type)
				, _msgType(msgType)
			{
			}

			//dynamically allocated objects are freed
			// or subscription is deleted
			inline ~Base()
			{
				_seqNum = 0;
				_xt.tv_sec = 0;
				_xt.tv_nsec = 0;
				_sub = nullptr;
				_type = UNKNOWN_UPDATE;
				_msgType = 0;
			}

			Line *_line;
			uint64_t _seqNum;
			timespec  _xt;
			Subscription *_sub;
			UpdateType _type;
			uint32_t _msgType;
	};
}

#endif //__BASE_H__
