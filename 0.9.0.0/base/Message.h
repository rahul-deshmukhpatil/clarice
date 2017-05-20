#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "base/Packet.h"

namespace base
{
	//Line -> Assigner
	// Ensure that _customData member is always alligned
	// As it is filled with meaning full structs in native feed
	#define CUSTOM_DATA_SIZE 20

	class MessageInfo
	{
		public:
			MessageInfo()
			{
				_lastSeqNo = 0;
				_xtSecs = 0;
			}

			MessageInfo(uint64_t seq, uint64_t xtSecs)
			{
				_lastSeqNo = seq;
				_xtSecs = xtSecs;
			}
			uint64_t _lastSeqNo;
			uint64_t _xtSecs;
	};


	class Message : public BoostSPSCQueueElement
	{
		public: 
		Message()
		: _packet(nullptr)
		, _func(nullptr)
		, _currentMsg(nullptr)
		, _sub(nullptr)
		, _msgInfo()
		{
		}

		Message(std::shared_ptr<Packet> &ptr, NativFunc func, const char *msg, Subscription *sub, uint64_t seq, uint64_t xtSecs)
		: _packet(ptr)
		, _func(func)
		, _currentMsg(msg)
		, _sub(sub)
		, _msgInfo(seq, xtSecs)
		{
			//@TODO: remove the zero initialization
			//memset(_customData, 0, sizeof(_customData));
			//write the custom data with placement constructor 
			// later in native process funct before coming here
		}

		// This destructor is neccesory to immediatly
		// free the packet held in the shared pointer 
		// and is called by the deleter of the SPSC queue element 
		~Message()
		{
			//@TODO: below assignement is not compulsory as it will
			//		be alway over written
			//		and could be included only in debug builds.
			//		and could be removed from release build
			_func = nullptr;
			_sub  = nullptr;
			_currentMsg = nullptr;
			// Here it will unintizlize the shared ptr message->_packet with destructor
			//Call the base BoostSPSCQueueElement destructor
		}

		std::shared_ptr<Packet> _packet;
		NativFunc				_func;
		Subscription			*_sub;
		const char 				*_currentMsg;
		MessageInfo				_msgInfo;
	};

	}
#endif // __MESSAGE_H__



