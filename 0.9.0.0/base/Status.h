#ifndef __STATUS_H__
#define __STATUS_H__

#include <stdint.h>
#include "base/BaseCommon.h"
#include "base/Base.h"

namespace base
{
	enum class InstrState : uint8_t
	{
		UNKNOWN	= 0,
		PREOPEN	= 1,
		OPEN	= 2,
		AUCTION	= 3,
		AUCTION_OPEN	= 4,
		AUCTION_INTRA	= 5,
		AUCTION_VOLA	= 6,
		AUCTION_CLOSE	= 7,
		HALTED		= 8,
		CLOSED		= 9,
		SUSPENDED	= 10,
		// Add new case for new state below in StateToCStr function
	};

	class Status: public Base
	{
		public:
			Status(Subscription *sub);
			
			void update(Line *, Subscription *, uint32_t, uint64_t, uint32_t, uint32_t, InstrState);

			static const char* StateToCStr(InstrState state);

			//_prev state must come before the _state as constructor assumes
			// order of declaration in initializer list
			InstrState _prevState;
			InstrState _state;
	};
}

#include "base/Subscription.h"
#include "base/Line.h"
#include "base/API.h"

using namespace base;

inline Status::Status(Subscription *sub)
	: Base(sub, STATUS_UPDATE)
	, _state(InstrState::UNKNOWN) 
	, _prevState(InstrState::UNKNOWN)
{
}

inline void Status::update(Line *line, Subscription *sub, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, InstrState state)
{
	Base::update(line, msgType, seqNo, exSeconds, exUSeconds);
	_prevState = _state;
	_state = state;
	LineAPI *api = _line->_lineAPI;
	(api->*(api->_onStatus))(this);
}
#endif //__STATUS_H__
