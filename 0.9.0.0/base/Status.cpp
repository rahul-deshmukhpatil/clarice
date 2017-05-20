#include "base/Status.h"

const char* Status::StateToCStr(InstrState state)
{
	switch(state)
	{
		case InstrState::UNKNOWN:
			return "UNKNOWN";

		case InstrState::PREOPEN:
			return "PREOPEN";

		case InstrState::OPEN:
			return "OPEN";

		case InstrState::AUCTION:
			return "AUCTION";

		case InstrState::AUCTION_OPEN:
			return "AUCTION_OPEN";

		case InstrState::AUCTION_INTRA:
			return "AUCTION_INTRA";

		case InstrState::AUCTION_VOLA:
			return "AUCTION_VOLA";

		case InstrState::AUCTION_CLOSE:
			return "AUCTION_CLOSE";

		case InstrState::HALTED:
			return "HALTED";

		case InstrState::CLOSED:
			return "AUCTION_CLOSED";

		case InstrState::SUSPENDED:
			return "SUSPENDED";
	}
}
